cmake_minimum_required(VERSION 3.14)

include(FetchContent)

message(STATUS "Configuring external dependencies...")

set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/../.cmake_deps" CACHE PATH "FetchContent base directory")

message(STATUS "Configuring LLVM build options for minimal build time...")


# Core LLVM configuration
set(LLVM_LINK_LLVM_DYLIB ON)
set(LLVM_BUILD_LLVM_DYLIB ON)
set(LLVM_ENABLE_PROJECTS "clang" CACHE STRING "")
set(LLVM_TARGETS_TO_BUILD "" CACHE STRING "")  # Disable all backends
set(LLVM_DEFAULT_TARGET_TRIPLE "x86_64-unknown-unknown" CACHE STRING "")

# Disable unnecessary components to speed up build
set(LLVM_BUILD_TESTS OFF CACHE BOOL "")
set(LLVM_ENABLE_LIBXML2 OFF CACHE BOOL "" FORCE)
set(LLVM_BUILD_TOOLS OFF CACHE BOOL "")
set(LLVM_BUILD_EXAMPLES OFF CACHE BOOL "")
set(LLVM_INCLUDE_TESTS OFF CACHE BOOL "")
set(LLVM_INCLUDE_DOCS OFF CACHE BOOL "")
set(LLVM_BUILD_DOCS OFF CACHE BOOL "")
set(LLVM_INCLUDE_UTILS OFF CACHE BOOL "")
set(LLVM_BUILD_UTILS OFF CACHE BOOL "")
set(LLVM_INCLUDE_BENCHMARKS OFF CACHE BOOL "")
set(LLVM_BUILD_BENCHMARKS OFF CACHE BOOL "")
set(LLVM_BUILD_RUNTIME OFF CACHE BOOL "")
set(LLVM_BUILD_RUNTIMES OFF CACHE BOOL "")
set(LLVM_INCLUDE_EXAMPLES OFF CACHE BOOL "")
set(LLVM_ENABLE_BINDINGS OFF CACHE BOOL "")
set(LLVM_ENABLE_OCAMLDOC OFF CACHE BOOL "")
set(LLVM_ENABLE_Z3_SOLVER OFF CACHE BOOL "")
set(LLVM_ENABLE_ZLIB OFF CACHE BOOL "")
set(LLVM_ENABLE_ZSTD OFF CACHE BOOL "")
set(LLVM_INCLUDE_GO_TESTS OFF CACHE BOOL "")
set(LLVM_ENABLE_DOXYGEN OFF CACHE BOOL "")
set(LLVM_ENABLE_SPHINX OFF CACHE BOOL "")
set(LLVM_TOOL_LLVM_CONFIG_BUILD OFF CACHE BOOL "")
set(LLVM_TOOL_LLVM_AR_BUILD OFF CACHE BOOL "")
set(LLVM_TOOL_LLVM_AS_BUILD OFF CACHE BOOL "")
set(LLVM_TOOL_LLVM_DIS_BUILD OFF CACHE BOOL "")
set(LLVM_TOOL_LLVM_LINK_BUILD OFF CACHE BOOL "")

set(LLVM_ENABLE_RTTI ON CACHE BOOL "")

set(CLANG_TOOL_CLANG_BUILD OFF CACHE BOOL "")
set(CLANG_BUILD_EXAMPLES OFF CACHE BOOL "")
set(CLANG_BUILD_TOOLS OFF CACHE BOOL "")
set(CLANG_INCLUDE_TESTS OFF CACHE BOOL "")
set(CLANG_ENABLE_STATIC_ANALYZER OFF CACHE BOOL "")
set(CLANG_ENABLE_ARCMT OFF CACHE BOOL "")
set(CLANG_TOOL_CLANG_CHECK_BUILD OFF CACHE BOOL "")
set(CLANG_TOOL_CLANG_FORMAT_BUILD OFF CACHE BOOL "")
set(CLANG_TOOL_C_INDEX_TEST_BUILD OFF CACHE BOOL "")
set(CLANG_TOOL_CLANG_IMPORT_TEST_BUILD OFF CACHE BOOL "")
set(CLANG_TOOL_CLANG_DIFF_BUILD OFF CACHE BOOL "")
set(CLANG_TOOL_CLANG_SCAN_DEPS_BUILD OFF CACHE BOOL "")
set(CLANG_TOOL_CLANG_EXTDEF_MAPPING_BUILD OFF CACHE BOOL "")
set(CLANG_TOOL_CLANG_OFFLOAD_BUNDLER_BUILD OFF CACHE BOOL "")
set(CLANG_BUILD_LIBCLANG OFF CACHE BOOL "")
set(CLANG_TOOL_LIBCLANG_BUILD OFF CACHE BOOL "")
set(LIBCLANG_BUILD_STATIC OFF CACHE BOOL "")
set(CLANG_ENABLE_LIBCLANG OFF CACHE BOOL "")


# Fix MASM warnings on Windows
# More comprehensive MASM warning fix
if(WIN32 AND MSVC)
    # Clear all MASM flags completely
    set(CMAKE_ASM_MASM_FLAGS "" CACHE STRING "MASM flags" FORCE)
    set(CMAKE_ASM_MASM_FLAGS_DEBUG "" CACHE STRING "MASM debug flags" FORCE)
    set(CMAKE_ASM_MASM_FLAGS_RELEASE "" CACHE STRING "MASM release flags" FORCE)
    set(CMAKE_ASM_MASM_FLAGS_RELWITHDEBINFO "" CACHE STRING "MASM release with debug info flags" FORCE)
    set(CMAKE_ASM_MASM_FLAGS_MINSIZEREL "" CACHE STRING "MASM min size release flags" FORCE)
    
    # Override MASM runtime library options to prevent inheritance
    set(CMAKE_ASM_MASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreaded "" CACHE STRING "" FORCE)
    set(CMAKE_ASM_MASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDLL "" CACHE STRING "" FORCE)
    set(CMAKE_ASM_MASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDebug "" CACHE STRING "" FORCE)
    set(CMAKE_ASM_MASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDebugDLL "" CACHE STRING "" FORCE)
    
    # Set minimal MASM flags that work
    set(CMAKE_ASM_MASM_FLAGS "/nologo /W0" CACHE STRING "MASM flags" FORCE)
endif()

message(STATUS "Setting up LLVM dependency...")

FetchContent_Declare(
    LLVM
    URL https://github.com/llvm/llvm-project/releases/download/llvmorg-20.1.8/llvm-project-20.1.8.src.tar.xz
    URL_HASH SHA256=6898f963c8e938981e6c4a302e83ec5beb4630147c7311183cf61069af16333d
    SOURCE_SUBDIR llvm
)

# Make dependencies available
message(STATUS "Making LLVM available via FetchContent...")
FetchContent_MakeAvailable(LLVM)

# Verify LLVM is available
if(TARGET llvm-config)
    message(STATUS "LLVM successfully configured")
else()
    message(STATUS "LLVM libraries configured (llvm-config not built)")
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
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/3rdParty/kuzu/lib/msvc/debug/${KUZU_LIB_NAME}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/3rdParty/kuzu/include/"
)

set_target_properties(kuzu::kuzu PROPERTIES
    IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/3rdParty/kuzu/lib/msvc/debug/${KUZU_LIB_NAME}"
)
else()
set_target_properties(kuzu::kuzu PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/3rdParty/kuzu/lib/msvc/release/${KUZU_LIB_NAME}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/3rdParty/kuzu/include/"
)

set_target_properties(kuzu::kuzu PROPERTIES
    IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/3rdParty/kuzu/lib/msvc/release/${KUZU_LIB_NAME}"
)
endif()

message(STATUS "External dependencies configuration completed")
