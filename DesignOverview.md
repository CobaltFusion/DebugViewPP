Project dependencies
--------------------

![DebugView++ Projects](art/DebugViewProjects.png "DebugView++ Projects")

- 1) PROJECT DebugViewCmd:
CommandLine version of debugview++, 
enables piping debugview output into another process.

2) PROJECT DebugView:
Main project, contains all WTL/ UI  specific code

3) PROJECT DebugView++Lib:
All infrastructure a 'debugview'-like application needs,
but nothing UI-related,for example: filters, FileIO, PipeReader, LogSources

4) PROJECT CobaltFusion:
Anything this is reusable beyond DebugView and has no dependencies other then C++11
(and boost while we are using vs2010)

5) PROJECT Win32Lib:
This project contains left-overs, it is all Windows specific, 
but some of it is WTL-related and should be moved into 
DebugView++/Win32Support.cpp

6) PROJECT IndexedStorageLib:
Contains VectorStorage and SnappyStorage, two
indexed-string-storage classes


Current state of implementation:
--------------------------------

- class LogSources is now a container and not using the circular buffer yet
- m_autoNewline should be a per-logsource setting

- LogSource::Notify() is called from LogSources::Run 
when WaitForMultipleObjects returns and indicated that the corresponding HANDLE is signaled.

- LogSource::Notify() then adds a Line to the circular buffer,
or in case of autonewLine = false, into a fixed 8kb buffer of the LogSource

