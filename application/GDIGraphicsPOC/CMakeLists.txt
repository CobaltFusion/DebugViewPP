project(GDIGraphicsPOC)

add_executable(${PROJECT_NAME}
    resource.rc
    MainFrame.cpp
    CMainFrame2.cpp
    Logview.cpp
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
        dv::library
        dv::win32
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)
