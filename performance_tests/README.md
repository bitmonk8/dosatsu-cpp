# Dosatsu Performance Tests

This directory contains performance tests for comparing indexing speed between standard library and library-free C++ examples.

## Purpose

The standard library headers (especially `<vector>`, `<string>`, `<iostream>`) are very large and contain many complex templates, making them slow to parse and index. This performance test demonstrates the speed difference between:

1. **Standard Library Example** - Uses `<vector>` and demonstrates the indexing overhead
2. **Library-Free Examples** - Self-contained C++ code without standard library dependencies

## Usage

### Prerequisites

1. Build Dosatsu:
   ```bash
   python please.py build
   ```

2. Run the performance comparison:
   ```bash
   cd performance_tests
   python run_performance_tests.py
   ```

### Expected Results

The library-free examples should index significantly faster than the standard library example, demonstrating the performance benefit of avoiding heavy standard library headers.

## Files

- `std_library_performance_test.cpp` - Simple C++ example using `<vector>`
- `run_performance_tests.py` - Script to run performance comparison
- `CMakeLists.txt` - Build configuration for the performance test
- `README.md` - This file

## Performance Tips

For optimal Dosatsu indexing performance:

1. **Avoid heavy standard library headers** like `<vector>`, `<string>`, `<iostream>`
2. **Use simple custom implementations** when possible
3. **Minimize template complexity** in frequently included headers
4. **Prefer forward declarations** over full includes

## Example Output

```
=== Dosatsu Performance Test ===
Comparing indexing performance: Standard Library vs Library-Free Examples

Timing Standard Library Example (with <vector>)...
✓ Standard Library Example completed successfully
  Time: 0.856 seconds

Timing Library-Free Example...
✓ Library-Free Example completed successfully
  Time: 0.142 seconds

=== Performance Comparison Results ===
Standard Library Example:  0.856 seconds
Library-Free Example:      0.142 seconds

✓ Library-free example is 6.0x faster!
```

