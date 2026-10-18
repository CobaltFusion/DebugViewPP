project(DebugviewppTest)

add_executable(${PROJECT_NAME} DebugView++Test.cpp)

target_link_libraries(${PROJECT_NAME}
PRIVATE
    project::definitions
    project::compile_features
    project::compile_options
    nuget::boost_test
    dv::library
    dv::indexedstorage
    CobaltFusion
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

# the files the tests read, see testdata/README.md
target_compile_definitions(${PROJECT_NAME} PRIVATE TESTDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/testdata")

add_test(${PROJECT_NAME} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}" WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
