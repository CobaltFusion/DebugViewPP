project(DebugViewConsole)

add_executable(${PROJECT_NAME} DebugViewConsole.cpp)

target_link_libraries(${PROJECT_NAME}
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

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
