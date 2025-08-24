# C++ Examples for Dosatsu

This directory contains comprehensive C++ example files that demonstrate various language features and serve as test cases for the Dosatsu analysis tool.

## Directory Structure

### `basic/` - Focused Language Feature Examples

Each file demonstrates specific C++ language constructs:

- **`inheritance.cpp`** - Class inheritance, virtual functions, polymorphism
- **`templates.cpp`** - Template classes, functions, specializations, metaprogramming  
- **`namespaces.cpp`** - Namespace usage, scope resolution, ADL
- **`control_flow.cpp`** - Control flow statements, loops, exception handling
- **`expressions.cpp`** - Operators, literals, type conversions, lambdas
- **`preprocessor.cpp`** - Preprocessor directives, macros, conditional compilation
- **`inheritance_no_std.cpp`** - Inheritance examples without standard library
- **`simple_no_includes.cpp`** - Basic examples with minimal dependencies
- **`simple2.cpp`** - Additional simple examples

### `comprehensive/` - Complex Multi-Feature Examples

These files combine multiple C++ features in realistic scenarios:

- **`complete_example.cpp`** - Comprehensive example combining all major features
- **`standard_example.cpp`** - Complete example with standard library usage
- **`no_std_example.cpp`** - Comprehensive example without standard library dependencies
- **`clean_example.cpp`** - Well-documented comprehensive example

### `compilation/` - Compilation Configurations

Ready-to-use `compile_commands.json` files for different scenarios:

- **`comprehensive_compile_commands.json`** - Compiles all basic examples
- **`comprehensive_no_std_compile_commands.json`** - Compiles no-std comprehensive example
- **`simple_compile_commands.json`** - Compiles basic examples
- **`single_file_compile_commands.json`** - Template for single file compilation
- **`multi_file_compile_commands.json`** - Template for multi-file projects

## Example Features

### Documentation Standards

Each example file includes:

```cpp
/**
 * @file filename.cpp
 * @brief Brief description of what this example demonstrates
 * 
 * This example showcases:
 * - Feature 1 with explanation
 * - Feature 2 with explanation
 * - Feature 3 with explanation
 */
```

### Code Quality

- **Self-contained**: Examples compile independently where possible
- **Well-commented**: Complex constructs include explanatory comments
- **Realistic**: Demonstrates real-world usage patterns
- **Progressive**: Build from simple to complex concepts
- **Standards-compliant**: Follow C++17 standard

### Compilation Flags

Examples are designed to compile with:
- `-std=c++17` - C++17 standard
- `-I.` - Include current directory
- `-DEXAMPLE_MODE` - Define example mode for conditional compilation

## Usage Patterns

### Individual Compilation

```bash
# Compile a single example
clang++ -std=c++17 -I. Examples/cpp/basic/inheritance.cpp -o inheritance_example

# Run the compiled example
./inheritance_example
```

### Batch Compilation

```bash
# Use compilation database
clang++ @Examples/cpp/compilation/comprehensive_compile_commands.json
```

### Analysis with Dosatsu

```bash
# Analyze examples
./artifacts/debug/bin/dosatsu_cpp.exe Examples/cpp/compilation/comprehensive_compile_commands.json output_db

# Verify analysis
python Examples/queries/run_queries.py
```

## Educational Progression

### Beginner Level
1. `simple_no_includes.cpp` - Basic syntax without dependencies
2. `simple2.cpp` - Additional basic concepts
3. `inheritance.cpp` - Object-oriented programming basics

### Intermediate Level
1. `templates.cpp` - Generic programming
2. `namespaces.cpp` - Code organization
3. `expressions.cpp` - Advanced operators and conversions

### Advanced Level
1. `control_flow.cpp` - Complex control structures
2. `preprocessor.cpp` - Metaprogramming with preprocessor
3. `complete_example.cpp` - Integration of all concepts

## Testing Integration

These examples are used by:

1. **Dosatsu Analysis** - Verify correct parsing and indexing
2. **Regression Testing** - Ensure changes don't break existing functionality  
3. **Feature Development** - Test new language feature support
4. **Performance Benchmarking** - Measure analysis speed and memory usage

## Adding New Examples

### Guidelines

1. **Focus**: Each basic example should focus on specific language features
2. **Documentation**: Include comprehensive file and inline documentation
3. **Compilation**: Ensure examples compile cleanly with specified flags
4. **Dependencies**: Minimize external dependencies where possible
5. **Realism**: Use realistic code patterns, not just syntax demonstrations

### Process

1. Create the example file in the appropriate subdirectory
2. Add comprehensive documentation headers
3. Test compilation with standard flags
4. Add to relevant compilation configurations
5. Update this README if adding new categories
6. Test with Dosatsu analysis tools

## Maintenance

- **Regular Updates**: Keep examples current with C++ standards
- **Compiler Testing**: Verify compatibility with major compilers
- **Documentation**: Keep README files synchronized with actual content
- **Analysis Verification**: Ensure examples work with latest Dosatsu features
