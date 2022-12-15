cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

project(
  debugviewpp
  VERSION 0.0.1
  DESCRIPTION "DebugView++, collects, views, filters your application logs"
  HOMEPAGE_URL "https://github.com/CobaltFusion/DebugViewPP"
  LANGUAGES CXX C
)

enable_testing() 

add_library(project_definitions INTERFACE)
add_library(project_compile_features INTERFACE)
add_library(project_compile_options INTERFACE)
#add_library(project_link_options INTERFACE)

target_compile_features(project_compile_features INTERFACE cxx_constexpr cxx_auto_type cxx_final cxx_std_20)
target_compile_definitions(project_definitions
	INTERFACE
	UNICODE _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN WIN32 WIN64
	_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
)
target_compile_options(project_compile_options INTERFACE /utf-8)

#target_link_directories(project_link_options INTERFACE )

add_library(nuget_boost INTERFACE)
target_include_directories(nuget_boost INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/packages/boost.1.80.0/lib/native/include")
target_link_directories(nuget_boost INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/packages/boost_date_time-vc143.1.80.0/lib/native")
target_link_directories(nuget_boost INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/packages/boost_regex-vc143.1.80.0/lib/native")
target_link_directories(nuget_boost INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/packages/boost_system-vc143.1.80.0/lib/native")
add_library(nuget::boost ALIAS nuget_boost)

add_library(nuget_boost_test INTERFACE)
target_include_directories(nuget_boost_test INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/packages/boost.1.80.0/lib/native/include")
target_link_directories(nuget_boost_test INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/packages/boost_unit_test_framework-vc143.1.80.0/lib/native")
add_library(nuget::boost_test ALIAS nuget_boost_test)

add_library(nuget_snappy
	"${CMAKE_CURRENT_SOURCE_DIR}/packages/Snappy.1.1.1.7/lib/native/src/snappy-single-file.cpp"
)

target_compile_definitions(nuget_snappy PUBLIC SNAPPY_STATIC)
target_include_directories(nuget_snappy PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/packages/Snappy.1.1.1.7/lib/native/include")
add_library(nuget::snappy ALIAS nuget_snappy)

add_library(nuget_wtl INTERFACE)
target_include_directories(nuget_wtl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/packages/wtl.10.0.10320/lib/native/include")
add_library(nuget::wtl ALIAS nuget_wtl)

#add_subdirectory(DebugView++)
add_subdirectory(CobaltFusion)
add_subdirectory(CobaltFusionTest)
add_subdirectory(DbgMsgSrc)
add_subdirectory(DebugView++Lib)
#add_subdirectory(DebugView++Test)
#add_subdirectory(DebugViewConsole)
#add_subdirectory(GDIGraphicsPOC)
add_subdirectory(IndexedStorageLib)
#add_subdirectory(libsnappy)
add_subdirectory(Win32Lib)

