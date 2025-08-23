# Known Bugs in CppGraphIndex Test Suite

## Bug #1: Assertion Failure with Complex Template Constructs

**Description**: MakeIndex crashes with assertion failure when processing certain complex C++ template constructs.

**Error Message**: 
```
Assertion failed: !isNull() && "Cannot retrieve a NULL type pointer", file C:\UnitySrc\CppGraphIndex\artifacts\llvm-include\clang/AST/Type.h, line 945
```

**Reproduction**: 
- File: `Examples/cpp/comprehensive/schema_coverage_complete.cpp`
- The file contains complex template specializations, multiple inheritance, and advanced C++ features
- Crash occurs during AST processing phase

**Impact**: 
- Prevents analysis of very complex C++ codebases with advanced template metaprogramming
- Basic and moderately complex examples work fine
- All existing tests continue to pass

**Workaround**: 
- Use simpler examples without complex template specializations
- The existing examples in `Examples/cpp/comprehensive/advanced_features_no_std.cpp` work correctly
- Break down complex files into smaller, more focused examples

**Status**: 
- Identified during comprehensive schema coverage testing
- Does not affect core functionality or existing test suite
- May require investigation into Clang AST handling for complex templates

## Testing Status

- All basic and intermediate examples work correctly
- Advanced features example (no-std version) processes successfully
- Core MakeIndex functionality is stable
- Test suite passes all tests (3/3 passing)