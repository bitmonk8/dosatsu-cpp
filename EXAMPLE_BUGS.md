# Known Issues in Examples

This document describes known issues in the Dosatsu examples system that require attention.

## 🚀 Quick Reproduction

### **Bug #1 (Standard Library Dependencies)**
```bash
# Single command to reproduce:
python Examples\run_examples.py --index comprehensive_compile_commands.json
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
- ✅ `schema_coverage_compile_commands.json`

### Problematic Compilation Databases
- ❌ `comprehensive_compile_commands.json` (Bug #1)
- ❌ `comprehensive_advanced_compile_commands.json` (Bug #1)

---

## Resolution Priority

1. **Medium Priority**: Bug #1 (Standard library) - Limits example coverage

This issue should be addressed to provide complete examples functionality.
