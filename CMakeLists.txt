cmake_minimum_required(VERSION 3.24)

project(CppGraphIndex 
    VERSION 1.0.0
    DESCRIPTION "C++ Graph Index with LLVM Integration"
    LANGUAGES CXX
)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configure build types
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Configure artifact directories
set(ARTIFACTS_BASE_DIR "${CMAKE_SOURCE_DIR}/artifacts")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ARTIFACTS_BASE_DIR}/${CMAKE_BUILD_TYPE}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ARTIFACTS_BASE_DIR}/${CMAKE_BUILD_TYPE}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ARTIFACTS_BASE_DIR}/${CMAKE_BUILD_TYPE}/lib")

# Platform and toolchain validation
message(STATUS "Build system: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Generator: ${CMAKE_GENERATOR}")

# Platform-specific compiler settings
if(WIN32)
    if(MSVC)
        message(STATUS "✓ Windows platform with MSVC detected")
        # MSVC-specific flags
        add_compile_options(/W4)  # Warning level 4
        add_compile_options(/wd4146)  # Disable specific warning from LLVM
        # Runtime library selection
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
        else()
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
        endif()
    else()
        message(FATAL_ERROR "On Windows, only MSVC is supported")
    endif()
elseif(UNIX AND NOT APPLE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "✓ Linux platform with GCC detected")
        add_compile_options(-Wall -Wextra -Wpedantic)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(STATUS "✓ Linux platform with Clang detected")
        add_compile_options(-Wall -Wextra -Wpedantic)
    else()
        message(FATAL_ERROR "On Linux, only GCC or Clang are supported")
    endif()
elseif(APPLE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(STATUS "✓ macOS platform with Clang detected")
        add_compile_options(-Wall -Wextra -Wpedantic)
    else()
        message(FATAL_ERROR "On macOS, only Clang is supported")
    endif()
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# Ensure artifacts directories exist
file(MAKE_DIRECTORY "${ARTIFACTS_BASE_DIR}/${CMAKE_BUILD_TYPE}/bin")
file(MAKE_DIRECTORY "${ARTIFACTS_BASE_DIR}/${CMAKE_BUILD_TYPE}/lib")
file(MAKE_DIRECTORY "${ARTIFACTS_BASE_DIR}/${CMAKE_BUILD_TYPE}/logs")

# Include helper scripts
if(EXISTS "${CMAKE_SOURCE_DIR}/scripts/setup_deps.cmake")
    include(scripts/setup_deps.cmake)
endif()

# Dependencies - will be implemented in Phase 2
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/dependencies.cmake")
    include(third_party/dependencies.cmake)
else()
    message(STATUS "Dependencies configuration not found - will be created in Phase 2")
endif()

# Configure clang-tidy integration if available and explicitly enabled
option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)
find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
if(CLANG_TIDY_EXECUTABLE AND ENABLE_CLANG_TIDY)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXECUTABLE}")
    if(EXISTS "${CMAKE_SOURCE_DIR}/.clang-tidy")
        set(CMAKE_CXX_CLANG_TIDY 
            ${CLANG_TIDY_EXECUTABLE};
            --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
        )
        message(STATUS "Enabled clang-tidy with project configuration")
    else()
        message(STATUS "clang-tidy found but no .clang-tidy config file - will be configured in Phase 3")
    endif()
else()
    if(NOT CLANG_TIDY_EXECUTABLE)
        message(STATUS "clang-tidy not found - static analysis will be unavailable")
    else()
        message(STATUS "clang-tidy found but disabled (use -DENABLE_CLANG_TIDY=ON to enable)")
    endif()
endif()

# Configure compilation database for tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable testing support  
include(CTest)
enable_testing()

# Add subdirectories
message(STATUS "Adding source directories...")
if(EXISTS "${CMAKE_SOURCE_DIR}/MakeIndex")
    add_subdirectory(MakeIndex)
else()
    message(FATAL_ERROR "Source directory not found: ${CMAKE_SOURCE_DIR}/MakeIndex")
endif()

# Tests will be added in Phase 4
if(EXISTS "${CMAKE_SOURCE_DIR}/tests")
    message(STATUS "Adding tests directory...")
    add_subdirectory(tests)
    
    # Enable testing
    enable_testing()
else()
    message(STATUS "Tests directory not found - will be created in Phase 4")
endif()

# Display configuration summary
message(STATUS "=====================================")
message(STATUS "CppGraphIndex Build Configuration")
message(STATUS "=====================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Runtime output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Library output: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
if(CLANG_TIDY_EXECUTABLE AND ENABLE_CLANG_TIDY)
    message(STATUS "Static analysis: Enabled (clang-tidy)")
else()
    message(STATUS "Static analysis: Disabled")
endif()
message(STATUS "=====================================")
