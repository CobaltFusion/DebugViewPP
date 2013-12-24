DebugView++
~~~~~~~~~~~

DebugView++ is a viewer for Win32 OutputDebugString based logging in the style of
Sysinternals DbgView. DebugView++ features:

- [x] tabbed views
- [x] advanced filtering, exclude, track, stop (ability to use regular expressions) 
- [x] line and token highlighting (create your own syntax highlighting)
- [x] minimal delay of the traced application
- [x] responsive user-interface, even with +50.000 incoming lines per second
- [x] SAIT (search-as-I-type) token highlighting
- [x] bookmarks
- [x] statusbar shows detailed log/view/selection information
- [x] open saved logs for post-mortum analysis

How to build
------------

This is a Visual Studio 2010 project with the following dependencies:
- boost 1.43 or later
- WTL 8.0 or later

These libraries must be installed in /Libraries. All projects should now build.


Gert-Jan de Vos
mailto:boosttestui@on.nl

Jan Wilmans
mailto:janwilmans _at_ gmail.com
