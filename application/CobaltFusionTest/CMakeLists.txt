project(CobaltFusionTest)

add_executable(${PROJECT_NAME} CobaltFusionTest.cpp TestExecutor.cpp TestGuiExecutor.cpp)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
        project::definitions
        project::compile_features
        project::compile_options
		nuget::boost_test
		CobaltFusion
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

add_test(${PROJECT_NAME} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
