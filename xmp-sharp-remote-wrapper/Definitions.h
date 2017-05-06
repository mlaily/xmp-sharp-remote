#pragma once

#include <windows.h>

typedef struct
{
    LPCWSTR title;
    LPCWSTR filePath;
} PLAYLIST_ITEM;

typedef struct { // miscellaneous functions
                 //DWORD(WINAPI *GetVersion)(); // get XMPlay version (eg. 0x03040001 = 3.4.0.1)
                 //HWND(WINAPI *GetWindow)(); // get XMPlay window handle
                 //void *(WINAPI *Alloc)(DWORD len); // allocate memory
                 //void *(WINAPI *ReAlloc)(void *mem, DWORD len); // re-allocate memory
                 //void (WINAPI *Free)(void *mem); // free allocated memory/text
                 //BOOL(WINAPI *CheckCancel)(); // user wants to cancel?
                 //DWORD(WINAPI *GetConfig)(DWORD option); // get a config (XMPCONFIG_xxx) value
                 //const char *(WINAPI *GetSkinConfig)(const char *name); // get a skinconfig value
                 //void (WINAPI *ShowBubble)(const char *text, DWORD time); // show a help bubble (time in ms, 0=default)
                 //void (WINAPI *RefreshInfo)(DWORD mode); // refresh info displays (XMPINFO_REFRESH_xxx flags)
                 //char *(WINAPI *GetInfoText)(DWORD mode); // get info window text (XMPINFO_TEXT_xxx)
                 //char *(WINAPI *FormatInfoText)(char *buf, const char *name, const char *value); // format text for info window (tabs & new-lines)
                 //char *(WINAPI *GetTag)(const char *tag); // get a current track's tag (tag name or TAG_xxx)
                 //BOOL(WINAPI *RegisterShortcut)(const XMPSHORTCUT *cut); // add a shortcut
                 //BOOL(WINAPI *PerformShortcut)(DWORD id); // perform a shortcut action
                 //                                         // version 3.4.0.14
                 //const XMPCUE *(WINAPI *GetCue)(DWORD cue); // get a cue entry (0=image, 1=1st track)
                 //                                           // version 3.8
                 //BOOL(WINAPI *DDE)(const char *command); // execute a DDE command without using DDE
    void(WINAPI *ShowBubbleInfo)(const char* text, int displayTimeMs);
    LPCWSTR(WINAPI *GetPlaylist)();
    void(WINAPI *GetPlaylist2)(PLAYLIST_ITEM** items, int* size);
} PLUGIN_EXPORTS;