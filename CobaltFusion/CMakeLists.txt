cmake_minimum_required(VERSION 3.12)
project(CobaltFusion)

add_library(${PROJECT_NAME} CircularBuffer.cpp Executor.cpp ExecutorClient.cpp fusionassert.cpp GuiExecutor.cpp Throttle.cpp Timer.cpp)
add_library(fusion::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME}
    PRE_BUILD
    COMMAND nuget restore ${CMAKE_BINARY_DIR}/DebugView++.sln
)

#find_package( Boost REQUIRED COMPONENTS program_options regex )

target_compile_definitions(${PROJECT_NAME} PUBLIC UNICODE)
target_compile_definitions(${PROJECT_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_constexpr cxx_auto_type cxx_final cxx_std_17)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

