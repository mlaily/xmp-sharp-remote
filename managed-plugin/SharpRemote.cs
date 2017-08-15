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
using MoreLinq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace xmp_sharp_remote_managed
{
    public class SharpRemote
    {
        private static readonly TimeSpan DefaultErrorBubbleDisplayTime = TimeSpan.FromSeconds(5);

        private Server _server;

        //public string SessionKey { get; set; }

        public SharpRemote()
        {
            _server = new Server();
            _server.Start();
        }

        //public string AskUserForNewAuthorizedSessionKey(IntPtr ownerWindowHandle)
        //{
        //    Configuration configurationForm = new Configuration();
        //    if (configurationForm.ShowDialog(new Win32Window(ownerWindowHandle)) == DialogResult.OK)
        //    {
        //        // refresh with the new session key
        //        SessionKey = configurationForm.SessionKey;
        //    }
        //    return SessionKey;
        //}

        public void OnTrackStartsPlaying(TrackInfo trackInfo)
        {
            // var current = NativeWrapper.GetCurrentTrackInfo();
            //var playlist = NativeWrapper.GetPlaylist();
        }

        public void OnTrackCompletes()
        {
        }

        //private async Task ShowBubbleOnErrorAsync<T>(Task<ApiResponse<T>> request)
        //{
        //    try
        //    {
        //        var response = await request;
        //        if (!response.Success)
        //        {
        //            ShowErrorBubble(response.Error.Message);
        //            // TODO: log
        //        }
        //    }
        //    catch (Exception ex)
        //    {
        //        ShowErrorBubble(ex);
        //        // TODO: log
        //    }
        //}

        private static void ShowErrorBubble(string message)
        {
            NativeWrapper.ShowInfoBubble($"Scrobbler Error! {message}", DefaultErrorBubbleDisplayTime);
        }
        private static void ShowErrorBubble(Exception ex)
        {
            NativeWrapper.ShowInfoBubble($"Scrobbler Error! {ex?.GetType()?.Name + " - " ?? ""}{ex.Message}", DefaultErrorBubbleDisplayTime);
        }
    }
}