cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(CMakeLists.buildnumber)
include(CMakeLists.nuget)

MESSAGE("Building DebugViewpp version: ${DEBUGVIEW_VERSION}")

project(
  debugviewpp
  VERSION ${DEBUGVIEW_VERSION}
  DESCRIPTION "DebugView++, collects, views, filters your application logs"
  HOMEPAGE_URL "https://github.com/CobaltFusion/DebugViewPP"
  LANGUAGES CXX C
)

enable_testing()

add_compile_options(/MP)

if(BUILD_WITH_ASAN)
    add_compile_options(/fsanitize=address /D_DISABLE_VECTOR_ANNOTATION=0)
    message("-- build with ASAN enabled")
endif(BUILD_WITH_ASAN)

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
endif()

set(PROJECT_UTILS_DIRECTORY "${PROJECT_SOURCE_DIR}/utils")

add_library(project_definitions INTERFACE)
add_library(project_compile_features INTERFACE)
add_library(project_compile_options INTERFACE)
add_library(msvc_gui_link_options INTERFACE)
add_library(msvc_console_link_options INTERFACE)

target_compile_features(project_compile_features INTERFACE cxx_constexpr cxx_auto_type cxx_final cxx_std_20)
target_compile_definitions(project_definitions
	INTERFACE
	UNICODE
	NOMINMAX
	WIN32_LEAN_AND_MEAN
	_CRT_SECURE_NO_WARNINGS
	_WINSOCK_DEPRECATED_NO_WARNINGS
	WIN32
	WIN64
	_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	"_WIN32_WINNT=0x0601"
)
# 	"_ITERATOR_DEBUG_LEVEL=0" # trips over debug/release mismatches

# /Oy and /RELEASE are added to avoid tripping malware detection
# this is quite random, ymmv
target_compile_options(project_compile_options INTERFACE /utf-8 /Oy)
target_link_options(msvc_gui_link_options INTERFACE /SUBSYSTEM:WINDOWS /NOLOGO /RELEASE)
target_link_options(msvc_console_link_options INTERFACE /SUBSYSTEM:CONSOLE /NOLOGO /RELEASE)

# warnings (the project picked up a bunch of unresolved warnings with migrating to new VS versions and when {fmt} was added, work in progress...)
target_compile_options(project_compile_options INTERFACE /W4 /w14242 /w14254 /w14263 /w14265 /w14287 /w14289 /w14296 /w14311 /w14545 /w14546 /w14547 /w14549 /w14555 /w14640 /w14826 /w14905 /w14906 /w14928 /we4715)

add_library(project::definitions ALIAS project_definitions)
add_library(project::compile_features ALIAS project_compile_features)
add_library(project::compile_options ALIAS project_compile_options)
add_library(project::gui_link_options ALIAS msvc_gui_link_options)
add_library(project::console_link_options ALIAS msvc_console_link_options)

add_subdirectory(application)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Debugviewpp)

message("")
message("configuration:        ${CMAKE_BUILD_TYPE}")
message("will be built in:     ${CMAKE_BINARY_DIR}")
message("CMAKE_C_COMPILER:     ${CMAKE_C_COMPILER}")
message("CMAKE_CXX_COMPILER:   ${CMAKE_CXX_COMPILER}")
message("will be installed in: ${CMAKE_INSTALL_PREFIX}")
message("\n")

