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

highlighted:
------------
- regex (token filter):     ``[^\s]*\\[\\\w+\.\s]+``    filenames in blue
- regex (token filter):     ``0x\w+``                   hexadecimal numbers in red
- regex (highlight filter): data                    lines with the word 'data' lightgreen
- a doubleclick on 'bytes' causes all instances of 'bytes' to highlight in yellow

See http://www.cplusplus.com/reference/regex/ECMAScript/ for all options for supported regular expressions

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
