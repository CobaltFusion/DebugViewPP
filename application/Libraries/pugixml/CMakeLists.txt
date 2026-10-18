project(pugixml)

# pugixml 1.14 (https://pugixml.org), MIT license, see LICENSE.md. Used to read the legacy XML configuration files.
# Vendored as it is: it is compiled without the warning options of the project.
add_library(${PROJECT_NAME} STATIC pugixml.cpp)
add_library(dv::pugixml ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
