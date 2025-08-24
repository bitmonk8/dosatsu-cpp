cmake_minimum_required(VERSION 3.14)

include(FetchContent)

message(STATUS "Configuring external dependencies...")

set(LLVM_HEADERS_ARCHIVE "${CMAKE_SOURCE_DIR}/third_party/llvm-clang-20.1.8-headers.zip")
set(LLVM_HEADERS_DIR "${ARTIFACTS_BASE_DIR}/llvm-include")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
set(LLVM_BIN_ARCHIVE "${CMAKE_SOURCE_DIR}/third_party/llvm-clang-20.1.8-windows-debug.zip")
else()
set(LLVM_BIN_ARCHIVE "${CMAKE_SOURCE_DIR}/third_party/llvm-clang-20.1.8-windows-release.zip")
endif()
set(LLVM_BIN_DIR "${ARTIFACTS_BASE_DIR}/${CMAKE_BUILD_TYPE}/llvm-bin")
    
if(NOT EXISTS "${LLVM_HEADERS_DIR}")
    file(ARCHIVE_EXTRACT INPUT ${LLVM_HEADERS_ARCHIVE} DESTINATION ${LLVM_HEADERS_DIR})
endif()

if(NOT EXISTS "${LLVM_BIN_DIR}")
    file(ARCHIVE_EXTRACT INPUT ${LLVM_BIN_ARCHIVE} DESTINATION ${LLVM_BIN_DIR})
endif()

# Determine the correct KuzuDB release based on platform
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(KUZU_DLL_NAME "libkuzu.so")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(KUZU_LIB_NAME "kuzu_shared.lib")
    set(KUZU_DLL_NAME "kuzu_shared.dll")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(KUZU_LIB_NAME "libkuzu.dylib")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# Create an imported target for KuzuDB
add_library(kuzu::kuzu SHARED IMPORTED)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
set_target_properties(kuzu::kuzu PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/kuzu/lib/msvc/debug/${KUZU_LIB_NAME}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/kuzu/include/"
)

set_target_properties(kuzu::kuzu PROPERTIES
    IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/third_party/kuzu/lib/msvc/debug/${KUZU_LIB_NAME}"
)
else()
set_target_properties(kuzu::kuzu PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/kuzu/lib/msvc/release/${KUZU_LIB_NAME}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/kuzu/include/"
)

set_target_properties(kuzu::kuzu PROPERTIES
    IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/third_party/kuzu/lib/msvc/release/${KUZU_LIB_NAME}"
)
endif()

message(STATUS "External dependencies configuration completed")
