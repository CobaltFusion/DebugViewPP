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

get_property(EXE_NAME TARGET DebugViewConsole PROPERTY TARGET_FILE_NAME)
add_custom_command(TARGET DebugViewConsole POST_BUILD COMMAND ${PROJECT_UTILS_DIRECTORY}/upx.exe --best $<TARGET_FILE:DebugViewConsole>)

# CPACK_ARCHIVE_FILE_NAME
