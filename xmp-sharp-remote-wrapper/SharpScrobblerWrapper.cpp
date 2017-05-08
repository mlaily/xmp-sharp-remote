#include "Stdafx.h"
#include <windows.h>
#include "Definitions.h"
#include <msclr\auto_gcroot.h>

#using "xmp-sharp-remote-managed.dll"
#using "System.dll"

using namespace System;
using namespace Runtime::InteropServices;
using namespace xmp_sharp_remote_managed;

// Create a managed proxy function for a native function pointer
// to allow the C# part to call a native function:

class SharpScrobblerAdapter
{
public:
    msclr::auto_gcroot<SharpScrobbler^> Instance;
    SharpScrobblerAdapter() : Instance(gcnew SharpScrobbler()) {}
};

class __declspec(dllexport) SharpScrobblerWrapper
{
private:
    SharpScrobblerAdapter* _adapter;

public:
    SharpScrobblerWrapper()
    {
        _adapter = new SharpScrobblerAdapter();
    }
    ~SharpScrobblerWrapper()
    {
        delete _adapter;
    }

    static void InitializeExports(PLUGIN_EXPORTS* pluginExports)
    {
        xmp_sharp_remote_managed::PluginExports^ managedExports = gcnew xmp_sharp_remote_managed::PluginExports();
        managedExports->ShowBubbleInfo =
            Marshal::GetDelegateForFunctionPointer<ShowInfoBubbleHandler^>((IntPtr)pluginExports->ShowBubbleInfo);
        managedExports->GetPlaylist =
            Marshal::GetDelegateForFunctionPointer<GetPlaylistHandler^>((IntPtr)pluginExports->GetPlaylist);
        managedExports->FreePlaylist =
            Marshal::GetDelegateForFunctionPointer<FreePlaylistHandler^>((IntPtr)pluginExports->FreePlaylist);
        managedExports->GetPlaybackStatus =
            Marshal::GetDelegateForFunctionPointer<GetPlaybackStatusHandler^>((IntPtr)pluginExports->GetPlaybackStatus);
        managedExports->GetCurrentTrackInfo =
            Marshal::GetDelegateForFunctionPointer<GetCurrentTrackInfoHandler^>((IntPtr)pluginExports->GetCurrentTrackInfo);
        managedExports->TogglePlayPause =
            Marshal::GetDelegateForFunctionPointer<TogglePlayPauseHandler^>((IntPtr)pluginExports->TogglePlayPause);
        managedExports->GetVolume =
            Marshal::GetDelegateForFunctionPointer<GetVolumeHandler^>((IntPtr)pluginExports->GetVolume);
        managedExports->SetVolume =
            Marshal::GetDelegateForFunctionPointer<SetVolumeHandler^>((IntPtr)pluginExports->SetVolume);
        managedExports->GetCurrentPlaylistPosition =
            Marshal::GetDelegateForFunctionPointer<GetCurrentPlaylistPositionHandler^>((IntPtr)pluginExports->GetCurrentPlaylistPosition);
        managedExports->SetCurrentPlaylisPosition =
            Marshal::GetDelegateForFunctionPointer<SetCurrentPlaylisPositionHandler^>((IntPtr)pluginExports->SetCurrentPlaylisPosition);
        managedExports->GetPlaybackTime =
            Marshal::GetDelegateForFunctionPointer<GetPlaybackTimeHandler^>((IntPtr)pluginExports->GetPlaybackTime);
        managedExports->SetPlaybackTime =
            Marshal::GetDelegateForFunctionPointer<SetPlaybackTimeHandler^>((IntPtr)pluginExports->SetPlaybackTime);

        Util::InitializeExports(managedExports);
    }

    static void LogInfo(const char* message)
    {
        Logger::Log(LogLevel::Info, gcnew String(message));
    }
    static void LogInfo(const wchar_t* message)
    {
        Logger::Log(LogLevel::Info, gcnew String(message));
    }

    static void LogWarning(const char* message)
    {
        Logger::Log(LogLevel::Warn, gcnew String(message));
    }
    static void LogWarning(const wchar_t* message)
    {
        Logger::Log(LogLevel::Warn, gcnew String(message));
    }

    //void SetSessionKey(const char* sessionKey)
    //{
    //    _adapter->Instance->SessionKey = gcnew String(sessionKey);
    //}

    void OnTrackStartsPlaying(const wchar_t* artist, const wchar_t* track, const wchar_t* album, int durationMs, const wchar_t* trackNumber, const char* mbid)
    {
        _adapter->Instance->OnTrackStartsPlaying(gcnew String(artist), gcnew String(track), gcnew String(album), durationMs, gcnew String(trackNumber), gcnew String(mbid));
    }

    //void OnTrackCanScrobble(const wchar_t* artist, const wchar_t* track, const wchar_t* album, int durationMs, const wchar_t* trackNumber, const char* mbid, time_t utcUnixTimestamp)
    //{
    //    _adapter->Instance->OnTrackCanScrobble(gcnew String(artist), gcnew String(track), gcnew String(album), durationMs, gcnew String(trackNumber), gcnew String(mbid), utcUnixTimestamp);
    //}

    void OnTrackCompletes()
    {
        _adapter->Instance->OnTrackCompletes();
    }

    //const char* AskUserForNewAuthorizedSessionKey(HWND ownerWindowHandle)
    //{
    //    String^ managedResult = _adapter->Instance->AskUserForNewAuthorizedSessionKey(IntPtr(ownerWindowHandle));
    //    return (const char*)Marshal::StringToHGlobalAnsi(managedResult).ToPointer();
    //}
};