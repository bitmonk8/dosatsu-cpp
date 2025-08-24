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
    KuzuDatabase.cpp
    KuzuDatabase.h
    ASTNodeProcessor.cpp
    ASTNodeProcessor.h
    ScopeManager.cpp
    ScopeManager.h
    TypeAnalyzer.cpp
    TypeAnalyzer.h
    DeclarationAnalyzer.cpp
    DeclarationAnalyzer.h
    StatementAnalyzer.cpp
    StatementAnalyzer.h
    TemplateAnalyzer.cpp
    TemplateAnalyzer.h
    CommentProcessor.cpp
    CommentProcessor.h
    AdvancedAnalyzer.cpp
    AdvancedAnalyzer.h
    GlobalDatabaseManager.cpp
    GlobalDatabaseManager.h
    NoWarningScope_Enter.h
    NoWarningScope_Leave.h
)

target_link_directories(MakeIndex PRIVATE ${LLVM_BIN_DIR}/lib)

# Copy KuzuDB DLL to output directory on Windows
if(WIN32)
    # Define the DLL name for Windows
    set(KUZU_DLL_NAME "kuzu_shared.dll")
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(TARGET MakeIndex POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/third_party/kuzu/lib/msvc/debug/${KUZU_DLL_NAME}"
                "$<TARGET_FILE_DIR:MakeIndex>"
            COMMENT "Copying KuzuDB DLL to executable directory"
        )
        
        # Verify the DLL exists before copying
        if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/kuzu/lib/msvc/debug/${KUZU_DLL_NAME}")
            message(WARNING "KuzuDB DLL not found at: ${CMAKE_SOURCE_DIR}/third_party/kuzu/lib/msvc/debug/${KUZU_DLL_NAME}")
        endif()
    else()
        add_custom_command(TARGET MakeIndex POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/third_party/kuzu/lib/msvc/release/${KUZU_DLL_NAME}"
                "$<TARGET_FILE_DIR:MakeIndex>"
            COMMENT "Copying KuzuDB DLL to executable directory"
        )

        # Verify the DLL exists before copying
        if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/kuzu/lib/msvc/release/${KUZU_DLL_NAME}")
            message(WARNING "KuzuDB DLL not found at: ${CMAKE_SOURCE_DIR}/third_party/kuzu/lib/msvc/release/${KUZU_DLL_NAME}")
        endif()
    endif()
endif()


# Set target properties
set_target_properties(MakeIndex PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories
target_include_directories(MakeIndex PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/include
    ${LLVM_HEADERS_DIR}
)

target_link_libraries(MakeIndex PRIVATE kuzu::kuzu)

# Platform-specific compiler flags
if(WIN32 AND MSVC)
    target_compile_options(MakeIndex PRIVATE
        /wd4146  # Suppress unsigned minus warning from LLVM
    )
endif()

target_link_libraries(MakeIndex PRIVATE
    clangFrontend
    clangTooling
    clangDriver
    clangSerialization
    clangCodeGen
    clangSema
    clangParse
    clangAST
    clangASTMatchers
    clangAnalysis
    clangStaticAnalyzerCore
    clangEdit
    clangRewrite
    clangLex
    clangAPINotes
    clangBasic
    clangSupport

    LLVMFrontendOpenMP
    LLVMFrontendAtomic
    LLVMSupport
    LLVMCodeGen
    LLVMCore
    LLVMMC
    LLVMMCParser
    LLVMAnalysis
    LLVMBitstreamReader
    LLVMRemarks
    LLVMDemangle
    LLVMDebugInfoDWARF
    LLVMOption
    LLVMProfileData
    LLVMTargetParser
    LLVMObject
    LLVMBinaryFormat
    LLVMTextAPI
    LLVMBitReader
    LLVMBitWriter
    LLVMTransformUtils
    LLVMWindowsDriver
    LLVMScalarOpts
    LLVMInstCombine
    LLVMipo
    LLVMAsmParser
    LLVMIRReader
    LLVMCodeGenTypes
    LLVMFrontendOffloading
    LLVMAsmPrinter
    LLVMSelectionDAG
    LLVMGlobalISel
)

if(WIN32)
    target_link_libraries(MakeIndex PRIVATE
        version.lib
        ntdll.lib
        ole32.lib
        oleaut32.lib
    )
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
