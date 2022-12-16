cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

project(
  debugviewpp
  VERSION 0.0.1
  DESCRIPTION "DebugView++, collects, views, filters your application logs"
  HOMEPAGE_URL "https://github.com/CobaltFusion/DebugViewPP"
  LANGUAGES CXX C
)

enable_testing()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(CMakeLists.nuget)

add_library(project_definitions INTERFACE)
add_library(project_compile_features INTERFACE)
add_library(project_compile_options INTERFACE)
add_library(msvc_gui_link_options INTERFACE)
add_library(msvc_console_link_options INTERFACE)

target_compile_features(project_compile_features INTERFACE cxx_constexpr cxx_auto_type cxx_final cxx_std_17)
target_compile_definitions(project_definitions
	INTERFACE
	UNICODE _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN
	WIN32 WIN64 "_WIN32_WINNT=0x0601"
	_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
)
target_compile_options(project_compile_options INTERFACE /utf-8 /permissive)
target_link_options(msvc_gui_link_options INTERFACE /SUBSYSTEM:WINDOWS /NOLOGO)
target_link_options(msvc_console_link_options INTERFACE /SUBSYSTEM:CONSOLE /NOLOGO)

add_library(project::definitions ALIAS project_definitions)
add_library(project::compile_features ALIAS project_compile_features)
add_library(project::compile_options ALIAS project_compile_options)
add_library(project::gui_link_options ALIAS msvc_gui_link_options)
add_library(project::console_link_options ALIAS msvc_console_link_options)

add_subdirectory(application)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Debugviewpp)
