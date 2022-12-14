project(CobaltFusionTest)

add_executable(${PROJECT_NAME} CobaltFusionTest.cpp)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		CobaltFusion
		project_definitions
		project_compile_features
		nuget_boost_test
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
