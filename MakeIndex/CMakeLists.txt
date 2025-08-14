# MakeIndex executable target

# Define the target
add_executable(MakeIndex
    MakeIndex.cpp
    KuzuDump.cpp
    KuzuDump.h
    CompilationDatabaseLoader.cpp
    CompilationDatabaseLoader.h
    ASTDumpAction.cpp
    ASTDumpAction.h
    NoWarningScope_Enter.h
    NoWarningScope_Leave.h
)

# Set target properties
set_target_properties(MakeIndex PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories
target_include_directories(MakeIndex PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/3rdParty/include  # For doctest
)

target_link_libraries(MakeIndex PRIVATE kuzu::kuzu)

# Add LLVM include directories manually if linking doesn't work
if(TARGET LLVMSupport)
    # Add the main LLVM include directories
    target_include_directories(MakeIndex PRIVATE 
        "${CMAKE_BINARY_DIR}/_deps/llvm-src/llvm/include"
        "${CMAKE_BINARY_DIR}/_deps/llvm-build/include"
        "${CMAKE_BINARY_DIR}/_deps/llvm-src/clang/include"
        "${CMAKE_BINARY_DIR}/_deps/llvm-build/tools/clang/include"
    )
    message(STATUS "Added LLVM/Clang include directories manually")
endif()

# Platform-specific compiler flags
if(WIN32 AND MSVC)
    target_compile_options(MakeIndex PRIVATE
        /wd4146  # Suppress unsigned minus warning from LLVM
    )
endif()

# Link against LLVM libraries
if(TARGET ClangLibraries)
    target_link_libraries(MakeIndex PRIVATE ClangLibraries LLVMCommon)
    message(STATUS "Linking MakeIndex against ClangLibraries and LLVMCommon")
else()
    # Fallback to individual libraries if convenience targets aren't available
    message(STATUS "Using individual LLVM libraries")
    target_link_libraries(MakeIndex PRIVATE
        clangAST
        clangBasic
        clangFrontend
        clangTooling
        clangSerialization
        LLVMSupport
        LLVMCore
        LLVMMC
        LLVMAnalysis
    )
    message(STATUS "Linking MakeIndex against individual LLVM libraries")
endif()

message(STATUS "Configured MakeIndex executable target")

# Test configuration with doctest framework
# Enable testing support
include(CTest)
enable_testing()

if(BUILD_TESTING)
    # Main self-test using doctest framework
    add_test(NAME MakeIndex_SelfTest
        COMMAND MakeIndex --selftest
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
    
    # Configure test properties
    set_tests_properties(MakeIndex_SelfTest PROPERTIES
        TIMEOUT 60
        LABELS "unit;selftest"
        ENVIRONMENT "DOCTEST_CONFIG_NO_COLORS=1"
    )
    
    # Additional test configurations for different verbosity levels
    add_test(NAME MakeIndex_SelfTest_Verbose
        COMMAND MakeIndex --selftest --verbose
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
    
    set_tests_properties(MakeIndex_SelfTest_Verbose PROPERTIES
        TIMEOUT 60
        LABELS "unit;selftest;verbose"
        ENVIRONMENT "DOCTEST_CONFIG_NO_COLORS=1"
    )
    
    # Basic functionality test (no doctest, just ensure binary runs)
    add_test(NAME MakeIndex_BasicRun
        COMMAND MakeIndex --help
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
    
    set_tests_properties(MakeIndex_BasicRun PROPERTIES
        TIMEOUT 10
        LABELS "smoke;basic"
    )
endif()

# Test result reporting configuration
set(CTEST_OUTPUT_ON_FAILURE ON)
set(CTEST_PARALLEL_LEVEL 4)
