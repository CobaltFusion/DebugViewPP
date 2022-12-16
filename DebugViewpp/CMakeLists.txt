project(Debugviewpp)

add_executable(${PROJECT_NAME}
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

target_link_libraries(${PROJECT_NAME}
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

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)
