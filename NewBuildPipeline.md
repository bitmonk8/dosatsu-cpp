
# Design for Modern C++ Build Pipeline

## Overall Architecture Overview

The new build system will follow a **layered architecture** where a Python orchestration script serves as the single entry point for all developer operations, delegating to specialized tools underneath:

```
┌─────────────────────────────────────┐
│ Python Build Orchestrator (build.py) │ ← Single interface for developers
├─────────────────────────────────────┤
│ CMake + FetchContent + Ninja          │ ← Core build infrastructure
├─────────────────────────────────────┤
│ Tools: clang-format, clang-tidy, git │ ← Individual specialist tools
└─────────────────────────────────────┘
```


## Proposed Project Structure

The new project directory structure will organize all artifacts cleanly while maintaining separation of concerns

```
CppGraphIndex/
├── build.py                    # Main orchestrator script
├── CMakeLists.txt             # Root CMake configuration
├── .clang-format              # Code formatting rules
├── .clang-tidy                # Static analysis rules
├── .gitignore                 # Updated to ignore artifacts/
│
├── src/                       # Main source code
│   ├── MakeIndex/
│   │   ├── CMakeLists.txt
│   │   ├── MakeIndex.cpp
│   │   ├── KuzuDump.cpp
│   │   ├── KuzuDump.h
│   │   ├── NoWarningScope_Enter.h
│   │   └── NoWarningScope_Leave.h
│
├── third_party/              # Third-party dependencies info
│   └── dependencies.cmake    # FetchContent declarations
│
├── scripts/                  # Build helper scripts
│   ├── setup_deps.cmake     # Dependency setup helpers
│   └── format_config.py     # Formatting configuration
│
├── artifacts/               # ALL BUILD ARTIFACTS GO HERE
│   ├── debug/
│   │   ├── build/          # CMake/Ninja build files
│   │   ├── bin/            # Executables
│   │   ├── lib/            # Libraries
│   │   └── logs/           # Build logs
│   ├── release/
│   │   └── (same structure)
│   ├── lint/               # clang-tidy outputs
│   ├── format/             # formatting logs
│   └── test/               # test results
│
└── .github/
    └── workflows/
        └── ci.yml
```


## Tool Responsibilities

### Python Build Orchestrator (`build.py`)

**Primary Responsibilities:**

- Single command-line interface for all developer operations
- Environment validation and setup
- Orchestrating the correct sequence of tool invocations
- Output management and logging
- Cross-platform path and command handling

**Key Features:**

- Argparse-based subcommand structure
- Rich console output with progress indicators
- Comprehensive error handling and user-friendly messages
- Artifact management and cleanup operations


### CMake

**Primary Responsibilities:**

- Build configuration and dependency management
- Cross-platform compiler detection and setup
- Target definition and linking
- Integration with FetchContent for dependency management


### FetchContent Module

**Primary Responsibilities:**

- Automated dependency fetching at configure time
- LLVM 19.1.7 source download and integration
- Dependency version and hash verification
- Clean integration with existing project targets


### Ninja

**Primary Responsibilities:**

- Fast, parallel build execution
- Efficient incremental builds
- Build artifact generation
- Integration with CMake generator


### Platform-Specific Compilers

- **Windows:** MSVC
- **Linux:** GCC
- **macOS:** Clang


## Python Script Command Interface

The `build.py` script will provide the following subcommands using argparse subparsers :

### Git Operations

```bash
python build.py git pull
python build.py git push
python build.py git commit -m "message"
python build.py git clean
```


### Build Configuration

```bash
python build.py configure [--debug|--release] [--clean]
python build.py reconfigure  # Clean configure from scratch
```


### Build Operations

```bash
python build.py build [--debug|--release] [--target TARGET] [--parallel N]
python build.py clean
python build.py rebuild  # Clean + build
```


### Testing

```bash
python build.py test [--parallel N] [--verbose]
python build.py test --target specific_test
```


### Code Quality

```bash
python build.py format [--check-only] [--files FILE1 FILE2...]
python build.py lint [--fix] [--files FILE1 FILE2...] 
python build.py lint --target specific_file.cpp
```


### Utility Commands

```bash
python build.py setup      # Initial environment setup
python build.py info       # Display build environment info
python build.py clean-all  # Clean all artifacts
```


## Artifact Management Strategy

All build outputs will be organized under the `artifacts/` directory to keep the source tree clean :

### Directory Structure

```
artifacts/
├── debug/
│   ├── build/           # CMakeCache.txt, build.ninja, etc.
│   ├── bin/            # Debug executables
│   ├── lib/            # Debug libraries  
│   └── logs/           # Build logs
├── release/
│   └── (same structure as debug)
├── lint/
│   ├── clang-tidy-output.txt
│   ├── compile_commands.json
│   └── per-file-reports/
├── format/
│   └── format-check.log
└── test/
    ├── results.xml     # JUnit-style test results
    └── coverage/       # Coverage reports if enabled
```


### Logging Strategy

- **Console Output:** Real-time progress and essential information
- **Persistent Logs:** Detailed command outputs saved to artifact subdirectories
- **Structured Naming:** Timestamp-based log files for historical tracking
- **Error Preservation:** Failed command outputs always saved for debugging


## CMake Configuration Design

### Root CMakeLists.txt Structure

```cmake
cmake_minimum_required(VERSION 3.24)

project(CppGraphIndex 
    VERSION 1.0.0
    DESCRIPTION "C++ Graph Index with LLVM Integration"
    LANGUAGES CXX
)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure artifact directories
set(ARTIFACTS_BASE_DIR "${CMAKE_SOURCE_DIR}/artifacts")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${ARTIFACTS_BASE_DIR}/${CMAKE_BUILD_TYPE}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${ARTIFACTS_BASE_DIR}/${CMAKE_BUILD_TYPE}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ARTIFACTS_BASE_DIR}/${CMAKE_BUILD_TYPE}/lib")

# Platform and toolchain validation
include(scripts/setup_deps.cmake)

# Dependencies
include(third_party/dependencies.cmake)

# Configure clang-tidy integration
find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
if(CLANG_TIDY_EXECUTABLE)
    set(CMAKE_CXX_CLANG_TIDY 
        ${CLANG_TIDY_EXECUTABLE};
        --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
    )
endif()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)
```


### FetchContent Dependencies Configuration

```cmake
# third_party/dependencies.cmake
include(FetchContent)

# LLVM 19.1.7 dependency
FetchContent_Declare(
    LLVM
    URL https://github.com/llvm/llvm-project/releases/download/llvmorg-19.1.7/llvm-project-19.1.7.src.tar.xz
    URL_HASH SHA256=82401fea7b79d0078043f7598b835284d6650a75b93e64b6f761ea7b63097501
)

# Configure LLVM build options - preserving all your original settings
set(LLVM_ENABLE_PROJECTS "clang" CACHE STRING "")
set(LLVM_DISTRIBUTION_COMPONENTS "clang-libraries;LLVMSupport;LLVMCore;LLVMMC;LLVMOption;LLVMTargetParser;LLVMBinaryFormat;LLVMAnalysis;LLVMBitReader;LLVMBitWriter;LLVMTransformUtils;LLVMInstCombine;LLVMScalarOpts;LLVMipo;LLVMInstrumentation;LLVMVectorize;LLVMObjCARCOpts;LLVMCoroutines;LLVMCFGuard;LLVMLinker;LLVMIRReader;LLVMFrontendOpenMP;LLVMPasses;LLVMProfileData;LLVMObject;LLVMDebugInfoDWARF;LLVMDebugInfoCodeView;LLVMDebugInfoMSF;LLVMDebugInfoPDB;LLVMDebugInfoBTF;LLVMRemarks;LLVMTextAPI;LLVMWindowsDriver;LLVMDemangle;LLVMBitstreamReader;LLVMAsmParser;LLVMMCParser;LLVMSymbolize;LLVMAggressiveInstCombine;LLVMCodeGen;LLVMHipStdPar;LLVMIRPrinter;LLVMTarget;LLVMFrontendOffloading;LLVMSelectionDAG;LLVMCodeGenTypes;LLVMCoverage;LLVMExtensions;LLVMFrontendDriver;LLVMFrontendHLSL;LLVMLTO;LLVMTextAPIBinaryReader;LLVMOrcJIT;LLVMOrcDebugging;LLVMOrcShared;LLVMOrcTargetProcess;LLVMExecutionEngine;LLVMMCJIT;LLVMRuntimeDyld;LLVMJITLink;LLVMX86CodeGen;LLVMX86AsmParser;LLVMX86Desc;LLVMX86Disassembler;LLVMX86Info;LLVMAsmPrinter;LLVMGlobalISel;LLVMMCDisassembler" CACHE STRING "")
set(LLVM_BUILD_TOOLS OFF CACHE BOOL "")
set(LLVM_BUILD_EXAMPLES OFF CACHE BOOL "")
set(LLVM_INCLUDE_TESTS OFF CACHE BOOL "")
set(LLVM_INCLUDE_DOCS OFF CACHE BOOL "")
set(LLVM_BUILD_DOCS OFF CACHE BOOL "")
set(LLVM_INCLUDE_UTILS OFF CACHE BOOL "")
set(LLVM_BUILD_UTILS OFF CACHE BOOL "")
set(LLVM_INCLUDE_BENCHMARKS OFF CACHE BOOL "")
set(LLVM_BUILD_BENCHMARKS OFF CACHE BOOL "")
set(LLVM_TARGETS_TO_BUILD "host" CACHE STRING "")
set(CLANG_BUILD_EXAMPLES OFF CACHE BOOL "")
set(CLANG_BUILD_TOOLS OFF CACHE BOOL "")
set(CLANG_INCLUDE_TESTS OFF CACHE BOOL "")
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
set(LLVM_ENABLE_RTTI ON CACHE BOOL "")

# Make dependencies available
FetchContent_MakeAvailable(LLVM)
```


## GitHub Actions CI/CD Workflow

### Workflow Configuration

```yaml
name: CI/CD Pipeline

on: [push, pull_request]

jobs:
  build-and-test:
    strategy:
      matrix:
        os: [windows-latest, ubuntu-latest, macos-latest]
        build_type: [Debug, Release]
        include:
          - os: windows-latest
            compiler: msvc
          - os: ubuntu-latest  
            compiler: gcc
          - os: macos-latest
            compiler: clang

    runs-on: ${{ matrix.os }}
    
    steps:
    - uses: actions/checkout@v4
    
    - name: Setup Python
      uses: actions/setup-python@v4
      with:
        python-version: '3.11'
        
    - name: Setup CMake and Ninja
      uses: lukka/get-cmake@latest
        
    - name: Setup MSVC Environment (Windows)
      if: matrix.os == 'windows-latest'
      uses: ilammy/msvc-dev-cmd@v1
      
    - name: Configure Build
      run: python build.py configure --${{ matrix.build_type }}
      
    - name: Build Project  
      run: python build.py build --${{ matrix.build_type }}
      
    - name: Run Tests
      run: python build.py test
      
    - name: Code Quality Checks
      run: |
        python build.py format --check-only
        python build.py lint
        
```


## Implementation Plan

### Phase 1: Foundation Setup

1. **Project Structure Migration**
    - Create new directory structure
    - Move existing source files to appropriate locations
    - Update .gitignore to exclude artifacts/ directory
2. **Python Build Orchestrator**
    - Implement basic `build.py` with argparse structure
    - Add subcommands for configure, build, clean operations
    - Implement cross-platform path handling and command execution
    - Add comprehensive logging and output management
3. **Basic CMake Configuration**
    - Create root CMakeLists.txt with modern CMake practices
    - Set up artifact directory structure
    - Implement basic target for MakeIndex executable

### Phase 2: Dependency Management

4. **FetchContent Integration**
    - Implement LLVM FetchContent declaration
    - Configure LLVM build options for minimal build time
    - Test dependency resolution
    - Add proper error handling for build issues
5. **Ninja Integration**
    - Configure CMake Ninja generator
    - Optimize build parallelization settings
    - Test build performance optimization

### Phase 3: Code Quality Tools

6. **clang-format Integration**
    - Port existing formatting configuration
    - Integrate formatting into build script
    - Add pre-commit formatting checks
7. **clang-tidy Integration**
    - Port and enhance existing linting configuration
    - Implement compilation database generation
    - Add proper output filtering and reporting

### Phase 4: Testing and CI

8. **Testing Framework**
    - Set up unit testing with your existing test structure
    - Integrate test execution into build script
    - Add test result reporting and artifact generation
9. **GitHub Actions Setup**
    - Create multi-platform CI workflow
    - Test all build combinations (Debug/Release on Windows)
    - Configure artifact collection and deployment

### Phase 5: Advanced Features

10. **Git Integration**
    - Implement git command wrappers in build script
    - Add repository state validation
    - Implement clean working directory checks
11. **Performance Optimization**
    - Fine-tune dependency caching strategies
    - Optimize incremental build performance
    - Add build time reporting and analysis

### Phase 6: Documentation and Polish

12. **Documentation**
    - Update README with new build instructions
    - Create developer workflow documentation
    - Add troubleshooting guides for common issues

## Implementation Strategy

- **Incremental Implementation:** Phases implemented with validation points
- **Comprehensive Testing:** Full validation completed across all platforms

