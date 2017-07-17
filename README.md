Cobalt Fusion presents:

DebugView++
----------

[Download latest here](https://github.com/djeedjay/DebugViewPP/releases)

DebugView++ started as a viewer for Win32 OutputDebugString messages in the style of
Sysinternals DebugView. However, it can now be attached to virtually any other kind of logging, such as:
- tailing ascii and UTF logfiles (just drag it onto the window)
- Android ADB (or any console based standard output)
- serial ports (using plink)
- sockets, telnet or ssh ports (also using plink)
- it can listen for UDP messages, handy in distributed systems

See examples down below.

Sponsors
--------

This project is sponsored by:

[![Resharper logo](art/resharpercpp_logo.png)](https://www.jetbrains.com/)

Build in code analysis, handy auto-fixes and refactoring options

[![Backtrace logo](art/backtrace_logo.png)](https://backtrace.io/ )

Gather and analyse crash information.

[![Incredibuild logo](art/incredi_logo.png)](https://www.incredibuild.com/)

We use Incredibuild to make use of all cores of multiple machines to accelerate building our C++ projects.

So why is this Debugview++ usefull and why not just use a debugger?
- first of all, this way you can see messages from different processes, not just 'attached' processes.
- alos: filtering, coloring and linking. To make sense of a large amount of information humans need to filter it or order it understand it. Also it helps if important events have different colors to quickly interpret the occurring patterns.
- finally, filtering is nice, but sometimes you need to see a line in its context to understand it, this is where linked views can help to quickly switch between a fully filtered view and a fully detailed view. 

Expected changes in next stable version 1.8.x:
- bugfixes
- last version with (official) XP support (v140_xp target)
- internal refactoring from boost to C++11/14 constructs
- no new features planned, if you're missing something you need, file an issue!

Features we dream about and will create when we choose to spend the time:
- a gantt chart-like view, a horzontal timeline, with bars/flags/signs on it to identify events
- a better plugin based input system
- transparent background streaming to disk
- proper memory limits

Known issues:
- the history limit doesn't work right, this is troublesome for long-running duration-tests.
  A workaround is to send 'DBGVIEWCLEAR' before each test-cycle (this clears all logs from memory).
- there is no 'pass-through' mode like the original dbgview had, if you can help me implement this, please contact me.
- same goes for catching kernel messages, help wanted.

References
----------
[OutputDebugString on MSDN](https://msdn.microsoft.com/en-us/library/windows/desktop/aa363362(v=vs.85).aspx)

Screenshot
-----------
![DebugView++ Screenshot](art/syntax_high.png "DebugView++ Screenshot")

Here are some features:

- single selfcontaining executable, setup is provided but not required
- minimal delay of the traced application, compared to the original dbgview a factor of 10 better.
- fast and responsive user-interface, even with +50.000 incoming lines per second
- runs without prerequisites on WinXPSP3 and up (v1.5 and earlier also on WinXPSP2)
- in-memory compressed logbuffer using google snappy (typically -50% RAM consumtion)
- tailing files (drag ascii or UTF files into debugview to tail it)

And more features:

- after v1.8 we drop WindowsXP support, allowing us to move to C++17 or at least the parts that are available in vs2017/ v141 
- capture both Win32 and Global Win32 messages
- tabbed views
- resolve process PID to name and track their lifetime
- filter by process or message
- advanced filtering, exclude, track, stop, clear (optionally using regular expressions) 
- line and token highlighting (create your own syntax highlighting)
- SAIT (search-as-I-type) + token highlighting
- bookmarks
- statusbar shows detailed log/view/selection information
- open saved logs for post-mortum analysis
- commandline version
- capture stdin piped messages, allows you to connect any kind of logging
- beep-filter for monitoring without seeing the screen (To hear it make sure a 'Default Beep' sound is defined in Control Panel->Sounds)
- clear Log now releases the message buffer instead of reusing the memory (useful when running debugview 
  for a very long time)
- tailing logfiles over samba network (experimental)
- support for reading and tailing Sysinternals Dbgview logfiles (in the four most common formats)

Added in 1.5:

- added console version (DebugViewConsole.exe) for use without UI
- several minor UI bugs fixed
- dbgview agent client mode allowing logging of kernel messages
- added socket listening, Log->Sources->Add can add TCP and UDP listeners, the protocol is sending raw newline terminated strings. Multiple lines can be send in one packet.
- fixed troubles with tabs (highlighting/logfile/regex)
- timezone independent and human readable timestamps in the logfiles
- save filters after changing instead of only at exit
- moved filters out of sub-menu and add shortcut keys (try highlighting a word and pressing delete)
- add basic support for tailing unicode logfiles (unicode characters are truncated)
- fixed crash when saving files in UTC-n timezones
 
Not new features, but often overlooked, see below for details
- View->Process Colors, easy way to give every process its own color!
- Options->Link views, best effort to synchronize the line-selection over all views

Changes in 1.7.x so far:
- restructuring log-sources code 
- experimental horizontal scrolling by dragging mouse
- fixed a bug in version 1.6.48 that prevented dbgview-agent messages from showing
- added name of tailing file in window title
- fixed swallowing newlines
- fixed DebugviewConsole (was broken in 1.6)
- fixed all tests
- fixed threading-issues

Download latest version (stable, dated 20 Sept 2015)
-----------------------
+ [DebugView v1.5.x Zipped executables](http://www.myquest.nl/sites/debugview/DebugView++v1.5._2016_03_12.zip)
+ [DebugView v1.5.x Win32 installer](http://www.myquest.nl/sites/debugview/DebugView++v1.5._2016_03_12.msi)

Screenshot demonstrating bookmarks and highlighting features.

**Highlighted**:

- regex (token filter):     ``[^\s]*\\[\\\w+\.\s]+``    filenames in blue
- regex (token filter):     ``0x\w+``                   hexadecimal numbers in red
- regex (highlight filter): ``Unittest``                lines with the word 'Unittest' have a lightgreen background
- a doubleclick on 'bytes' causes all instances of 'bytes' to highlight in yellow

See http://www.cplusplus.com/reference/regex/ECMAScript/ for all options for supported regular expressions

**Android ADB example**:

![DebugView++ Screenshot](art/logcat_example.png "DebugView++ Screenshot")

Screenshot demonstrating connecting to ADB logcat (Android Debug Bridge)

More examples
-------------

**Connect any pipe**:

To connect directly to a port or service, [plink] can be used, make sure an instance of debugview++ is already running before running this command:

> plink -ssh -batch -v 192.168.0.1 2>&1 | debugview++

Notice that 2>&1 is used *before* the pipe (|) symbol to redirect stderr to stdout.

[plink]: http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html

**Connect to sysinternals dbgview agent for kernel messages**:

Example: connect to sysinternals DbgView Agent, first start Dbgview.exe /a /k (/k for kernel messages)
And connect DebugView++ using Log->Connect DebugView Agent. Note that 'Log->Connect DebugView Agent' assumes the agent is running on the same workstation as DebugView++ so it connects to 127.0.0.1. If you need to connect to a remote agent, use Log->Sources...->Add->DbgView Agent and fill in the ip-address.

**Use RegexGroups + Token Highlighting**:

Suppose you want to highlight some data value in your logging, since the actually value may differ, you cannot use normal matching to highlight them. With RegexGroups you can match text _before_ or _after the actual token you want to highlight.

Example:

![FilterDialog Screenshot](art/regexgroups.png "RegexGroups Screenshot")

Filters:
--------

![FilterDialog Screenshot](art/filterdialog.png "FilterDialog Screenshot")


Filters can be defined per view, for example choose File -> New View, and the filter dialog will popup.
Pressing OK will open a new view without any filters. 

Different types of filters:

All filters support regular expressions, if you are not familliar with regular expressions you can
just type any word or part of a word to match.

- include: if an include filter is added only lines containing a matching expression will be included.
- exclude: lines containing a matching expression will be excluded from the view, excluding always takes precedence over including
- once: only the first line containing a matching expression will be included, this resets automatically at 'clear view'. 
- highlight: lines containing a matching expression will be highlighted using the specified foreground and background colors
- token: only the matching expression will be highlighted using the specified foreground and background colors
- track: lines containing a matching expression will be focused and centered if possible. Note: auto scroll turns off if a track filter is matched 
- stop: if a matching expression is found autoscroll is turned off, all track filters will be disabled and the line is focused. Note: stop filters work only if autoscroll is on, think of a stop-filter as a single-shot track filter
- beep: a standard windows beep (configurable in config panel->sounds) is played 

*Practical uses*:

Include, exclude, once and highlight filters are the most intuitive filters to use. Track and stop can be a little confusing, let me try to give some examples.

**track**: use this filter to focus interesting lines that do not occur very often, but at a regular interval, for example, so you are monitoring a process that logs output every 30 seconds and you need to check the result. 

**stop**: this filter is good when some special event occurs (an exception?) and you want to inspect the context of the event in the log before continuing. A press of the 'end' button will resume auto scrolling.

Other features:
--------------------

**link views**: the selected line in the current view is located re-selected when you switch to another view. This is done on a best-effort bases, so if the exact line is not found, the nearest line is selected. In that case switching views will cause the currently selected line to change.

Consider this use case:

If you want to have auto scoll on, but some high frequeny messages are annoying you, but you cannot exclude them because they help you diagnose your event when it occurs, try this:

Use two views, one where the diagnostic messages are filtered and autoscroll is on, and one where the messages are included (and maybe highlighted), next turn on the 'link views' feature.

Now you can monitor the filtered view, and when your event occurs, select a line and switch to the unfiltered view, the same line is now highlighted, but in full unfiltered context.

**process colors**: If enabled each process (even processed with identical names) will get a its own background color automatically without adding any filters.

Other documentation:
--------------------

**Auto Scroll**: scrolls down to new incoming lines automatically

**Auto Scroll Stop**: If enabled 'auto scroll' is turned on when the last line in the view is selected, and also
turned off if any other line is selected.

**Bookmarks**: bookmarks are view specific and can be placed on a line by clicking left of the line number or Ctrl+F2, press F2 to move to the next bookmark. Bookmarks are temporary, so cannot be saved.

**ClockTime**: when enabled the time is displayed as provided by the system's real-time clock (RTC). Such a timestamp has a 16ms resolution on a typical desktop PC. When disabled, time is displayed as a relative time to the first message, however this timestamp is obtained from the High-Performance Counter (HPC) which typically has a sub-microsecond resolution.

The resolution should not be confused with accuracy here, the recorded timestamp is not the actual time the message occured, it is the time the message was received by DebugView++. Also there is no quarantee that the time between occurance and reception of messages is constant, *however* in practice this is **pretty** constant :)

How to build
------------
This is a Visual Studio 2017 project targeted to v140_XP, we plan to move to v141 soon.
The projects are configured to use Nuget to get there dependencies (boost and WTL)
This means that after cloning the GIT repository, you get press 'build', visual studio will download boost and WTL the first time only  and then build the project. It is as simple as that... if it is not, contact me (jan), so I can fix any remaining issues.

- zip.exe, http://gnuwin32.sourceforge.net/packages/zip.htm, choose [zip-3.0-setup.exe]
 decompress the archive and you're done (add zip.exe to the path)

-= Cobalt Fusion =-

Jan Wilmans
mailto:janwilmans _at_ gmail.com

Gert-Jan de Vos
mailto:boosttestui@on.nl

