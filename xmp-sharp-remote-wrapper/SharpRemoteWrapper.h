#include "Definitions.h"

class SharpRemoteAdapter;

class SharpRemoteWrapper
{
private:
    SharpRemoteAdapter* _adapter;

public:
    SharpRemoteWrapper();
    ~SharpRemoteWrapper();

    static void InitializeExports(PLUGIN_EXPORTS* pluginExports);

    static void LogInfo(const char* message);
    static void LogInfo(const wchar_t* message);

    static void LogWarning(const char* message);
    static void LogWarning(const wchar_t* message);

    //void SetSessionKey(const char* sessionKey);

    void OnTrackStartsPlaying(const TRACK_INFO* trackInfo);
    //void OnTrackCanScrobble(const wchar_t* artist, const wchar_t* track, const wchar_t* album, int durationMs, const wchar_t* trackNumber, const char* mbid, time_t utcUnixTimestamp);
    void OnTrackCompletes();

    //const char* AskUserForNewAuthorizedSessionKey(HWND ownerWindowHandle);
};
