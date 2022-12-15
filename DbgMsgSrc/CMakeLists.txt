project(DbgMsgSrc)

add_executable(${PROJECT_NAME} DbgMsgSrc.cpp Timer.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        project_definitions
        project_compile_features
        nuget::boost
        dv::cobaltfusion
        dv::library
        dv::win32
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)
