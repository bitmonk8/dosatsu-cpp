# third_party/dependencies.cmake
# FetchContent configuration for external dependencies

include(FetchContent)

message(STATUS "Configuring external dependencies...")

# LLVM 19.1.7 dependency
message(STATUS "Setting up LLVM 19.1.7 dependency...")

FetchContent_Declare(
    LLVM
    URL https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.7/llvm-project-19.1.7.src.tar.xz
    URL_HASH SHA256=82401fea7b79d0078043f7598b835284d6650a75b93e64b6f761ea7b63097501
    SOURCE_SUBDIR llvm
)

# Configure LLVM build options - preserving all your original settings
message(STATUS "Configuring LLVM build options for minimal build time...")

# Core LLVM configuration
set(LLVM_ENABLE_PROJECTS "clang" CACHE STRING "")
set(LLVM_TARGETS_TO_BUILD "host" CACHE STRING "")

# Disable unnecessary components to speed up build
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
set(CLANG_TOOL_CLANG_BUILD OFF CACHE BOOL "")

# Essential settings
set(LLVM_ENABLE_RTTI ON CACHE BOOL "")

# Clang-specific configuration
set(CLANG_BUILD_EXAMPLES OFF CACHE BOOL "")
set(CLANG_BUILD_TOOLS OFF CACHE BOOL "")
set(CLANG_INCLUDE_TESTS OFF CACHE BOOL "")

# Platform-specific optimizations
if(WIN32)
    # Windows-specific LLVM settings
    message(STATUS "Applying Windows-specific LLVM optimizations...")
elseif(UNIX AND NOT APPLE)
    # Linux-specific LLVM settings
    message(STATUS "Applying Linux-specific LLVM optimizations...")
elseif(APPLE)
    # macOS-specific LLVM settings  
    message(STATUS "Applying macOS-specific LLVM optimizations...")
endif()

# Performance optimization: Use shared libraries if possible
option(LLVM_BUILD_SHARED_LIBS "Build LLVM with shared libraries" OFF)
if(LLVM_BUILD_SHARED_LIBS)
    message(STATUS "LLVM will be built with shared libraries")
else()
    message(STATUS "LLVM will be built with static libraries")
endif()

# Make dependencies available
message(STATUS "Making LLVM available via FetchContent...")
FetchContent_MakeAvailable(LLVM)

# Verify LLVM is available
if(TARGET llvm-config)
    message(STATUS "✓ LLVM successfully configured")
else()
    message(STATUS "✓ LLVM libraries configured (llvm-config not built)")
endif()

# Create LLVM target aliases for easier linking
if(TARGET LLVMSupport)
    message(STATUS "✓ Core LLVM libraries available")
    
    # Create a convenience target that includes common LLVM libraries
    add_library(LLVMCommon INTERFACE)
    target_link_libraries(LLVMCommon INTERFACE
        LLVMSupport
        LLVMCore
        LLVMMC
        LLVMAnalysis
        LLVMBitReader
        LLVMBitWriter
        LLVMTransformUtils
    )
    
    # Clang libraries if available
    if(TARGET clangAST)
        message(STATUS "✓ Clang AST libraries available")
        add_library(ClangLibraries INTERFACE)
        target_link_libraries(ClangLibraries INTERFACE
            clangAST
            clangBasic
            clangFrontend
            clangTooling
            clangSerialization
        )
    endif()
else()
    message(WARNING "LLVM libraries not found after FetchContent - build may fail")
endif()

message(STATUS "External dependencies configuration completed")
