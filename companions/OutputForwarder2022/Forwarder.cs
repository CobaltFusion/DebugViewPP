using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Editor;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.Text.Editor;
using Microsoft.VisualStudio.TextManager.Interop;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace OutputForwarder2022
{
    internal class Forwarder
    {
        Forwarder(AsyncPackage package)
        {
            InstallForwarder(package);
        }

        private void InstallForwarder(AsyncPackage package)
        {
            // Get a reference to the pane in the output window
            IVsOutputWindow outputWindow = Package.GetGlobalService(typeof(SVsOutputWindow)) as IVsOutputWindow;
            Guid paneGuid = VSConstants.GUID_OutWindowDebugPane;    // codenotes: GUID_BuildOutputWindowPane / GUID_OutWindowDebugPane
            IVsOutputWindowPane debugPane;
            outputWindow.GetPane(ref paneGuid, out debugPane);

            IVsUserData userData = debugPane as IVsUserData;
            object o;
            Guid guidViewHost = DefGuidList.guidIWpfTextViewHost;
            userData.GetData(ref guidViewHost, out o);

            IWpfTextViewHost viewHost = o as IWpfTextViewHost;
            m_textView = viewHost.TextView;
            m_textView.TextBuffer.Changed += new EventHandler<Microsoft.VisualStudio.Text.TextContentChangedEventArgs>(TextBuffer_Changed);
            Trace.AutoFlush = true;
        }

        private void TextBuffer_Changed(object sender, Microsoft.VisualStudio.Text.TextContentChangedEventArgs e)
        {
            foreach (var change in e.Changes)
            {
                string text = change.NewText.ToString();
                while (text.Length > 4090)
                {
                    Trace.Write("Following lines exceeded 4090 chars!\n");
                    var part = text.Substring(0, 4090);
                    int nlIndex = part.LastIndexOf("\n", 0);
                    if (nlIndex == -1)
                    {
                        Trace.Write(part);
                        text = text.Substring(4091);
                    }
                    else
                    {
                        Trace.Write(part.Substring(0, nlIndex + 1));
                        text = text.Substring(nlIndex + 1);
                    }
                }
                Trace.Write(text);
            }
        }

        public static Forwarder Instance
        {
            get;
            private set;
        }

        public static void Initialize(AsyncPackage package)
        {
            Instance = new Forwarder(package);
        }

        private IWpfTextView m_textView;
    }
}
