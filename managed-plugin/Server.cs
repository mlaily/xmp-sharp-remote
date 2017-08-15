using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Web;
using xmp_sharp_remote_managed.Properties;

namespace xmp_sharp_remote_managed
{
    public class Server
    {
        private HttpListener _listener;
        private Dictionary<Func<HttpListenerContext, bool>, Action<HttpListenerContext>> _routesMap = new Dictionary<Func<HttpListenerContext, bool>, Action<HttpListenerContext>>();
        private const int ActionResponseDelay = 100;

        public Server()
        {
            // GET
            _routesMap.Add(x => Method(x, "GET") && Path(x) == "requests/track", x =>
            {
                var status = NativeWrapper.GetPlaybackStatus();
                var track = NativeWrapper.GetCurrentTrackInfo();
                var volume = NativeWrapper.GetVolume();
                NativeWrapper.GetPlaybackTime(out var current, out var total);

                var templatedResponse = FormatTemplate(Resources.currentTrackHtmlTemplate, new
                {
                    track.title,
                    track.artist,
                    track.album,
                    track.trackNumber,
                    Volume = $"{volume * 100:0}",
                    PlaybackTime = $"{current:hh\\:mm\\:ss\\.f}/{total:hh\\:mm\\:ss\\.f}",
                    PlaybackStatus = status
                });

                WriteResponse(x.Response, templatedResponse);
            });
            _routesMap.Add(x => Method(x, "GET") && Path(x) == "requests/playlist", x => WriteResponse(x.Response, FormatPlaylist(NativeWrapper.GetPlaylist(), NativeWrapper.GetCurrentPlaylistPosition())));

            // POST 
            _routesMap.Add(x => Method(x, "POST") && Path(x) == "requests/togglePlayPause", x =>
            {
                NativeWrapper.TogglePlayPause();
                Thread.Sleep(ActionResponseDelay);
                x.Response.Redirect("/requests/track");
            });
            _routesMap.Add(x => Method(x, "POST") && Path(x) == "requests/volume", x =>
            {
                var stringVolume = ParsePostParameters(x).Get("volume");
                var volume = int.TryParse(stringVolume, out var intValue) ? intValue : (int?)null;
                if (volume != null)
                {
                    NativeWrapper.SetVolume(Math.Min(1, Math.Max(0, volume.Value / 100f)));
                }
                x.Response.Redirect("/requests/track");
            });
            _routesMap.Add(x => Method(x, "POST") && Path(x) == "requests/track/time", x =>
            {
                var parameters = ParsePostParameters(x);
                if (parameters["skipBackward"] != null)
                {
                    NativeWrapper.GetPlaybackTime(out var current, out var total);
                    var newTime = current - new TimeSpan(0, 0, 5);
                    if (newTime.TotalMilliseconds < 0)
                    {
                        newTime = new TimeSpan(0);
                    }
                    NativeWrapper.SetPlaybackTime(newTime);
                }
                else if (parameters["skipForward"] != null)
                {
                    NativeWrapper.GetPlaybackTime(out var current, out var total);
                    var newTime = current + new TimeSpan(0, 0, 5);
                    if (newTime > total)
                    {
                        newTime = total;
                    }
                    NativeWrapper.SetPlaybackTime(newTime);
                }
                x.Response.Redirect("/requests/track");
            });
            _routesMap.Add(x => Method(x, "POST") && Path(x) == "requests/playlist/position", x =>
            {
                var stringPosition = ParsePostParameters(x).Get("position");
                var position = int.TryParse(stringPosition, out var intValue) ? intValue : (int?)null;
                if (position != null)
                {
                    NativeWrapper.SetCurrentPlaylisPosition(position.Value);
                }
                Thread.Sleep(ActionResponseDelay);
                x.Response.Redirect("/requests/playlist");
            });

            _routesMap.Add(x => Path(x) == "requests", x => WriteResponse(x.Response, Resources.requestsHtml));

            _routesMap.Add(x => Path(x) == "favicon.ico", x => WriteResponse(x.Response, Resources.favicon));

            // Catch all
            _routesMap.Add(_ => true, x => x.Response.Redirect("/requests"));
        }

        private static string Path(HttpListenerContext context) => context.Request.Url.GetComponents(UriComponents.Path, UriFormat.SafeUnescaped).TrimEnd(new[] { '/' });
        private static bool Method(HttpListenerContext context, string method) => context.Request.HttpMethod == method;

        private static string FormatPlaylist(IEnumerable<PlaylistItem> playlist, int currentPosition)
        {
            var itemTemplate = Encoding.UTF8.GetString(Resources.playlistItemHtmlTemplate);
            var templatedItems = string.Join("<br/>", playlist.Select((x, i) => FormatTemplate(itemTemplate, new { x.title, x.filePath, Position = i, IsCurrentPosition = currentPosition == i ? "==>" : "" })));
            var templatedPlaylist = FormatTemplate(Resources.playlistHtmlTemplate, new { Items = templatedItems });
            return templatedPlaylist;
        }

        private static NameValueCollection ParsePostParameters(HttpListenerContext context)
        {
            string body;
            using (var reader = new StreamReader(context.Request.InputStream, Encoding.UTF8))
            {
                body = reader.ReadToEnd();
            }
            return HttpUtility.ParseQueryString(body);
        }

        private static string FormatTemplate(byte[] template, object templateValues) => FormatTemplate(Encoding.UTF8.GetString(template), templateValues);

        private static string FormatTemplate(string template, object templateValues)
        {
            var allowedMemberTypes = new[] { MemberTypes.Field, MemberTypes.Property };

            object GetMemberValue(MemberInfo member, object instance)
            {
                switch (member)
                {
                    case FieldInfo field: return field.GetValue(instance);
                    case PropertyInfo property: return property.GetValue(instance);
                    default: return "ERROR";
                }
            }

            var namedValues = (from m in templateValues.GetType().GetMembers()
                               where allowedMemberTypes.Contains(m.MemberType)
                               let value = GetMemberValue(m, templateValues)
                               select new { m.Name, value })
                               .ToDictionary(x => x.Name, x => x.value);

            var result = Regex.Replace(template, @"\{(?<value>[a-zA-z0-9_]+)\}", x =>
            {
                var key = x.Groups["value"].Value;
                if (namedValues.ContainsKey(key))
                {
                    return namedValues[key]?.ToString() ?? "N/A";
                }
                else
                {
                    return "ERROR";
                }
            });
            return result;
        }

        public void Start()
        {
            int port = 9000;
            var url = $"http://+:{port}/";
            bool retry;
            do
            {
                try
                {
                    _listener = new HttpListener() { IgnoreWriteExceptions = true };
                    _listener.Prefixes.Add(url);
                    _listener.Start();
                    retry = false;
                }
                catch (HttpListenerException ex) when (ex.ErrorCode == 5) // Access denied
                {
                    {
                        var info = new ProcessStartInfo("netsh", $"http add urlacl url={url} user={Environment.UserName}") { Verb = "runas" };
                        var process = Process.Start(info);
                        retry = process.WaitForExit(3000) && process.ExitCode == 0;
                        if (process.HasExited && process.ExitCode != 0)
                        {
                            throw new Exception("Unable to register the required http prefix!");
                        }
                    }
                    {
                        var info = new ProcessStartInfo("netsh", $"advfirewall firewall add rule name=\"XMPlay Remote\" dir=in action=allow protocol=TCP localport={port} profile=private") { Verb = "runas" };
                        var process = Process.Start(info);
                        retry = process.WaitForExit(3000) && process.ExitCode == 0;
                    }
                }
                catch (Exception)
                {
                    retry = false;
                    throw;
                }
            } while (retry);


            Thread t = new Thread(ThreadLoop) { Name = "Server Thread" };
            t.Start();
        }

        private void ThreadLoop()
        {
            while (_listener.IsListening)
            {
                try
                {
                    var context = _listener.GetContext();

                    foreach (var route in _routesMap)
                    {
                        if (route.Key(context))
                        {
                            route.Value(context);
                            context.Response.Close();
                            break;
                        }
                    }
                }
                catch
                {
#if DEBUG
                    Debugger.Break();
#endif
                }
            }
        }

        private void WriteResponse(HttpListenerResponse response, string body)
        {
            var buffer = Encoding.UTF8.GetBytes($"{body}\r\n");
            response.ContentLength64 = buffer.Length;
            response.OutputStream.Write(buffer, 0, buffer.Length);
            response.OutputStream.Close();
        }

        private void WriteResponse(HttpListenerResponse response, byte[] body)
        {
            response.ContentLength64 = body.Length;
            response.OutputStream.Write(body, 0, body.Length);
            response.OutputStream.Close();
        }

        public void Exit()
        {
            _listener.Close();
        }
    }
}
