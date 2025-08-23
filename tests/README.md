# CppGraphIndex End-to-End Test Suite

This directory contains comprehensive black-box end-to-end tests for the CppGraphIndex project. The tests verify that the system correctly analyzes C++ code and stores the results in a Kuzu graph database according to the schema defined in `DB_SCHEMA.md`.

## Overview

The test suite consists of:

1. **Comprehensive C++ test files** in `KuzuOutputTest/` that exercise all major C++ language features
2. **Python test framework** that runs MakeIndex on the test files and validates the database
3. **Individual test modules** that verify specific aspects of the analysis

## Test Architecture

### C++ Test Files

Located in `KuzuOutputTest/`, these files comprehensively cover:

- **`test_inheritance.cpp`**: Class inheritance, virtual functions, multiple inheritance, access specifiers
- **`test_templates.cpp`**: Class and function templates, specializations, metaprogramming, SFINAE
- **`test_namespaces.cpp`**: Namespaces, using declarations, qualified names, scope analysis
- **`test_control_flow.cpp`**: Control flow statements, CFG blocks, loops, conditionals, exceptions
- **`test_expressions.cpp`**: All expression types, operators, literals, casts, lambdas
- **`test_preprocessor.cpp`**: Macros, includes, conditional compilation, pragmas
- **`simple_test.cpp`**: Original simple test file

### Python Test Framework

The test framework (`test_framework.py`) provides:

- **Database setup**: Automatically runs MakeIndex on the test files
- **Query utilities**: Helper methods for executing Cypher queries
- **Assertion framework**: Methods to verify expected results
- **Cleanup**: Automatic cleanup of temporary databases

### Individual Test Modules

Each test module validates a specific aspect:

- **`test_ast_nodes.py`**: Basic AST node structure and relationships
- **`test_inheritance.py`**: Inheritance relationships and virtual function analysis
- **`test_templates.py`**: Template declarations, specializations, and instantiations
- **`test_namespaces.py`**: Namespace analysis and using declarations
- **`test_control_flow.py`**: Control flow statements and CFG block analysis
- **`test_expressions.py`**: Expression analysis and operator recognition
- **`test_preprocessor.py`**: Preprocessor directive and macro analysis
- **`test_types.py`**: Type system analysis and type relationships
- **`test_declarations.py`**: Declaration analysis across all categories
- **`test_statements.py`**: Statement analysis and patterns

## Running the Tests

### Prerequisites

1. **Build the project**:
   ```bash
   please build
   ```

2. **Install Kuzu Python library**:
   ```bash
   pip install kuzu
   ```

### Running All Tests

```bash
# From the project root
cd tests
python run_tests.py
```

Or:

```bash
# From the project root
python tests/run_tests.py
```

### Running Individual Tests

```bash
cd tests
python -c "
from test_framework import TestFramework
from test_inheritance import TestInheritanceTest

framework = TestFramework()
framework.setup_test_database()
test = TestInheritanceTest(framework)
test.execute()
framework.cleanup_test_database()
"
```

## What the Tests Verify

### Schema Compliance

- All expected node types (ASTNode, Declaration, Type, Statement, Expression, etc.)
- All expected relationship types (PARENT_OF, HAS_TYPE, INHERITS_FROM, etc.)
- Proper field population according to the schema

### C++ Feature Coverage

- **Classes and Objects**: Class declarations, member functions, constructors/destructors
- **Inheritance**: Single, multiple, virtual inheritance; access specifiers
- **Templates**: Class/function templates, specializations, metaprogramming
- **Namespaces**: Namespace declarations, using declarations, qualified names
- **Control Flow**: All statement types, CFG blocks, control flow edges
- **Expressions**: All expression types, operators, literals, type conversions
- **Preprocessor**: Macros, includes, conditional compilation
- **Types**: Built-in and user-defined types, qualifiers, template instantiations

### Data Quality

- Proper source location information (file, line, column)
- Correct parent-child relationships in the AST
- Accurate type information and relationships
- Meaningful qualified names and namespace contexts

## Test Output

The test suite provides detailed output showing:

- Database statistics (number of nodes per type)
- Feature coverage (which C++ constructs were found)
- Sample data (examples of detected patterns)
- Warnings for missing expected features
- Pass/fail status for each test module

Example output:
```
=== Database Summary ===
ASTNode: 1247
Declaration: 312
Type: 89
Statement: 445
Expression: 678
...

--- Running TestInheritanceTest ---
✓ Found class Animal
✓ Found class Mammal
✓ Found inheritance: Mammal -> Animal
✓ Found 3 virtual function overrides
✓ TestInheritanceTest PASSED
```

## Extending the Tests

### Adding New C++ Test Cases

1. Create new `.cpp` files in `KuzuOutputTest/`
2. Add them to `KuzuOutputTest/compile_commands.json`
3. The existing tests will automatically analyze the new files

### Adding New Test Modules

1. Create a new Python file in `tests/`
2. Inherit from `BaseTest` class
3. Implement the `run()` method with your test logic
4. Add the module to the test list in `test_framework.py`

### Adding New Assertions

The framework provides these assertion methods:
- `assert_query_has_results()`: Verify query returns at least one result
- `assert_query_count()`: Verify exact result count
- `assert_query_min_count()`: Verify minimum result count
- `query_to_list()`: Get query results as a list of dictionaries

## Troubleshooting

### MakeIndex Not Found

Ensure you've built the project:
```bash
please build
```

### Kuzu Import Error

Install the Kuzu Python library:
```bash
pip install kuzu
```

### Test Failures

1. Check that all C++ test files compile correctly
2. Verify MakeIndex runs without errors
3. Examine the database summary for expected node counts
4. Look at specific test failure messages for details

### Database Issues

The framework automatically handles database cleanup, but if you encounter issues:
```bash
# Clean up any leftover temporary directories
rm -rf /tmp/kuzu_test_*
```

## Implementation Notes

- Tests use temporary databases that are automatically cleaned up
- MakeIndex is run with a 5-minute timeout
- Database connections are properly managed and closed
- All tests are independent and can run in any order
- The framework is designed to be extensible for future features
