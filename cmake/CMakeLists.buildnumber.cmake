
execute_process(
  COMMAND cmd /C UpdateBuildNr.cmd version.h version.wxi
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Application/DebugviewPP/" 
  COMMAND_ERROR_IS_FATAL ANY
)

file(READ "${CMAKE_SOURCE_DIR}/application/DebugViewpp/version.h" VERSION_FILE_CONTENTS)

# Regular expression to extract VERSION_STR
string(REGEX MATCH "#define VERSION_STR \"([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)\"" VERSION_STR_MATCH "${VERSION_FILE_CONTENTS}")

# Extract the version string from the matched content
if(VERSION_STR_MATCH)
    set(VERSION_STR ${CMAKE_MATCH_1})
else()
    message(FATAL_ERROR "Failed to extract VERSION_STR from version.h")
endif()

# Set the build number as a CMake variable
set(DEBUGVIEW_VERSION "${VERSION_STR}" CACHE INTERNAL "")

MESSAGE("Retrieved version: ${VERSION_STR} from version.h")