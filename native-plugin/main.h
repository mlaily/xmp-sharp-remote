#pragma once

#include "time.h"

#define PLUGIN_FRIENDLY_NAME    "XMPlay Sharp Remote"
#define PLUGIN_VERSION          0,4,0,0
#define PLUGIN_VERSION_STRING   "0.4.0.0"
#define IDD_ABOUT               1001
#define IDC_ABOUT_DOTNET_LINK   1002
#define ABOUT_DIALOG_TEXT PLUGIN_FRIENDLY_NAME "\n\nA Last.fm scrobbling plugin for XMPlay.\n\nVersion " PLUGIN_VERSION_STRING \
" - 2017\n\nBy Melvyn Laïly\n\nThis plugin requires the .Net Framework 4.6 to be installed to run.\n\n<a>Download .Net 4.6</a>"


// Config structure, as stored by XMPlay.
typedef struct
{
    char sessionKey[32];
} ScrobblerConfig;

/* DSP functions: */

static void WINAPI DSP_About(HWND win);
static void* WINAPI DSP_New(void);
static void WINAPI DSP_Free(void* inst);
static const char* WINAPI DSP_GetDescription(void* inst);
static void WINAPI DSP_Config(void* inst, HWND win);
static DWORD WINAPI DSP_GetConfig(void* inst, void* config);
static BOOL WINAPI DSP_SetConfig(void* inst, void* config, DWORD size);
static void WINAPI DSP_NewTrack(void* inst, const char* file);
static void WINAPI DSP_SetFormat(void* inst, const XMPFORMAT* form);
static void WINAPI DSP_Reset(void* inst);
static DWORD WINAPI DSP_Process(void* inst, float* data, DWORD count);
static void WINAPI DSP_NewTitle(void* inst, const char* title);

/* Plugin functions: */

static void CompleteCurrentTrack();
static void TrackStartsPlaying();
static void ReleaseTrackInfo(TRACK_INFO* trackInfo);
static int GetExpectedEndOfCurrentTrackInMs(int fromPositionMs);
static LPWSTR GetStringW(const char* string);
static LPWSTR GetTagW(const char* tag);
static std::wstring NullCheck(LPCWSTR* string);

static BOOL CALLBACK AboutDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Exported functions: */
static void WINAPI ShowInfoBubble(LPCWSTR text, int displayTimeMs);
static void WINAPI GetPlaylist(PLAYLIST_ITEM** items, int* size);
static PLAYBACK_STATUS WINAPI GetPlaybackStatus();
static void WINAPI GetCurrentTrackInfo(TRACK_INFO* currentTrackInfo);
static void WINAPI TogglePlayPause();
static double WINAPI GetVolume();
static void WINAPI SetVolume(double volume);
static int WINAPI GetCurrentPlaylistPosition();
static void WINAPI SetCurrentPlaylisPosition(int index);
static void WINAPI GetPlaybackTime(int* currentTimeMs, int* totalTimeMs);
static void WINAPI SetPlaybackTime(int currentTimeMs);
