using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.IO;
using System.Diagnostics;
using System.Threading;

namespace HttpMonitor
{
    internal class Log
	{
		public Log(string url)
		{
			this.url = url;
		}

		public HttpWebRequest CreateWebRequest(string urlString)
		{
		    var request = WebRequest.Create(urlString) as HttpWebRequest;
			request.Timeout = Settings.HttpTimeout;
			request.ProtocolVersion = HttpVersion.Version11;
			return request;
		}


		public bool IsRangeAvailable()
		{
			var request = CreateWebRequest(url);
			//request.Accept = "image/*";
			request.AddRange(0,1);
		    var response = request.GetResponse() as HttpWebResponse;

		    if (!Settings.Verbose) return (response.StatusCode == HttpStatusCode.PartialContent);
		    Console.WriteLine("StatusCode: " + response.StatusCode + " (" + (int)response.StatusCode + ")");

		    foreach (var headerkey in response.Headers.AllKeys)
		    {
		        Console.WriteLine(headerkey + " : " + response.Headers[headerkey]);
		    }
		    return (response.StatusCode == HttpStatusCode.PartialContent);
		}

		public void TraceDiff(List<string> lastLines, List<string> lines)
		{
			if (lines.Count > lastLines.Count)
			{
				for (int i = lastLines.Count; i < lines.Count; i++)
				{
					Trace.Write(lines[i]);
				}
			}
			else
			{
				foreach (var line in lines)
				{
					Trace.Write(line);
				}
			}
		}

		public void Monitor()
		{
			if (IsRangeAvailable())
			{
				Util.DebugWrite("HTTP/1.1 section 14.35.1 Byte Ranges available");
			}
			else
			{
				Util.DebugWrite("HTTP/1.1 section 14.35.1 Byte Ranges NOT available");
			}

			var lastLines = Get();
			foreach (var line in Get())
				Trace.Write(line);
			while (true)
			{
				Thread.Sleep(Settings.PollInterval);
				var lines = Get();
				if (lines.Count != lastLines.Count)
				{
					TraceDiff(lastLines, lines);
				}
				lastLines = lines;
			}
		}

		public List<string> Get()
		{
			List<string> result = new List<string>();
			var request = CreateWebRequest(url);

			try
			{
				using (WebResponse response = (HttpWebResponse)request.GetResponse())
				{

					var encoding = ASCIIEncoding.ASCII;
					using (var reader = new System.IO.StreamReader(response.GetResponseStream(), encoding))
					{
						while (!reader.EndOfStream)
						{
							result.Add(reader.ReadLine());
						}
					}
				}
			}
			catch (WebException e)
			{
				if (Settings.Verbose)
				{
					Util.DebugWrite(e.ToString());
				}
				else
				{
					Util.DebugWrite(e.Message);
				}
				Trace.WriteLine("--- HttpMonitor of '" + url + "' stopped ---");
				throw;
			}
			return result;
		}

		private string url;
	}

	static class Util
	{
		public static void DebugWrite(string msg)
		{
			Console.WriteLine(msg);
			Trace.WriteLine(msg);
		}
	}

	static class Settings
	{
		public static bool Verbose;
		public static int PollInterval = 5000;
		public static int HttpTimeout = 5000;
	}

	class DebugViewPlugin
	{
		public static int IsIdentified(string argument)
		{
			if (string.IsNullOrEmpty(argument)) return -1;
			if (argument.ToLower().StartsWith("http")) return 1;
			return -1;
		}

		public static void ProcessCommandline(string[] args)
		{
			if (args.Length < 1) return;

			string command = args[0].ToLower();
			List<string> list = new List<string>(args);
			list.RemoveAt(0);
			string argument = string.Join(" ", list.ToArray());

			if (command.Contains("--info"))
			{
				// multiple 'input type' entries are allowed
				// input type: .dblog				// for .dblog files
				// input type: URL					// for any URL 
				// input type: URL-http				// for urls starting with http
				Console.WriteLine("input type: http");
				Console.WriteLine("input type: https");
				Console.WriteLine("outputdebugstring: yes");
				System.Environment.Exit(0);
			}
			else if (command.Contains("--identify"))
			{
				// exit code 1 means we recognize the specified file, otherwise exit code -1;
				System.Environment.Exit(IsIdentified(argument));
			}
		}
	}

	class Program
	{
		static void Main(string[] args)
		{
			DebugViewPlugin.ProcessCommandline(args);
			foreach (var arg in args)
			{
				if (arg.ToLower().Contains("--verbose"))
					Settings.Verbose = true;
			}

			if (args.Length < 1)
			{
				Console.WriteLine("usage: httpmonitor <url>");
				return;
			}
			string url = args[0];
		
			Console.WriteLine("Poll interval: " + Settings.PollInterval);
			Console.WriteLine("URL: " + url);
			Console.WriteLine("Monitoring...");

			//Trust all certificates
			System.Net.ServicePointManager.ServerCertificateValidationCallback =
				((sender, certificate, chain, sslPolicyErrors) => true);

			Log log = new Log(url);

			try
			{
				log.Monitor();
			}
			catch (Exception e)
			{
				Util.DebugWrite(e.ToString());
			}
			Util.DebugWrite("Monitoring " + url + " ended.");
		}
	}
}
