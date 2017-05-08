#pragma once

#include <windows.h>

typedef struct
{
    LPCWSTR title;
    LPCWSTR filePath;
} PLAYLIST_ITEM;

typedef struct
{
    LPCWSTR title;
    LPCWSTR artist;
    LPCWSTR album;
    LPCWSTR trackNumber;
} TRACK_INFO;

enum PLAYBACK_STATUS
{
    Stopped = 0,
    Playing = 1,
    Paused = 3,
};

typedef struct 
{
    void(WINAPI *ShowBubbleInfo)(LPCWSTR text, int displayTimeMs);
    void(WINAPI *GetPlaylist)(PLAYLIST_ITEM** items, int* size);
    void(WINAPI *FreePlaylist)(PLAYLIST_ITEM* items, int size);
    PLAYBACK_STATUS(WINAPI *GetPlaybackStatus)();
    void(WINAPI *GetCurrentTrackInfo)(TRACK_INFO* currentTrackInfo);
    void(WINAPI *TogglePlayPause)();
    double(WINAPI *GetVolume)();
    void(WINAPI *SetVolume)(double volume);
    int(WINAPI *GetCurrentPlaylistPosition)();
    void(WINAPI *SetCurrentPlaylisPosition)(int index);
    void(WINAPI *GetPlaybackTime)(int* currentTimeMs, int* totalTimeMs);
    void(WINAPI *SetPlaybackTime)(int currentTimeMs);
} PLUGIN_EXPORTS;