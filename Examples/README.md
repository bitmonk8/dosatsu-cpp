# CppGraphIndex Examples

This directory contains comprehensive examples and analysis tools for the CppGraphIndex project, demonstrating C++ language features and providing tools to verify correct analysis.

## Directory Structure

```
Examples/
├── README.md                    # This file
├── cpp/                        # C++ example files
│   ├── README.md               # C++ examples documentation
│   ├── basic/                  # Focused examples of specific features
│   ├── comprehensive/          # Complex multi-feature examples
│   └── compilation/            # Compilation configuration files
└── analysis/                   # Python analysis and verification tools
    ├── README.md               # Analysis tools documentation
    ├── run_analysis.py         # Main analysis runner
    ├── framework.py            # Analysis framework
    └── analyzers/              # Individual analysis modules
```

## Quick Start

### Running Examples

1. **Build CppGraphIndex:**
   ```bash
   python please.py build
   ```

2. **Use the Examples Runner (Recommended):**
   ```bash
   # List all available examples
   python Examples/run_examples.py --list
   
   # Run complete workflow (analyze + verify)
   python Examples/run_examples.py --all
   
   # Run specific operations
   python Examples/run_examples.py --analyze comprehensive_no_std_compile_commands.json
   python Examples/run_examples.py --verify
   ```

3. **Or run analysis directly:**
   ```bash
   python Examples/analysis/run_analysis.py
   ```

### Using Individual Examples

1. **Compile a specific example:**
   ```bash
   clang++ -std=c++17 -I. Examples/cpp/basic/inheritance.cpp -o inheritance_example
   ```

2. **Analyze with CppGraphIndex:**
   ```bash
   ./artifacts/debug/bin/MakeIndex.exe Examples/cpp/compilation/comprehensive_compile_commands.json my_database
   ```

## C++ Examples

The `cpp/` directory contains well-documented C++ examples organized by complexity:

- **`basic/`** - Individual language features (inheritance, templates, etc.)
- **`comprehensive/`** - Complex examples combining multiple features
- **`compilation/`** - Ready-to-use compilation configurations

Each example includes comprehensive documentation and demonstrates real-world usage patterns.

## Analysis Tools

The `analysis/` directory contains Python tools for verifying that CppGraphIndex correctly analyzes C++ code:

- **Framework** - Database setup and query utilities
- **Analyzers** - Verification modules for specific language features
- **Runner** - Orchestrates the complete analysis process

## Educational Value

These examples serve multiple purposes:

1. **Learning C++** - Well-documented examples of language features
2. **Understanding CppGraphIndex** - See how the tool analyzes code
3. **Verification** - Ensure the analysis tool works correctly
4. **Development** - Test new features and edge cases

## Adding New Examples

### C++ Examples

1. Create new `.cpp` files in the appropriate `cpp/` subdirectory
2. Add comprehensive documentation headers
3. Include in relevant compilation configurations
4. Update README files as needed

### Analysis Modules

1. Create new analyzer in `analysis/analyzers/`
2. Inherit from `BaseAnalyzer` class
3. Implement analysis logic using the framework
4. Add to the analyzer list in `framework.py`

## Integration with Main Project

These examples are automatically used by the main CppGraphIndex test suite. The `tests/` directory in the project root uses these examples to verify correct operation during development and CI.

## Requirements

- **C++ Compiler**: clang++ with C++17 support
- **Python**: 3.7+ with kuzu library
- **CppGraphIndex**: Built from source (`python please.py build`)

## License

These examples are part of the CppGraphIndex project and follow the same license terms.