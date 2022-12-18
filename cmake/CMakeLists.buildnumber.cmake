# Set the initial build number
set(BUILD_NUMBER 0)

# Check if the build number file exists
if(EXISTS ${CMAKE_SOURCE_DIR}/cmake/build_number.txt)
  # If it exists, read the current build number from the file
  file(READ ${CMAKE_SOURCE_DIR}/cmake/build_number.txt BUILD_NUMBER)
endif()

# Increment the build number
math(EXPR BUILD_NUMBER "${BUILD_NUMBER} + 1")

# Write the updated build number to the file
file(WRITE ${CMAKE_SOURCE_DIR}/cmake/build_number.txt ${BUILD_NUMBER})

# Set the build number as a CMake variable
set(DEBUGVIEW_VERSION "1.9.0.${BUILD_NUMBER}" CACHE INTERNAL "")
