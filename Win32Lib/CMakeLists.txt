cmake_minimum_required(VERSION 3.12)

add_library(win32lib Com.cpp Process.cpp Registry.cpp Socket.cpp Utilities.cpp Win32Lib.cpp Window.cpp)
add_library(fusion::win32lib ALIAS win32lib)

target_compile_definitions(win32lib PUBLIC UNICODE)
target_compile_definitions(win32lib PUBLIC _CRT_SECURE_NO_WARNINGS)

target_compile_features(win32lib PRIVATE cxx_constexpr cxx_auto_type cxx_final cxx_std_17)

target_include_directories(win32lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

