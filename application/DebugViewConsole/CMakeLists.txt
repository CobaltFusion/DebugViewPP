project(DebugViewConsole)

add_executable(DebugViewConsole DebugViewConsole.cpp)

target_link_libraries(DebugViewConsole
PRIVATE
    project::definitions
    project::compile_features
    project::compile_options
    nuget::boost_test
    dv::library
    dv::indexedstorage
    CobaltFusion
    docopt
)

install(TARGETS DebugViewConsole RUNTIME DESTINATION bin)

target_include_directories(DebugViewConsole PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

add_custom_command(
  TARGET DebugViewConsole
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  $<TARGET_FILE:DebugViewConsole>
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/DebugViewConsole-small.exe

  COMMAND ${PROJECT_UTILS_DIRECTORY}/upx.exe
          ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/DebugViewConsole-small.exe
)

install(FILES $<TARGET_FILE_DIR:DebugViewConsole>/DebugViewConsole-small.exe DESTINATION bin)
