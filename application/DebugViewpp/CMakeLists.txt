project(Debugviewpp)

add_executable(Debugviewpp
    AboutDlg.cpp
    CLogViewTabItem2.cpp
    DebugView++.cpp
    DropTargetSupport.cpp
    FileOptionDlg.cpp
    FilterDlg.cpp
    FilterPage.cpp
    FindDlg.cpp
    Grid.cpp
    HistoryDlg.cpp
    LogView.cpp
    MainFrame.cpp
    RegExDlg.cpp
    RenameProcessDlg.cpp
    RunDlg.cpp
    SourceDlg.cpp
    SourcesDlg.cpp
    DebugView++.rc
 )

target_link_libraries(Debugviewpp
    PRIVATE
        project::definitions
        project::compile_features
        project::compile_options
        project::gui_link_options
        nuget::boost
        nuget::wtl
        dv::cobaltfusion
        dv::tabbingframework
        dv::propertygrid
        dv::indexedstorage
        dv::library
        dv::win32
)

install(TARGETS Debugviewpp DESTINATION bin)

target_include_directories(Debugviewpp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

install(CODE "
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debugviewpp.exe ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debugviewpp-small.exe)
  execute_process(COMMAND ${PROJECT_UTILS_DIRECTORY}/upx.exe ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debugviewpp-small.exe)
")

INSTALL(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debugviewpp-small.exe DESTINATION bin)
