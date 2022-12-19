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
        fmt::fmt
)

install(TARGETS Debugviewpp RUNTIME DESTINATION bin)

target_include_directories(Debugviewpp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

get_property(EXE_NAME TARGET Debugviewpp PROPERTY TARGET_FILE_NAME)
add_custom_command(TARGET Debugviewpp POST_BUILD COMMAND  ${PROJECT_UTILS_DIRECTORY}/upx.exe --best $<TARGET_FILE:Debugviewpp>)

