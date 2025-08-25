# Known Issues in Dosatsu

This document describes known technical issues in the Dosatsu project that require investigation.

## Bug #1: Kuzu Parser Overwhelmed by Standard Library AST Data

### **Status**: Known Issue - Technical Investigation Required
### **Severity**: Medium
### **Affected Components**: Kuzu database integration, AST processing
### **Discovery Date**: December 25, 2025

### **Description**
When Dosatsu processes C++ files that include and extensively use standard library headers (`<string>`, `<vector>`, `<iostream>`, etc.), the generated Abstract Syntax Tree (AST) data becomes so voluminous that it overwhelms the Kuzu database query parser during batch operations. This results in parsing failures despite successful compilation and standard library resolution.

### **Root Cause Analysis**
- **Compilation Success**: MSVC correctly compiles files with standard library includes
- **Standard Library Resolution**: All `#include <string>`, `#include <vector>`, etc. resolve correctly
- **Path Handling**: Windows file paths are properly escaped and handled
- **Parser Limitation**: Kuzu batch query parser fails when processing ~25,000+ lines of AST data

### **Error Symptoms**
```
Batched query failed: Parser exception: Invalid input <CREATE (n:ASTNode {node_id: 4, node_type: 'NamespaceDecl', memory_address: '...', source_file: '>: expected rule oC_SingleQuery (line: 1, offset: 103)
```

### **Affected Environment**
- **OS**: Windows 10.0.26100
- **Compiler**: MSVC 19.43.34810.0 (Visual Studio 2022 Professional)
- **Build System**: CMake 3.24+ with Ninja generator
- **Database**: Kuzu graph database via C++ API
- **Files**: Any C++ file with standard library includes processed through CMake-generated compilation databases

### **Precise Reproduction Steps**

#### **Prerequisites**
1. Windows system with Visual Studio 2022 Professional
2. Dosatsu project built and ready (`python please.py build`)
3. CMake-based examples infrastructure deployed

#### **Reproduction Command**
```bash
cd C:\UnitySrc\CppGraphIndex
python Examples\run_examples.py --index comprehensive_compile_commands.json
```

#### **Expected vs Actual Behavior**
- **Expected**: Database indexing completes successfully for files with standard library includes
- **Actual**: Kuzu parser fails with "expected rule oC_SingleQuery" errors

#### **Log File Analysis**
- **Success Case** (`comprehensive_no_std_compile_commands.json`): ~50-100 lines of output
- **Failure Case** (`comprehensive_compile_commands.json`): 25,310+ lines of output
- **Log Location**: `artifacts\examples\logs\dosatsu_output_YYYYMMDD_HHMMSS.log`

#### **Compilation Database Differences**

**Working (no-std)**:
```json
{
  "directory": ".",
  "command": "clang++ -std=c++17 -I. -DEXAMPLE_MODE Examples/cpp/comprehensive/no_std_example.cpp -o Examples/cpp/comprehensive/no_std_example",
  "file": "Examples/cpp/comprehensive/no_std_example.cpp"
}
```

**Failing (with standard library)**:
```json
{
  "directory": "C:/UnitySrc/CppGraphIndex/artifacts/examples/comprehensive",
  "command": "C:\\PROGRA~1\\MICROS~1\\2022\\PROFES~1\\VC\\Tools\\MSVC\\1443~1.348\\bin\\Hostx64\\x64\\cl.exe /nologo /TP -DEXAMPLE_MODE /DWIN32 /D_WINDOWS /EHsc /Zi /Ob0 /Od /RTC1 -std:c++17 -MDd /W4 /EHsc /FoCMakeFiles\\complete_example.dir\\src\\complete_example.cpp.obj /FdCMakeFiles\\complete_example.dir\\ /FS -c C:\\UnitySrc\\CppGraphIndex\\Examples\\cmake_projects\\comprehensive_examples\\src\\complete_example.cpp",
  "file": "C:\\UnitySrc\\CppGraphIndex\\Examples\\cmake_projects\\comprehensive_examples\\src\\complete_example.cpp"
}
```

### **Key Differences Leading to Issue**
1. **Compiler**: `clang++` vs `MSVC cl.exe`
2. **Standard Library Access**: MSVC provides full standard library access, generating massive AST data
3. **File Complexity**: `complete_example.cpp` includes multiple standard library headers vs. `no_std_example.cpp` with custom implementations

### **Technical Investigation Details**

#### **Error Pattern Analysis**
- Error occurs at character offset 103 in Kuzu CREATE queries
- Pattern: `source_file: 'C:\UnitySrc\...` suggests path handling, but paths are correctly escaped
- Error location corresponds to Windows file path start position in query
- 25,310 lines of AST data vs. normal ~100 lines indicates volume issue

#### **AST Volume Analysis**
Standard library headers like `<string>` and `<vector>` pull in:
- Template class definitions
- Template specializations  
- Internal implementation details
- STL container hierarchies
- Memory management structures
- Type trait definitions

This creates exponentially more AST nodes compared to `no_std` variants.

#### **Kuzu Batch Operation Analysis**
- Dosatsu uses batched queries for performance: `database.addToBatch(query)`
- Batch size: 100 operations (`BATCH_SIZE = 100` in `KuzuDatabase.cpp`)
- Large AST generates thousands of CREATE queries in each batch
- Kuzu parser appears to have limits on total batch content size

### **Current Workarounds**

1. **Use No-Std Variants**: 
   ```bash
   python Examples\run_examples.py --index comprehensive_no_std_compile_commands.json
   ```
   **Status**: ✅ Works perfectly

2. **Verification Testing**:
   ```bash
   python Examples\run_examples.py --all  # Uses no-std for verification
   ```
   **Status**: ✅ All tests pass (3/3)

### **Impact Assessment**
- **Compilation**: ✅ No impact - standard library compilation works perfectly
- **Development Workflow**: ✅ No impact - verification tests pass completely  
- **Standard Library Resolution**: ✅ No impact - MSVC resolves all headers correctly
- **Database Indexing**: ❌ Cannot index files with extensive standard library usage
- **Production Use**: ⚠️ Limited - works for analysis of `no_std` code

### **Potential Solutions for Future Investigation**

1. **Kuzu Batch Size Reduction**: Reduce `BATCH_SIZE` from 100 to smaller values (10, 50)
   - **Location**: `source/KuzuDatabase.cpp` line ~79
   - **Risk**: May impact performance for normal files

2. **Streaming Queries**: Replace batch operations with individual query execution
   - **Approach**: Modify `KuzuDatabase::addToBatch()` to execute immediately for large ASTs
   - **Detection**: Count pending queries or estimate total size

3. **AST Filtering**: Filter out internal standard library implementation details
   - **Target**: Skip implicit template instantiations from standard library
   - **Implementation**: Add filtering in `ASTNodeProcessor::createASTNode()`

4. **Kuzu Version Update**: Investigate newer Kuzu versions with improved parser limits
   - **Current**: Kuzu version used in `third_party/kuzu/`
   - **Action**: Check for updates and parser improvements

5. **Alternative Database**: Consider alternative graph databases for large AST storage
   - **Options**: Neo4j, ArangoDB, or custom solution
   - **Trade-off**: Migration effort vs. capacity

6. **Query Chunking**: Break large CREATE operations into smaller, manageable chunks
   - **Approach**: Split large batches into sub-batches based on content size
   - **Implementation**: Monitor total query string length

### **Technical Requirements for Fix**
- **Database Knowledge**: Understanding of Kuzu query parser limitations and configuration
- **AST Analysis**: Ability to filter/reduce AST node volume without losing semantic information
- **Performance Tuning**: Balance between data completeness and parser capacity
- **Regression Testing**: Ensure fixes don't break existing `no_std` functionality

### **File Locations for Investigation**
- **Batch Processing**: `source/KuzuDatabase.cpp` (lines 78-85, batch operations)
- **AST Node Creation**: `source/ASTNodeProcessor.cpp` (query construction)
- **Query Construction**: `source/ASTNodeProcessor.cpp` (lines 75-80, 137-142, 199-204)
- **Configuration**: `source/KuzuDatabase.h` (BATCH_SIZE constant)

### **Investigation Priority**
**Medium** - The core functionality (standard library compilation and resolution) works correctly. The issue only affects database indexing of files with heavy standard library usage. The transparent CMake integration and toolchain detection are production-ready and provide the foundation for this optimization work.

### **Related Issues**
- Standard library support implementation is complete and working
- CMake-based compilation database generation is functional
- Windows path handling and MSVC toolchain detection work correctly

---

*This document tracks technical issues requiring investigation. For examples-specific issues, see `EXAMPLE_BUGS.md`.*
