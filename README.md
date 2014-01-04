Cobalt Fusion presents:

DebugView++
----------

DebugView++ is a viewer for Win32 OutputDebugString based logging in the style of
Sysinternals DebugView. DebugView++ features:

- single selfcontaining executable, setup is provided but not required
- runs without prerequisites on WinXPSP3, Vista and 7/8.x
- capture both Win32 and Global Win32 messages
- tabbed views
- resolve PID to process name
- filter by process or message
- advanced filtering, exclude, track, stop (ability to use regular expressions) 
- line and token highlighting (create your own syntax highlighting)
- minimal delay of the traced application, compared to debugview a factor of 10 better.
- fast and responsive user-interface, even with +50.000 incoming lines per second
- SAIT (search-as-I-type) token highlighting
- bookmarks
- statusbar shows detailed log/view/selection information
- open saved logs for post-mortum analysis
- memory compressed logbuffer using google snappy (-50% RAM consumed)


Screenshot
----------
![DebugView++ Screenshot](/DebugView++/art/syntax_high.jpg "DebugView++ Screenshot")

The screenshot demonstrates bookmarks and highlighting features:

Highlighted:
------------
- regex (token filter):     ``[^\s]*\\[\\\w+\.\s]+``    filenames in blue
- regex (token filter):     ``0x\w+``                   hexadecimal numbers in red
- regex (highlight filter): ``Unittest``                lines with the word 'Unittest' have a lightgreen background
- a doubleclick on 'bytes' causes all instances of 'bytes' to highlight in yellow

See http://www.cplusplus.com/reference/regex/ECMAScript/ for all options for supported regular expressions

Filters:
--------

Filters can be defined per view, for example you can File->New View, and the filter dialog will popup.
Pressing OK will open a new view without any filters. 

Different types of filters:

All filters support regular expressions, if you are not familliar with regular expressions you can
just type any word or part of a word to match.

- include: ??? see highlight?
- exclude: lines containing a matching expression will be excluded from the view
- highlight: lines containing a matching expression will be highlighted using the specified foreground and background colors
- token: only the matching expression will be highlighted using the specified foreground and background colors
- track: lines containing a matching expression will be focused and centered if possible. Note: auto scroll turns off if a track filter is matched 
- stop: if a matching expression is found autoscroll is turned off, all track filters will be disabled and the line is focused. Note: stop filters work only of autoscroll is on, think of a stop-filter as a one-shot track filter
- ignore: lines containing a matching expression will be ignored by the auto scroll feature. 

*Practical uses*:

Exclude and highlight filters are the most intuitive filters to use. Track, stop and ignore can be a little confusing, let me try to give examples of all filters.

**track**; use this filter to focus interesting lines that do not occur very often, but at a regular interval, for example, so you are monitoring a process that logs output every 30 seconds and you need to check the result. 

**stop**; this filter is good when some special event occurs (an exception?) and you want to inspect the context of the event in the log before continuing. A press of the 'end' button will resume auto scrolling.

**ignore**; this filter is usefull if you want to have auto scoll on, but some high frequeny messages are annoying you, but you cannot exclude them because they help you diagnose your problem when it occurs.

In this last case you can consider a different approach:

Use two views, one where the diagnostic messages are filtered and autoscroll is on, and one where the messages are included (and maybe highlighted), next turn on the 'link views' feature.



Other documentation:
--------------------

Auto scroll: scrolls down to new incoming lines automatically; autoscroll is turned on automatically when the last line in the view is selected.



See http://www.cplusplus.com/reference/regex/ECMAScript/ for all options of the supported regular expressions


How to build
------------

This is a Visual Studio 2010 project with the following dependencies:
- boost 1.43 or later
- WTL 8.0 or later

These libraries must be installed in /Libraries. All projects should now build.

-= Cobalt Fusion =-

Gert-Jan de Vos
mailto:boosttestui@on.nl

Jan Wilmans
mailto:janwilmans _at_ gmail.com
