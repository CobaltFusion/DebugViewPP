project(DebugView++Lib)

add_library(${PROJECT_NAME}
    AnyFileReader.cpp
    BinaryFileReader.cpp
    Colors.cpp
    Conversions.cpp
    CTimelineView.cpp
    DbgviewReader.cpp
    DBWinBuffer.cpp
    DBWinReader.cpp
    DBWinWriter.cpp
    FileIO.cpp
    FileReader.cpp
    FileWriter.cpp
    Filter.cpp
    FilterType.cpp
    Line.cpp
    LineBuffer.cpp
    LogFile.cpp
    LogFilter.cpp
    LogSource.cpp
    LogSources.cpp
    Loopback.cpp
    MatchType.cpp
    NewlineFilter.cpp
    PipeReader.cpp
    PolledLogSource.cpp
    ProcessInfo.cpp
    ProcessMonitor.cpp
    ProcessReader.cpp
    SocketReader.cpp
    SourceType.cpp
    stdafx.cpp
    TestSource.cpp
    TimelineDC.cpp
)

add_library(dv::library ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        project_definitions
        project_compile_features
        nuget::boost
        nuget::wtl
        dv::cobaltfusion
        dv::win32
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)
