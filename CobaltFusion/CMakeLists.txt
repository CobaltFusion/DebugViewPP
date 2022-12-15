project(CobaltFusion)

add_library(${PROJECT_NAME} CircularBuffer.cpp Executor.cpp ExecutorClient.cpp fusionassert.cpp GuiExecutor.cpp Throttle.cpp Timer.cpp)
add_library(dv::cobaltfusion ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        project::definitions
        project::compile_features
        project::compile_options
        nuget::boost
        dv::win32
)

add_custom_command(TARGET ${PROJECT_NAME}
    PRE_BUILD
    COMMAND  ${CMAKE_SOURCE_DIR}/nuget.exe restore ${CMAKE_SOURCE_DIR}/DebugView++.sln
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)
