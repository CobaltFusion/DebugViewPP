project(fmt_library)

add_library(${PROJECT_NAME} src/format.cc src/os.cc)
add_library(fmt::fmt ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include)

