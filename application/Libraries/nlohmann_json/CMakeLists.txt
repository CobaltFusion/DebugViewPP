project(nlohmann_json)

# JSON for Modern C++ 3.11.3 (https://github.com/nlohmann/json), MIT license, see LICENSE.MIT. The single header version.
# Use it as #include <nlohmann/json.hpp>
add_library(${PROJECT_NAME} INTERFACE)
add_library(dv::json ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
