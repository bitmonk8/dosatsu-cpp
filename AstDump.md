# AST Dump Feature Design

## Overview

This document outlines the design for adding compile_commands.json support to MakeIndex, enabling AST parsing and dumping functionality using the existing KuzuDump class.

## Problem Statement

The current MakeIndex application is a minimal placeholder that only runs self-tests. We need to extend it to:
1. Read and parse `compile_commands.json` files (similar to how clangd works)
2. Parse source files to generate Abstract Syntax Trees (ASTs) 
3. Use the existing KuzuDump class to output AST information

## Research Summary

### Key Technologies Identified

**Clang Tooling APIs:**
- `clang::tooling::JSONCompilationDatabase` - For reading compile_commands.json files
- `clang::tooling::ClangTool` - For running frontend actions on multiple files
- `clang::ASTFrontendAction` - Base class for AST-based tools
- `clang::ASTConsumer` - Interface for processing ASTs

**Existing Infrastructure:**
- `KuzuDump` class already extends `ASTNodeTraverser<KuzuDump, TextNodeDumper>`
- Build system already links against required Clang libraries
- LLVM/Clang source available at `artifacts/debug/build/_deps/llvm-src`

## Architecture Design

### High-Level Flow

```
compile_commands.json → JSONCompilationDatabase → ClangTool → ASTDumpAction → ASTConsumer → KuzuDump
```

### Component Design

#### 1. Command Line Interface
```cmd
MakeIndex.exe [options] <compile_commands.json>

Options:
  --output <file>     Output file (default: stdout)
  --filter <pattern>  Filter files by pattern (e.g., "*MakeIndex*")

Examples:
  MakeIndex.exe artifacts\debug\build\compile_commands.json
  MakeIndex.exe --filter="*MakeIndex*" artifacts\debug\build\compile_commands.json
  MakeIndex.exe --output=self_analysis.txt artifacts\debug\build\compile_commands.json
```

#### 2. ASTDumpAction Class
A custom `ASTFrontendAction` that creates our AST consumer:

```cpp
class ASTDumpAction : public clang::ASTFrontendAction {
public:
    ASTDumpAction(llvm::raw_ostream& OS, bool ShowColors)
        : OS(OS), ShowColors(ShowColors) {}

protected:
    std::unique_ptr<clang::ASTConsumer> 
    CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef InFile) override;

private:
    llvm::raw_ostream& OS;
    bool ShowColors;
};
```

#### 3. ASTDumpConsumer Class
A custom `ASTConsumer` that uses KuzuDump:

```cpp
class ASTDumpConsumer : public clang::ASTConsumer {
public:
    ASTDumpConsumer(llvm::raw_ostream& OS, clang::ASTContext& Context, bool ShowColors)
        : Dumper(OS, Context, ShowColors) {}

    void HandleTranslationUnit(clang::ASTContext &Context) override {
        Dumper.Visit(Context.getTranslationUnitDecl());
    }

private:
    clang::KuzuDump Dumper;
};
```

#### 4. CompilationDatabaseLoader Class
Handles loading and validating compile_commands.json:

```cpp
class CompilationDatabaseLoader {
public:
    static std::unique_ptr<clang::tooling::CompilationDatabase> 
    loadFromFile(const std::string& path, std::string& errorMessage);
    
    static std::vector<std::string> 
    filterSourceFiles(const clang::tooling::CompilationDatabase& db, 
                     const std::string& pattern = "");
};
```

#### 5. Main Application Flow

```cpp
int RealMain(int argc, char** argv) {
    // 1. Parse command line arguments
    // 2. Load compilation database from compile_commands.json
    // 3. Filter source files if needed
    // 4. Create ClangTool with database and files
    // 5. Run ASTDumpAction on all files
    // 6. Handle errors and cleanup
}
```

### Error Handling Strategy

1. **File Access Errors**: Check file existence and permissions
2. **JSON Parse Errors**: Provide clear error messages for malformed compile_commands.json
3. **Compilation Errors**: Continue processing other files, report errors per file
4. **Memory Constraints**: Process files in batches if needed

### Integration with Existing Code

#### KuzuDump Integration
The existing `KuzuDump` class requires minimal changes:
- Already extends the correct base classes
- Constructor accepts `raw_ostream` and `ASTContext`
- Methods like `Visit()` can be called directly

#### Build System Integration
No changes needed to build system:
- Required Clang libraries already linked
- Headers already included in CMakeLists.txt

## Self-Testing Strategy

### Using MakeIndex's Own compile_commands.json

This implementation uses a **self-testing approach** where MakeIndex analyzes its own source code and the entire LLVM/Clang codebase it's built with. This provides several advantages:

#### **Real-World Test Data**
- **6MB+ compile_commands.json** with thousands of files
- **Complex build configuration** with MSVC and Clang flags
- **Comprehensive source coverage**: MakeIndex sources + full LLVM/Clang codebase
- **Header hierarchies**: Complex include relationships from LLVM

#### **Available Test Files**
From the existing `artifacts\debug\build\compile_commands.json`:
- `MakeIndex\MakeIndex.cpp` - Main application (self-analysis)
- `MakeIndex\KuzuDump.cpp` - AST dumper implementation  
- Thousands of LLVM/Clang source files (Demangle, Support, AST, etc.)

#### **Testing Benefits**
- **No setup required**: compile_commands.json already exists
- **Performance testing**: Large codebase provides realistic load testing
- **Edge case coverage**: LLVM codebase exercises complex C++ features
- **Self-validation**: MakeIndex can verify its own AST structure

#### **Development Workflow**
```cmd
rem Build MakeIndex
python build.py build

rem Test with own sources only  
artifacts\debug\bin\MakeIndex.exe --filter="*MakeIndex*" artifacts\debug\build\compile_commands.json

rem Test with full codebase
artifacts\debug\bin\MakeIndex.exe artifacts\debug\build\compile_commands.json
```

## Implementation Plan

### Step 1: Command Line Interface
**Goal**: Basic argument parsing and help system
**Files to modify**: `MakeIndex/MakeIndex.cpp`
**Dependencies**: LLVM CommandLine library

**Tasks**:
1. Replace placeholder `RealMain()` with command line parsing
2. Add options for:
   - Input compile_commands.json file (required)
   - Output file (optional, default stdout)
   - Color output (optional, default auto-detect)
   - Help and version information
3. Validate required arguments
4. Test: `MakeIndex --help` and `MakeIndex --version`

**Verification**: 
```cmd
python build.py build && python build.py test
artifacts\debug\bin\MakeIndex.exe --help
```

### Step 2: CompilationDatabase Loading
**Goal**: Load and validate compile_commands.json files
**Files to create**: 
- `MakeIndex/CompilationDatabaseLoader.h`
- `MakeIndex/CompilationDatabaseLoader.cpp`
**Files to modify**: `MakeIndex/CMakeLists.txt`, `MakeIndex/MakeIndex.cpp`

**Tasks**:
1. Create `CompilationDatabaseLoader` class with static methods
2. Implement JSON loading using `clang::tooling::JSONCompilationDatabase`
3. Add comprehensive error handling for:
   - File not found
   - JSON parse errors
   - Empty or invalid compilation database
4. Test with MakeIndex's own compile_commands.json (6MB+ with LLVM sources)

**Verification**:
```cmd
rem Use MakeIndex's own compile_commands.json for testing
artifacts\debug\bin\MakeIndex.exe artifacts\debug\build\compile_commands.json
```

### Step 3: Basic AST Action Framework
**Goal**: Create minimal AST processing pipeline
**Files to create**:
- `MakeIndex/ASTDumpAction.h`
- `MakeIndex/ASTDumpAction.cpp`
**Files to modify**: `MakeIndex/CMakeLists.txt`, `MakeIndex/MakeIndex.cpp`

**Tasks**:
1. Implement `ASTDumpAction` class extending `ASTFrontendAction`
2. Implement `ASTDumpConsumer` class extending `ASTConsumer`
3. Integrate with existing `KuzuDump` class in `HandleTranslationUnit()`
4. Test with single source file from compilation database

**Verification**:
```cmd
rem Test with MakeIndex's own source files
artifacts\debug\bin\MakeIndex.exe artifacts\debug\build\compile_commands.json
rem Should process MakeIndex.cpp and KuzuDump.cpp among others
```

### Step 4: ClangTool Integration
**Goal**: Process multiple files from compilation database
**Files to modify**: `MakeIndex/MakeIndex.cpp`

**Tasks**:
1. Create `ClangTool` instance with loaded compilation database
2. Get list of source files from database
3. Run `ASTDumpAction` on all files using `ClangTool::run()`
4. Add basic error reporting per file

**Verification**:
```cmd
rem Test with all files from MakeIndex project (includes LLVM sources)
artifacts\debug\bin\MakeIndex.exe artifacts\debug\build\compile_commands.json
rem Should process thousands of LLVM/Clang source files
```

### Step 5: Basic Error Handling
**Goal**: Handle basic error cases gracefully
**Files to modify**: All implementation files

**Tasks**:
1. Basic error messages for common failures
2. Continue processing other files when one fails
3. Validate file paths exist and are readable

**Verification**:
```cmd
rem Test with MakeIndex's compile_commands.json
artifacts\debug\bin\MakeIndex.exe artifacts\debug\build\compile_commands.json
```

### Step 6: Basic Output Options
**Goal**: Essential output functionality
**Files to modify**: `MakeIndex/MakeIndex.cpp`

**Tasks**:
1. Support output to file instead of stdout
2. Basic file filtering by pattern

**Verification**:
```cmd
artifacts\debug\bin\MakeIndex.exe --output=ast_dump.txt artifacts\debug\build\compile_commands.json
artifacts\debug\bin\MakeIndex.exe --filter="*MakeIndex*" artifacts\debug\build\compile_commands.json
```

### Step 7: Basic Testing
**Goal**: Verify functionality works
**Files to modify**: `MakeIndex/MakeIndex.cpp` (add basic tests)

**Tasks**:
1. Basic unit tests using existing doctest framework
2. Verify works with MakeIndex's compile_commands.json

**Verification**:
```cmd
python build.py test
artifacts\debug\bin\MakeIndex.exe --selftest
```



## Incremental Development Strategy

Each step builds on the previous ones and can be verified independently:

1. **Steps 1-4**: Core functionality (CLI, DB loading, AST processing)
2. **Steps 5-7**: Basic error handling and minimal polish (optional)

## Risk Mitigation Per Step

- **Start Small**: Begin with `--filter="*MakeIndex*"` (2 files only)
- **Build Incrementally**: Get basic functionality working first
- **Validate Early**: Test with real compile_commands.json after each step

## Testing Strategy

### Essential Tests
- Basic functionality with MakeIndex's own `compile_commands.json`
- Self-analysis: MakeIndex analyzing its own source code (`--filter="*MakeIndex*"`)
- Command line argument parsing
- Basic error handling for file not found



## Future Enhancements

After gaining experience with the core functionality, potential enhancements include:

### Possible Future Features
1. **JSON Output Format**: Alternative output format for programmatic consumption
2. **Configuration Files**: Support for .clang-ast-dump config files
3. **Include/Exclude Headers**: More sophisticated file filtering
4. **Incremental Processing**: Only process changed files
5. **Database Output**: Store AST information in Kuzu database
6. **Interactive Mode**: Query AST information interactively
7. **Language Server Integration**: Provide AST information via LSP

## Risk Assessment

### Technical Risks
- **Basic functionality**: AST parsing and dumping might fail
- **File path issues**: Windows paths might cause problems

### Mitigation Strategies
- **Start with MakeIndex files only**: Use `--filter="*MakeIndex*"` 
- **Test incrementally**: Get 2 files working before testing more

## Success Criteria

1. **Core Functionality**: Parse MakeIndex's compile_commands.json and generate AST dumps
2. **Self-Analysis**: Generate AST dumps of MakeIndex's own source code
3. **Basic CLI**: Simple command line interface that works
4. **KuzuDump Integration**: Use existing KuzuDump functionality

## Appendix: API Reference

### Key Clang APIs Used

```cpp
// Loading compilation database
clang::tooling::JSONCompilationDatabase::loadFromFile()

// Running tools
clang::tooling::ClangTool(CompilationDatabase&, ArrayRef<string>)
ClangTool::run(FrontendActionFactory*)

// AST processing
clang::ASTFrontendAction::CreateASTConsumer()
clang::ASTConsumer::HandleTranslationUnit()

// AST dumping (existing)
clang::KuzuDump::Visit(Decl*)
```

### Example compile_commands.json
```json
[
  {
    "directory": "C:/path/to/project",
    "command": "clang++ -IC:/Program Files/LLVM/include -std=c++17 -c src/main.cpp -o build/main.o",
    "file": "src/main.cpp"
  },
  {
    "directory": "C:/path/to/project", 
    "arguments": ["clang++", "-IC:/Program Files/LLVM/include", "-std=c++17", "-c", "src/utils.cpp"],
    "file": "src/utils.cpp"
  }
]
```

This design provides a solid foundation for implementing AST dump functionality while leveraging existing infrastructure and following established patterns from the Clang ecosystem.
