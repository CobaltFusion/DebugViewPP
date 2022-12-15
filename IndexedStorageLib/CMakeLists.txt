project(IndexedStorageLib)

add_library(${PROJECT_NAME} IndexedStorage.cpp)
add_library(dv::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        project_definitions
        project_compile_features
        nuget::snappy
        dv::win32
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)
