project(TabbingFramework)

add_library(${PROJECT_NAME} INTERFACE)
add_library(dv::tabbingframework ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
