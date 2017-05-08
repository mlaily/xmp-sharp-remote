// Copyright(c) 2015 Melvyn Laïly
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
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace xmp_sharp_remote_managed
{
    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public delegate void ShowInfoBubbleHandler(string text, int displayTimeMs);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public delegate void GetPlaylistHandler(out IntPtr items, out int size);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public delegate void FreePlaylistHandler(IntPtr items, int size);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public delegate PlaybackStatus GetPlaybackStatusHandler();

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public delegate void GetCurrentTrackInfoHandler(out TrackInfo currentTrackInfo);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public delegate void TogglePlayPauseHandler();

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public delegate double GetVolumeHandler();

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public delegate void SetVolumeHandler(double volume);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public delegate int GetCurrentPlaylistPositionHandler();

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public delegate void SetCurrentPlaylisPositionHandler(int index);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public delegate void GetPlaybackTimeHandler(out int currentTimeMs, out int totalTimeMs);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
    public delegate void SetPlaybackTimeHandler(int currentTimeMs);

    public enum PlaybackStatus
    {
        Stopped = 0,
        Playing = 1,
        Paused = 3,
    }

    [StructLayout(LayoutKind.Sequential)]
    public class PluginExports
    {
        public ShowInfoBubbleHandler ShowBubbleInfo;
        public GetPlaylistHandler GetPlaylist;
        public FreePlaylistHandler FreePlaylist;
        public GetPlaybackStatusHandler GetPlaybackStatus;
        public GetCurrentTrackInfoHandler GetCurrentTrackInfo;
        public TogglePlayPauseHandler TogglePlayPause;
        public GetVolumeHandler GetVolume;
        public SetVolumeHandler SetVolume;
        public GetCurrentPlaylistPositionHandler GetCurrentPlaylistPosition;
        public SetCurrentPlaylisPositionHandler SetCurrentPlaylisPosition;
        public GetPlaybackTimeHandler GetPlaybackTime;
        public SetPlaybackTimeHandler SetPlaybackTime;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public class TrackInfo
    {
        public string title;
        public string artist;
        public string album;
        public string trackNumber;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public class PlaylistItem
    {
        public string title;
        public string filePath;
    }

    public static class NativeWrapper
    {
        private static PluginExports _pluginExports;

        public static void InitializeExports(PluginExports exports)
        {
            _pluginExports = exports;
        }

        public static void ShowInfoBubble(string text, TimeSpan? displayTime = null)
            => _pluginExports.ShowBubbleInfo?.Invoke(text, displayTime == null ? 0 : (int)displayTime.Value.TotalMilliseconds);

        public static PlaylistItem[] GetPlaylist()
        {
            // Marshaling reference:
            // managed: https://msdn.microsoft.com/en-us/library/eshywdt7(v=vs.110).aspx
            // native: https://msdn.microsoft.com/en-us/library/as6wyhwt(v=vs.100).aspx
            _pluginExports.GetPlaylist(out var itemsPtr, out int size);
            PlaylistItem[] playlist = new PlaylistItem[size];
            IntPtr current = itemsPtr;
            for (int i = 0; i < size; i++)
            {
                playlist[i] = Marshal.PtrToStructure<PlaylistItem>(current);
                current = (IntPtr)((long)current + Marshal.SizeOf<PlaylistItem>());
            }
            _pluginExports.FreePlaylist(itemsPtr, size);
            return playlist;
        }

        public static PlaybackStatus GetPlaybackStatus() => _pluginExports.GetPlaybackStatus();

        public static TrackInfo GetCurrentTrackInfo()
        {
            _pluginExports.GetCurrentTrackInfo(out var currentTrackInfo);
            return currentTrackInfo;
        }

        public static void TogglePlayPause() => _pluginExports.TogglePlayPause();

        public static double GetVolume() => _pluginExports.GetVolume();

        public static void SetVolume(double volume) => _pluginExports.SetVolume(volume);

        public static void GetCurrentPlaylistPosition() => _pluginExports.GetCurrentPlaylistPosition();

        public static void SetCurrentPlaylisPosition(int index) => _pluginExports.SetCurrentPlaylisPosition(index);

        public static void GetPlaybackTime(out TimeSpan currentTime, out TimeSpan totalTime)
        {
            _pluginExports.GetPlaybackTime(out int currentTimeMs, out int totalTimeMs);
            currentTime = TimeSpan.FromMilliseconds(currentTimeMs);
            totalTime = TimeSpan.FromMilliseconds(totalTimeMs);
        }

        public static void SetPlaybackTime(TimeSpan time) => _pluginExports.SetPlaybackTime((int)time.TotalMilliseconds);
    }
}
