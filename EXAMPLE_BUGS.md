# Known Issues in Examples

This document describes known issues in the Dosatsu examples system that require attention.

## 🚀 Quick Reproduction

### **Bug #1 (Standard Library Dependencies)**
```bash
# Single command to reproduce:
python Examples\run_examples.py --index comprehensive_compile_commands.json
```

### **Bug #2 (AST Assertion Failure)**  
```bash
# Single command to reproduce:
python Examples\run_examples.py --index schema_coverage_compile_commands.json
```

---

## Bug #1: Standard Library Header Dependencies

### **Status**: Known Issue
### **Severity**: Medium
### **Affected Files**: Multiple compilation databases

### **Description**
Several compilation databases reference C++ files that include standard library headers (`<iostream>`, `<string>`, `<type_traits>`, etc.), but the compilation commands don't include the necessary flags to locate these headers.

### **Affected Compilation Databases**
- `comprehensive_compile_commands.json`
- `comprehensive_advanced_compile_commands.json`
- Any compilation database that references files with `#include <...>` directives

### **Error Symptoms**
```
fatal error: 'string' file not found
fatal error: 'iostream' file not found
fatal error: 'type_traits' file not found
```

### **Affected Files**
- `Examples/cpp/basic/inheritance.cpp` - includes `<string>`
- `Examples/cpp/basic/expressions.cpp` - includes `<iostream>`
- `Examples/cpp/basic/control_flow.cpp` - includes `<iostream>`
- `Examples/cpp/basic/templates.cpp` - includes `<type_traits>`
- `Examples/cpp/basic/namespaces.cpp` - includes `<iostream>`
- `Examples/cpp/basic/preprocessor.cpp` - includes `<iostream>`

### **Precise Reproduction Steps**

#### **Option 1: Using run_examples.py**
```bash
# This will reproduce the bug:
python Examples\run_examples.py --index comprehensive_compile_commands.json
```

#### **Option 2: Direct Dosatsu execution** 
```bash
# Create a test compilation database (inheritance.cpp includes <string>):
cat > test_bug1.json << EOF
[
  {
    "directory": ".",
    "command": "clang++ -std=c++17 -I. -DEXAMPLE_MODE Examples/cpp/basic/inheritance.cpp -o Examples/cpp/basic/inheritance",
    "file": "Examples/cpp/basic/inheritance.cpp"
  }
]
EOF

# Run Dosatsu directly:
artifacts\debug\bin\dosatsu_cpp.exe test_bug1.json --output-db test_bug1.db
```

Expected error:
```
fatal error: 'string' file not found
   13 | #include <string>
      |          ^~~~~~~~
```

#### **Option 3: Single file test**
```bash
# Test just the inheritance.cpp file that includes <string>:
python Examples\run_examples.py --index comprehensive_compile_commands.json
# Look for the specific error on inheritance.cpp in the output
```

### **Current Workaround**
Use the "no_std" variants of compilation databases:
- `comprehensive_no_std_compile_commands.json` ✅ Works
- `advanced_no_std_compile_commands.json` ✅ Works
- `two_file_no_std_compile_commands.json` ✅ Works

### **Potential Solutions**
1. **Add standard library paths**: Modify compilation databases to include `-stdlib=libc++` or system-specific standard library paths
2. **Use no_std variants**: Continue using files that don't depend on standard library
3. **Create isolated examples**: Replace standard library usage with custom implementations

### **Example Fix for compilation database**
```json
{
  "directory": ".",
  "command": "clang++ -std=c++17 -stdlib=libc++ -I. -DEXAMPLE_MODE Examples/cpp/basic/inheritance.cpp -o Examples/test_inheritance",
  "file": "Examples/cpp/basic/inheritance.cpp"
}
```

---

## Bug #2: AST Processing Assertion Failure

### **Status**: Known Issue  
### **Severity**: High
### **Affected Files**: `schema_coverage_complete.cpp`

### **Description**
The `schema_coverage_compile_commands.json` compilation database triggers an assertion failure in Clang's AST processing code when processing complex C++ constructs.

### **Error Message**
```
Assertion failed: !isNull() && "Cannot retrieve a NULL type pointer", 
file C:\UnitySrc\CppGraphIndex\artifacts\llvm-include\clang/AST/Type.h, line 945
```

### **Affected Compilation Database**
- `schema_coverage_compile_commands.json`

### **Affected File**
- `Examples/cpp/comprehensive/schema_coverage_complete.cpp`

### **Error Context**
The assertion occurs during AST type processing, likely when the Dosatsu indexer encounters a complex template or type construct that results in a NULL type pointer being accessed.

### **Impact**
- Complete failure of indexing process
- Prevents testing of the most comprehensive schema coverage example
- May indicate deeper issues with complex C++ construct handling

### **Precise Reproduction Steps**

#### **Option 1: Using run_examples.py**
```bash
# This will reproduce the assertion immediately:
python Examples\run_examples.py --index schema_coverage_compile_commands.json
```

#### **Option 2: Direct Dosatsu execution**
```bash
# Create a test compilation database for the problematic file:
cat > test_bug2.json << EOF
[
  {
    "directory": ".",
    "command": "clang++ -std=c++17 -I. -DEXAMPLE_MODE -c Examples/cpp/comprehensive/schema_coverage_complete.cpp -o Examples/cpp/comprehensive/schema_coverage_complete.o",
    "file": "Examples/cpp/comprehensive/schema_coverage_complete.cpp"
  }
]
EOF

# Run Dosatsu directly:
artifacts\debug\bin\dosatsu_cpp.exe test_bug2.json --output-db test_bug2.db
```

Expected error:
```
Assertion failed: !isNull() && "Cannot retrieve a NULL type pointer", 
file C:\UnitySrc\CppGraphIndex\artifacts\llvm-include\clang/AST/Type.h, line 945
```

#### **Option 3: Isolate the problem** 
```bash
# The assertion happens during AST processing, so the bug reproduces immediately
# when Dosatsu tries to process schema_coverage_complete.cpp
artifacts\debug\bin\dosatsu_cpp.exe Examples\cpp\compilation\schema_coverage_compile_commands.json --output-db test.db
```

### **Investigation Needed**
1. **Isolate the problematic construct**: Systematically remove portions of `schema_coverage_complete.cpp` to identify which specific C++ construct triggers the assertion
2. **Type analysis**: The error occurs in `Type.h` line 945, suggesting issues with:
   - Template instantiation
   - Complex inheritance hierarchies  
   - Advanced template metaprogramming constructs
   - Dependent types or SFINAE constructs

### **Temporary Workaround**
Use alternative comprehensive examples:
- `comprehensive_no_std_compile_commands.json` ✅ Works
- `clean_example.cpp` ✅ Works after template fix

### **Potential Root Causes**
1. **Template complexity**: The file contains advanced template metaprogramming that may trigger edge cases
2. **Clang version compatibility**: Assertion may be specific to Clang 20.1.7
3. **Dosatsu AST handling**: The indexer may not properly handle certain type constructs before passing to Clang

### **Recommended Investigation Steps**
1. Create minimal reproduction by simplifying `schema_coverage_complete.cpp`
2. Test with different Clang versions
3. Add defensive null checks in Dosatsu's type processing code
4. Enable additional Clang debugging output to identify the exact construct causing the issue

---

## Testing Status Summary

| Option | Status | Notes |
|--------|--------|-------|
| `--list` | ✅ Working | Lists all examples correctly |
| `--compile` | ✅ Working | Fixed executable vs object file compilation |
| `--index` | ⚠️ Mostly Working | Fails on std library deps and assertion bug |
| `--verify` | ✅ Working | Verification queries pass |
| `--all` | ✅ Working | Complete workflow succeeds |

### Working Compilation Databases
- ✅ `simple_compile_commands.json`
- ✅ `single_file_compile_commands.json` (after path fix)
- ✅ `two_file_no_std_compile_commands.json` (after path fix)
- ✅ `multi_file_compile_commands.json` (after path fix)
- ✅ `comprehensive_no_std_compile_commands.json`
- ✅ `advanced_no_std_compile_commands.json`

### Problematic Compilation Databases
- ❌ `comprehensive_compile_commands.json` (Bug #1)
- ❌ `comprehensive_advanced_compile_commands.json` (Bug #1)
- ❌ `schema_coverage_compile_commands.json` (Bug #2)

---

## Resolution Priority

1. **High Priority**: Bug #2 (AST assertion) - Complete system failure
2. **Medium Priority**: Bug #1 (Standard library) - Limits example coverage

Both issues should be addressed to provide complete examples functionality, but Bug #2 represents a more serious system stability issue.
