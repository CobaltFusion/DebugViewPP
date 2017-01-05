//------------------------------------------------------------------------------
// <copyright file="Forwarder.cs" company="Company">
//     Copyright (c) Company.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.ComponentModel.Design;
using System.Globalization;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.TextManager.Interop;
using Microsoft.VisualStudio.Editor;
using Microsoft.VisualStudio.Text.Editor;
using System.Diagnostics;

namespace OutputForwarderVSIX
{
    /// <summary>
    /// Command handler
    /// </summary>
    internal sealed class Forwarder
    {
        ///// <summary>
        ///// Command ID.
        ///// </summary>
        //public const int CommandId = 0x0100;

        ///// <summary>
        ///// Command menu group (command set GUID).
        ///// </summary>
        //public static readonly Guid CommandSet = new Guid("d337dc6e-ca3f-4b71-a608-575e3eabe693");

        /// <summary>
        /// VS Package that provides this command, not null.
        /// </summary>
        private readonly Package package;

        /// <summary>
        /// Initializes a new instance of the <see cref="Forwarder"/> class.
        /// Adds our command handlers for menu (commands must exist in the command table file)
        /// </summary>
        /// <param name="package">Owner package, not null.</param>
        private Forwarder(Package package)
        {
            if (package == null)
            {
                throw new ArgumentNullException("package");
            }

            this.package = package;
            InstallForwarder(package);
        }

        private void InstallForwarder(Package package)
        {
            IVsOutputWindow outWindow = Package.GetGlobalService(typeof(SVsOutputWindow)) as IVsOutputWindow;
            Guid paneGuid = VSConstants.GUID_OutWindowDebugPane;    // codenotes: GUID_BuildOutputWindowPane / GUID_OutWindowDebugPane
            IVsOutputWindowPane debugPane;
            outWindow.GetPane(ref paneGuid, out debugPane);
            debugPane.Activate(); // Brings this pane into view

            IVsUserData userData = (IVsUserData)debugPane;
            object o;
            Guid guidViewHost = DefGuidList.guidIWpfTextViewHost;
            userData.GetData(ref guidViewHost, out o);

            IWpfTextViewHost viewHost = (IWpfTextViewHost)o;
            m_textView = viewHost.TextView;
            m_textView.TextBuffer.Changed += new EventHandler<Microsoft.VisualStudio.Text.TextContentChangedEventArgs>(TextBuffer_Changed);

            debugPane.OutputString("Cobalt Fusion Output Window forwarding initialized.\n");
            Trace.AutoFlush = true;
        }

        void TextBuffer_Changed(object sender, Microsoft.VisualStudio.Text.TextContentChangedEventArgs e)
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

        /// <summary>
        /// Gets the instance of the command.
        /// </summary>
        public static Forwarder Instance
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the service provider from the owner package.
        /// </summary>
        private IServiceProvider ServiceProvider
        {
            get
            {
                return this.package;
            }
        }

        /// <summary>
        /// Initializes the singleton instance of the command.
        /// </summary>
        /// <param name="package">Owner package, not null.</param>
        public static void Initialize(Package package)
        {
            Instance = new Forwarder(package);
        }

        private IWpfTextView m_textView;
    }
}
