project(win32lib)

add_library(${PROJECT_NAME} Com.cpp Process.cpp Registry.cpp Socket.cpp Utilities.cpp Win32Lib.cpp Window.cpp)
add_library(dv::win32 ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
	PRIVATE
        project::definitions
        project::compile_features
        project::compile_options
)

target_include_directories(win32lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

