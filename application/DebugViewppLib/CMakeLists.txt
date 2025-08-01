project(DebugView++Lib)

add_library(${PROJECT_NAME}
    AnyFileReader.cpp
    BinaryFileReader.cpp
    Colors.cpp
    Conversions.cpp
    CTimelineView.cpp
    DbgviewReader.cpp
    Debugview_kernel_client.cpp
    DBWinBuffer.cpp
    DBWinReader.cpp
    DBWinWriter.cpp
    FileIO.cpp
    FileReader.cpp
    FileWriter.cpp
    Filter.cpp
    FilterType.cpp
    KernelReader.cpp
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
    TestSource.cpp
    TimelineDC.cpp
    VectorLineBuffer.cpp
)

add_library(dv::library ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        project::definitions
        project::compile_features
        project::compile_options
        nuget::boost
        nuget::wtl
        dv::cobaltfusion
        dv::win32
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)
