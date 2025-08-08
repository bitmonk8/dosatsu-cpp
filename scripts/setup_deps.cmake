# Dependency setup helpers
# This file contains helper functions for setting up build dependencies

message(STATUS "Loading dependency setup helpers...")

# Function to check if a required tool is available
function(check_required_tool TOOL_NAME TOOL_VAR)
    find_program(${TOOL_VAR} NAMES ${TOOL_NAME})
    if(${TOOL_VAR})
        message(STATUS "Found ${TOOL_NAME}: ${${TOOL_VAR}}")
    else()
        message(FATAL_ERROR "Required tool not found: ${TOOL_NAME}")
    endif()
endfunction()

# Check for required build tools
check_required_tool(cmake CMAKE_EXECUTABLE)
check_required_tool(ninja NINJA_EXECUTABLE)

# Verify generator is Ninja (preferred for this project)
if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
    message(WARNING "Ninja generator is recommended for optimal build performance. Current generator: ${CMAKE_GENERATOR}")
endif()

message(STATUS "Dependency setup helpers loaded successfully")
