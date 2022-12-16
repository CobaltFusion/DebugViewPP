project(PropertyGrid)

add_library(${PROJECT_NAME} INTERFACE)
add_library(dv::propertygrid ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include)

#add_executable(PropertyControlTest PropertyControlTest.cpp PropertyControlTest.rc)

#target_link_libraries(PropertyControlTest
#    PRIVATE
#        project_definitions
#        project_compile_features
#        project_compile_options
#        nuget::wtl
#        dv::propertygrid
#        dv::tabbingframework
#)
