// Copyright(c) 2015-2016 Melvyn Laïly
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include <iostream>
#include <time.h> 
#include <windows.h>
#include "Commctrl.h"

#include "xmpdsp.h"
#include "xmpfunc.h"

#include "SharpScrobblerWrapper.h"
#include "main.h"

// Magic number for MultiByteToWideChar()
// to auto detect the length of a null terminated source string
#define AUTO_NULL_TERMINATED_LENGTH     -1

static HINSTANCE hDll;
static HWND xmplayWinHandle = NULL;

static XMPFUNC_MISC* xmpfmisc;
static XMPFUNC_STATUS* xmpfstatus;

static ScrobblerConfig pluginConfig;

static SharpScrobblerWrapper* pluginWrapper = NULL;

static const char* currentFilePath = NULL;

static TRACK_INFO* currentTrackInfo = NULL;

// Sample rate of the current track, by 1000.
static DWORD xmprateBy1000 = 0;
// Total number of samples processed by the DSP for the current track.
static DWORD processedSamplesForCurrentTrack = 0;
// Number of samples processed by the DSP for the current track since the last reset.
// (A reset is triggered when the user manually seeks into the track)
static DWORD processedSamplesForCurrentTrackSinceLastReset = 0;
// Internal threshold used to debounce the calls to TrackStartsPlaying()
static int msThresholdForNewTrack = 0;
// Expected number of ms to the end of the current track from the last reset.
// (Or from the beginning of the track if there has been no reset)
static int expectedEndOfCurrentTrackInMs = INT_MAX;
// Used as a sanity check the loop detection.
static double lastMeasuredTrackPosition = NAN;
// Total duration of the current track.
static int currentTrackDurationMs = 0;
// When set to true, the current track will be scrobbled as soon as it ends.
// Used to debounce the calls to OnTrackCompletes()
static bool callOnTrackCompletesOnEnd = false;

static XMPDSP dsp =
{
    XMPDSP_FLAG_NODSP,
    PLUGIN_FRIENDLY_NAME,
    DSP_About,
    DSP_New,
    DSP_Free,
    DSP_GetDescription,
    DSP_Config,
    DSP_GetConfig,
    DSP_SetConfig,
    DSP_NewTrack,
    DSP_SetFormat,
    DSP_Reset,
    DSP_Process,
    DSP_NewTitle
};

static PLUGIN_EXPORTS exports =
{
    ShowInfoBubble,
    GetPlaylist,
    FreePlaylist,
    GetPlaybackStatus,
    GetCurrentTrackInfo,
    TogglePlayPause,
    GetVolume,
    SetVolume,
    GetCurrentPlaylistPosition,
    SetCurrentPlaylisPosition,
    GetPlaybackTime,
    SetPlaybackTime,
};

static void WINAPI DSP_About(HWND win)
{
    // Native dialog allowing to download .Net
    DialogBox(hDll, MAKEINTRESOURCE(IDD_ABOUT), win, &AboutDialogProc);
}

static BOOL CALLBACK AboutDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hWnd, IDC_ABOUT_DOTNET_LINK, ABOUT_DIALOG_TEXT);
        break;
    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->idFrom == IDC_ABOUT_DOTNET_LINK)
        {
            // Check for click or return key.
            if ((pnmh->code == NM_CLICK) || (pnmh->code == NM_RETURN))
            {
                // Take the user to the .Net 4.6 offline installer download page.
                ShellExecute(NULL, "open", "http://www.microsoft.com/en-us/download/details.aspx?id=48137", NULL, NULL, SW_SHOWNORMAL);
            }
        }
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hWnd, 0);
            break;
        }
        break;
    }
    return FALSE;
}

static const char* WINAPI DSP_GetDescription(void* inst)
{
    return PLUGIN_FRIENDLY_NAME;
}

static void* WINAPI DSP_New()
{
    pluginWrapper = new SharpScrobblerWrapper();
    SharpScrobblerWrapper::InitializeExports(&exports);
    SharpScrobblerWrapper::LogInfo("****************************************************************************************************");
    SharpScrobblerWrapper::LogInfo(PLUGIN_FRIENDLY_NAME " " PLUGIN_VERSION_STRING " started!");

    if (!xmplayWinHandle)
        xmplayWinHandle = xmpfmisc->GetWindow();

    return (void*)1;
}

static void WINAPI DSP_Free(void* inst)
{
    FreeTrackInfo(currentTrackInfo);
    delete pluginWrapper;
}

// Called after a click on the plugin Config button.
static void WINAPI DSP_Config(void* inst, HWND win)
{
    //const char* sessionKey = pluginWrapper->AskUserForNewAuthorizedSessionKey(win);
    //if (sessionKey != NULL)
    //{
    //    // If the new session key is valid, save it.
    //    memcpy(pluginConfig.sessionKey, sessionKey, sizeof(pluginConfig.sessionKey));
    //    pluginWrapper->SetSessionKey(pluginConfig.sessionKey);
    //}
}

// Get config from the plugin. (return size of config data)
static DWORD WINAPI DSP_GetConfig(void* inst, void* config)
{
    memcpy(config, &pluginConfig, sizeof(ScrobblerConfig));
    return sizeof(ScrobblerConfig);
}

// Apply config to the plugin.
static BOOL WINAPI DSP_SetConfig(void* inst, void* config, DWORD size)
{
    memcpy(&pluginConfig, config, sizeof(ScrobblerConfig));
    //pluginWrapper->SetSessionKey(pluginConfig.sessionKey);
    return TRUE;
}

// Called when a track has been opened or closed.
// (file will be NULL in the latter case)
static void WINAPI DSP_NewTrack(void* inst, const char* file)
{
    currentFilePath = file;
    CompleteCurrentTrack();
}

// Called when a format is set (because of a new track for example)
// (if form is NULL, output stopped)
static void WINAPI DSP_SetFormat(void* inst, const XMPFORMAT* form)
{
    CompleteCurrentTrack();
    if (form != NULL)
        xmprateBy1000 = form->rate / 1000;
    else
        xmprateBy1000 = 0;
}

// This is apparently useless as I have never seen it called once.
static void WINAPI DSP_NewTitle(void* inst, const char* title) { }

// Called when the user seeks into the track.
static void WINAPI DSP_Reset(void* inst)
{
    double resetPosition = xmpfstatus->GetTime();
    expectedEndOfCurrentTrackInMs = GetExpectedEndOfCurrentTrackInMs((int)resetPosition * 1000);
    processedSamplesForCurrentTrackSinceLastReset = 0;
    // Reinitialize the last measured position to avoid an improbable bug in the case where
    // the user would seek *just* before the track looping.
    lastMeasuredTrackPosition = NAN;
}

// Strictly speaking, this plugin doesn't do any "processing", and always let the data untouched.
// Nonetheless, this function is useful to count every sample played and keep track of the play time,
// And also to detect tracks starting or looping, and act accordingly...
static DWORD WINAPI DSP_Process(void* inst, float* data, DWORD count)
{
    // Check whether the processed track is a track just starting to play or not:
    if (processedSamplesForCurrentTrack == 0) msThresholdForNewTrack = count / xmprateBy1000;
    int calculatedPlayedMs = processedSamplesForCurrentTrack / xmprateBy1000;
    if (calculatedPlayedMs < msThresholdForNewTrack)
    {
        // New track starts playing normally.
        TrackStartsPlaying();
    }
    else
    {
        // In the middle of a track. Before continuing, we check whether the track has looped without XMPlay telling us.
        int calculatedPlayedMsSinceLastReset = processedSamplesForCurrentTrackSinceLastReset / xmprateBy1000;
        // If the track duration is 0, this is a stream. It's useless to check for a loop in that case, so we skip the next part entirely.
        if (currentTrackDurationMs != 0 && calculatedPlayedMsSinceLastReset > expectedEndOfCurrentTrackInMs)
        {
            // The calculated play time based on the number of samples actually played
            // is superior to the length of the track, but XMPlay did not signal a new track.
            // This means the track is looping, so this is effectively a new play.
            // So we reset everything like if it were a new track:

            // BUT
            // before doing that, we check the actual position reported by XMPlay.
            // (Unlike the track duration, the position seems to be correctly synchronized to the sample currently being processed)
            // We do that because otherwise, even with a full second being added to expectedEndOfCurrentTrackInMs to account for errors, the loop detection is often off by a few samples.
            // (A loop is believed to have been detected, followed almost immediately by an actual new track being played)
            // This might be because of gap-less transitions between tracks, the impossibility to precisely detect the length of a track in some file formats, or this might simply be a bug...
            // Anyway, for now, I trust the XMPlay reported position more than the calculations based on the number of samples having been played.

            double trustWorthyPosition = xmpfstatus->GetTime();
            if (isnan(lastMeasuredTrackPosition))
            {
                // This is the first time we get here.
                lastMeasuredTrackPosition = trustWorthyPosition;
            }
            else
            {
                // We've already been here...
                if (trustWorthyPosition >= lastMeasuredTrackPosition)
                {
                    // ...and the position has increased, so we're not done yet playing the track.
                    lastMeasuredTrackPosition = trustWorthyPosition;
                    // Nothing to see here, this is not a looping yet...
                }
                else
                {
                    // ...and the position reset, this is an actual loop!
                    CompleteCurrentTrack();
                    msThresholdForNewTrack = 0; // So that we don't risk detecting the new play a second time based on the processed samples count.
                    TrackStartsPlaying();
                }
            }
        }
    }

    if (callOnTrackCompletesOnEnd == false)
    {
        callOnTrackCompletesOnEnd = true;
    }

    // Keep track of the time the track has played.
    processedSamplesForCurrentTrack += count;
    processedSamplesForCurrentTrackSinceLastReset += count;
    return count;
}

// Complete playing the current track.
// Reset everything for a new track.
static void CompleteCurrentTrack()
{
    if (callOnTrackCompletesOnEnd)
    {
        pluginWrapper->OnTrackCompletes();
        callOnTrackCompletesOnEnd = false;
    }

    processedSamplesForCurrentTrack = 0;
    processedSamplesForCurrentTrackSinceLastReset = 0;
    lastMeasuredTrackPosition = NAN;
    // Set the expected end of the new track to a temporary safe upper value.
    // (We might not have a track playing yet, so we can't calculate the correct value)
    expectedEndOfCurrentTrackInMs = INT_MAX;
}

// Called when a track starts playing.
// This differs from DSP_NewTrack() in that it correctly accounts for looped tracks.
// (That is, if a track loops, this function is called whereas DSP_NewTrack() is not)
static void TrackStartsPlaying()
{
    currentTrackDurationMs = expectedEndOfCurrentTrackInMs = GetExpectedEndOfCurrentTrackInMs(0);
    callOnTrackCompletesOnEnd = false;

    // (Re)initialize currentTrackInfo:
    FreeTrackInfo(currentTrackInfo);

    TRACK_INFO* trackInfo = new TRACK_INFO();
    trackInfo->title = GetTagW(TAG_TITLE);
    trackInfo->artist = GetTagW(TAG_ARTIST);
    trackInfo->album = GetTagW(TAG_ALBUM);
    // If the subsong tag is set, we use it instead of the track tag,
    // because inside a multi-track, the subsong number is more accurate.
    char* subsongNumber = xmpfmisc->GetTag(TAG_SUBSONG); // separated subsong (number/total)
    if (subsongNumber != NULL)
    {
        trackInfo->trackNumber = GetStringW(subsongNumber);
        xmpfmisc->Free(subsongNumber);
    }
    else
    {
        trackInfo->trackNumber = GetTagW(TAG_TRACK);
    }

    currentTrackInfo = trackInfo;

    pluginWrapper->OnTrackStartsPlaying(
        currentTrackInfo->artist,
        currentTrackInfo->title,
        currentTrackInfo->album,
        currentTrackDurationMs,
        currentTrackInfo->trackNumber,
        NULL);

    LPCWSTR wFilePath = GetStringW(currentFilePath);
    delete[] wFilePath;
}

// Calculate the expected time until the end of the current track from the desired position.
static int GetExpectedEndOfCurrentTrackInMs(int fromPositionMs)
{
    // FIXME: an accurate AND strongly typed duration would be nice...
    char* stringDuration = xmpfmisc->GetTag(TAG_LENGTH);
    double parsed = atof(stringDuration);
    xmpfmisc->Free(stringDuration);
    int currentTrackMaxExpectedDurationMs = (int)(parsed * 1000);
    return currentTrackMaxExpectedDurationMs - fromPositionMs;
}

// Free an instance of TrackInfo.
static void FreeTrackInfo(TRACK_INFO* trackInfo)
{
    if (trackInfo != NULL)
    {
        delete[] trackInfo->title;
        delete[] trackInfo->artist;
        delete[] trackInfo->album;
        delete[] trackInfo->trackNumber;
        delete trackInfo;
        trackInfo = NULL;
    }
}

// Get a wide string from an ansi string. (Don't forget to free it)
static LPWSTR GetStringW(const char* string)
{
    if (string != NULL)
    {
        size_t requiredSize = Utf2Uni(string, AUTO_NULL_TERMINATED_LENGTH, NULL, 0);
        LPWSTR buffer = new wchar_t[requiredSize];
        Utf2Uni(string, AUTO_NULL_TERMINATED_LENGTH, buffer, requiredSize);
        return buffer;
    }
    else return NULL;
}

#define Uni2Utf(src,slen,dst,dlen) WideCharToMultiByte(CP_UTF8,0,src,slen,dst,dlen, NULL, NULL)
// Get a char* string from an wide string. (Don't forget to free it)
static char* GetString(LPCWSTR string)
{
    if (string != NULL)
    {
        size_t requiredSize = Uni2Utf(string, AUTO_NULL_TERMINATED_LENGTH, NULL, 0);
        char* buffer = new char[requiredSize];
        Uni2Utf(string, AUTO_NULL_TERMINATED_LENGTH, buffer, requiredSize);
        return buffer;
    }
    else return NULL;
}

// Get an XMPlay tag as a wide string. (Don't forget to free it)
static LPWSTR GetTagW(const char* tag)
{
    char* value = xmpfmisc->GetTag(tag);
    LPWSTR wValue = NULL;
    if (value != NULL)
        wValue = GetStringW(value);
    xmpfmisc->Free(value);
    return wValue;
}

static std::wstring NullCheck(LPCWSTR string)
{
    return string ? std::wstring(string) : std::wstring();
}

static void WINAPI ShowInfoBubble(LPCWSTR text, int displayTimeMs)
{
    char* string = GetString(text);
    xmpfmisc->ShowBubble(string, displayTimeMs);
    delete string;
}

static void WINAPI GetPlaylist(PLAYLIST_ITEM** items, int* size)
{
    *size = SendMessage(xmplayWinHandle, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
    *items = new PLAYLIST_ITEM[*size];
    for (int i = 0; i < *size; i++)
    {
        char* file = (char*)SendMessage(xmplayWinHandle, WM_WA_IPC, i, IPC_GETPLAYLISTFILE);
        char* title = (char*)SendMessage(xmplayWinHandle, WM_WA_IPC, i, IPC_GETPLAYLISTTITLE);

        PLAYLIST_ITEM item = PLAYLIST_ITEM();
        item.filePath = GetStringW(file);
        item.title = GetStringW(title);

        (*items)[i] = item;
    }
}

static void WINAPI FreePlaylist(PLAYLIST_ITEM* items, int size)
{
    for (int i = 0; i < size; i++)
    {
        PLAYLIST_ITEM item = items[i];
        delete[] item.filePath;
        delete[] item.title;
    }
    delete[] items;
}

static PLAYBACK_STATUS WINAPI GetPlaybackStatus()
{
    PLAYBACK_STATUS status = (PLAYBACK_STATUS)SendMessage(xmplayWinHandle, WM_WA_IPC, 0, IPC_ISPLAYING);
    return status;
}

static void WINAPI GetCurrentTrackInfo(TRACK_INFO* localCurrentTrackInfo)
{
    localCurrentTrackInfo->title = currentTrackInfo->title;
    localCurrentTrackInfo->album = currentTrackInfo->album;
    localCurrentTrackInfo->artist = currentTrackInfo->artist;
    localCurrentTrackInfo->trackNumber = currentTrackInfo->trackNumber;
}

static void WINAPI TogglePlayPause()
{
    //ShowWindow(xmplayWinHandle, SW_RESTORE);
    //SetFocus(xmplayWinHandle);
    //SetActiveWindow(xmplayWinHandle);
    SendMessage(xmplayWinHandle, WM_KEYDOWN, 0x50, 0); // 'P' key
    SendMessage(xmplayWinHandle, WM_KEYUP, 0x50, 0); // 'P' key
    //ShowWindow(xmplayWinHandle, SW_SHOWNOACTIVATE);
}

// (between the range of 0 to 255)
static double WINAPI GetVolume()
{
    int volume = SendMessage(xmplayWinHandle, WM_WA_IPC, -666, IPC_SETVOLUME);
    return volume / 255.0;
}

static void WINAPI SetVolume(double volume)
{
    int intVolume = volume * 255;
    if (intVolume > 255)
        intVolume = 255;
    if (intVolume < 0)
        intVolume = 0;
    SendMessage(xmplayWinHandle, WM_WA_IPC, intVolume, IPC_SETVOLUME);
}

static int WINAPI GetCurrentPlaylistPosition()
{
    int position = SendMessage(xmplayWinHandle, WM_WA_IPC, 0, IPC_GETLISTPOS);
    return position;
}

static void WINAPI SetCurrentPlaylisPosition(int index)
{
    SendMessage(xmplayWinHandle, WM_WA_IPC, index, IPC_SETPLAYLISTPOS);
    SendMessage(xmplayWinHandle, WM_USER + 26, 372/*XMP_LISTPLAY*/, 0);
}

static void WINAPI GetPlaybackTime(int* currentTimeMs, int* totalTimeMs)
{
    *currentTimeMs = SendMessage(xmplayWinHandle, WM_WA_IPC, 0, IPC_GETOUTPUTTIME);
    //int totalSECONDS = SendMessage(xmplayWinHandle, WM_WA_IPC, 1, IPC_GETOUTPUTTIME);
    *totalTimeMs = currentTrackDurationMs;
}

static void WINAPI SetPlaybackTime(int currentTimeMs)
{
    // We use the timeout version, because, I'm not sure why, but sometimes this call seems to deadlock XMPlay.
    // For example, this occurs when this method is called from TrackStartsPlaying and the playback time is still 0.
    // Setting a timeout resolves the problem...
    SendMessageTimeout(xmplayWinHandle, WM_WA_IPC, currentTimeMs, IPC_JUMPTOTIME, SMTO_ABORTIFHUNG, 300 /*ms*/, NULL);
}

// Get the plugin's XMPDSP interface.
XMPDSP* WINAPI XMPDSP_GetInterface2(DWORD face, InterfaceProc faceproc)
{
    if (face != XMPDSP_FACE) return NULL;
    xmpfmisc = (XMPFUNC_MISC*)faceproc(XMPFUNC_MISC_FACE); // Import misc functions.
    xmpfstatus = (XMPFUNC_STATUS*)faceproc(XMPFUNC_STATUS_FACE); // Import playback status functions.
    return &dsp;
}

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hDLL);
        hDll = hDLL;
    }
    return 1;
}
