# Known Issues in Dosatsu

This document describes known technical issues in the Dosatsu project that require investigation.

---

## Issue #1: `--all` Flag Does Not Actually Run All Examples

**Problem**: The `python Examples\run_examples.py --all` command does not process all available example categories as the name suggests.

**Expected Behavior**: The `--all` flag should process examples from all available categories:
- `basic` (Examples/cpp/basic/): 10 C++ files
- `comprehensive` (Examples/cpp/comprehensive/): 11 C++ files  
- `nostd` (Examples/cpp/nostd/): 4 C++ files

**Actual Behavior**: The `--all` flag only processes the `nostd` category examples.

**Root Cause**: 
1. In `Examples/run_examples.py` line 699, the `--all` flag is hardcoded to only process `"comprehensive_no_std_compile_commands.json"` which maps to the `nostd` category.
2. The verification queries in `Examples/queries/database_operations.py` line 163 are also hardcoded to use `nostd_cmake_compile_commands.json`.

**How to Reproduce**:
1. Run `python Examples\run_examples.py --all`
2. Observe that only 4 files from the `nostd` category are processed:
   - `advanced_features_no_std.cpp`
   - `simple_no_includes.cpp` 
   - `inheritance_no_std.cpp`
   - `no_std_example.cpp`
3. The 21 files from `basic` and `comprehensive` categories are completely ignored.

**Impact**: Users expecting comprehensive testing of all example files will miss potential issues in the `basic` and `comprehensive` example sets.

---

*This document tracks technical issues requiring investigation. For examples-specific issues, see `EXAMPLE_BUGS.md`.*