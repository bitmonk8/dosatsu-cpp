# Dosatsu Examples

This directory contains comprehensive examples and analysis tools for the Dosatsu project, demonstrating C++ language features and providing tools to verify correct analysis.

## Directory Structure

```
Examples/
├── README.md                    # This file
├── cpp/                        # C++ example files
│   ├── README.md               # C++ examples documentation
│   ├── basic/                  # Focused examples of specific features
│   ├── comprehensive/          # Complex multi-feature examples
│   └── compilation/            # Compilation configuration files
└── queries/                    # Python query and verification tools
    ├── README.md               # Query tools documentation
    ├── run_queries.py          # Main verification runner
    ├── database_operations.py  # Database setup functions
    ├── query_operations.py     # Query execution functions
    ├── assertion_helpers.py    # Query assertion utilities
    ├── verification_result.py  # Result data structures
    └── verifiers/              # Individual verification modules
```

## Quick Start

### Running Examples

1. **Build Dosatsu:**
   ```bash
   python please.py build
   ```

2. **Use the Examples Runner (Recommended):**
   ```bash
   # List all available examples
   python Examples/run_examples.py --list
   
   # Run complete workflow (index + verify)
   python Examples/run_examples.py --all
   
   # Run specific operations
   python Examples/run_examples.py --index comprehensive_no_std_compile_commands.json
   python Examples/run_examples.py --verify
   ```

3. **Or run verification queries directly:**
   ```bash
   python Examples/queries/run_queries.py
   ```

### Using Individual Examples

1. **Compile a specific example:**
   ```bash
   clang++ -std=c++17 -I. Examples/cpp/basic/inheritance.cpp -o inheritance_example
   ```

2. **Analyze with Dosatsu:**
   ```bash
   ./artifacts/debug/bin/Dosatsu.exe Examples/cpp/compilation/comprehensive_compile_commands.json my_database
   ```

## C++ Examples

The `cpp/` directory contains well-documented C++ examples organized by complexity:

- **`basic/`** - Individual language features (inheritance, templates, etc.)
- **`comprehensive/`** - Complex examples combining multiple features
- **`compilation/`** - Ready-to-use compilation configurations

Each example includes comprehensive documentation and demonstrates real-world usage patterns.

## Verification Query Tools

The `queries/` directory contains Python tools for verifying that Dosatsu correctly analyzes C++ code using a functional approach:

- **Database Operations** - Database setup and management functions
- **Query Operations** - Cypher query execution and result processing
- **Assertion Helpers** - Common verification patterns and utilities
- **Verifiers** - Functional verification modules for specific language features
- **Runner** - Orchestrates the complete verification process

## Educational Value

These examples serve multiple purposes:

1. **Learning C++** - Well-documented examples of language features
2. **Understanding Dosatsu** - See how the tool analyzes code
3. **Verification** - Ensure the analysis tool works correctly
4. **Development** - Test new features and edge cases

## Adding New Examples

### C++ Examples

1. Create new `.cpp` files in the appropriate `cpp/` subdirectory
2. Add comprehensive documentation headers
3. Include in relevant compilation configurations
4. Update README files as needed

### Verification Modules

1. Create new verifier function in `queries/verifiers/`
2. Return `VerificationResult` with pass/fail status and details
3. Use functional approach with database connection parameter
4. Add to the verifier list in `run_queries.py`

## Integration with Main Project

These examples are automatically used by the main Dosatsu test suite. The `tests/` directory in the project root uses these examples to verify correct operation during development and CI.

## Requirements

- **C++ Compiler**: clang++ with C++17 support
- **Python**: 3.7+ with kuzu library
- **Dosatsu**: Built from source (`python please.py build`)

## License

These examples are part of the Dosatsu project and follow the same license terms.