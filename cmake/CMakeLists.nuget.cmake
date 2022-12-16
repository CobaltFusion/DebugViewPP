set(NUGET_SOLUTION ${CMAKE_SOURCE_DIR}/application/nuget/DebugViewpp-nuget.sln)

if (NOT EXISTS ${NUGET_SOLUTION})
	MESSAGE(FATAL_ERROR " NUGET solution ${NUGET_SOLUTION} not found!")
else()
	execute_process(COMMAND ${CMAKE_SOURCE_DIR}/utils/nuget.exe restore ${NUGET_SOLUTION} COMMAND_ERROR_IS_FATAL ANY)
endif()

set(NUGET_PACKAGES_PATH ${CMAKE_SOURCE_DIR}/application/nuget/packages)

add_library(nuget_boost INTERFACE)
target_include_directories(nuget_boost INTERFACE "${NUGET_PACKAGES_PATH}/boost.1.80.0/lib/native/include")
target_link_directories(nuget_boost INTERFACE "${NUGET_PACKAGES_PATH}/boost_date_time-vc143.1.80.0/lib/native")
target_link_directories(nuget_boost INTERFACE "${NUGET_PACKAGES_PATH}/boost_regex-vc143.1.80.0/lib/native")
target_link_directories(nuget_boost INTERFACE "${NUGET_PACKAGES_PATH}/boost_system-vc143.1.80.0/lib/native")
add_library(nuget::boost ALIAS nuget_boost)

add_library(nuget_boost_test INTERFACE)
target_include_directories(nuget_boost_test INTERFACE "${NUGET_PACKAGES_PATH}/boost.1.80.0/lib/native/include")
target_link_directories(nuget_boost_test INTERFACE "${NUGET_PACKAGES_PATH}/boost_unit_test_framework-vc143.1.80.0/lib/native")
add_library(nuget::boost_test ALIAS nuget_boost_test)

add_library(nuget_snappy "${NUGET_PACKAGES_PATH}/Snappy.1.1.1.7/lib/native/src/snappy-single-file.cpp")
target_compile_definitions(nuget_snappy PUBLIC SNAPPY_STATIC)
target_include_directories(nuget_snappy PUBLIC "${NUGET_PACKAGES_PATH}/Snappy.1.1.1.7/lib/native/include")
add_library(nuget::snappy ALIAS nuget_snappy)

add_library(nuget_wtl INTERFACE)
target_include_directories(nuget_wtl INTERFACE "${NUGET_PACKAGES_PATH}/wtl.10.0.10320/lib/native/include")
add_library(nuget::wtl ALIAS nuget_wtl)

