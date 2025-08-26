# Dosatsu C++

Dosatsu C++ is a tool that scans C++ projects using Clang and builds a graph database in Kuzu containing parts or all of the Abstract Syntax Tree (AST) of the project. The resulting database assists AI tools in navigating large C++ codebases by providing structured access to code analysis data.

## 🚀 Features

Dosatsu scans C++ projects and builds comprehensive graph databases containing:

- **AST Analysis**: Complete Abstract Syntax Tree representation
- **Code Structure**: Classes, functions, templates, and their relationships  
- **Dependency Mapping**: Include relationships and symbol dependencies
- **Type Information**: Detailed type analysis and template instantiations

## 📋 Requirements

### System Requirements

- **Python 3.8+** for build orchestration
- **CMake 3.24+** for build configuration
- **Ninja** for fast parallel builds
- **C++20 compatible compiler**
- **Git** for dependency management

### Supported Platforms and Toolchains

Dosatsu supports the following platform and toolchain combinations:

- **Windows** with **MSVC** (Microsoft Visual C++)
- **Linux** with **GCC** or **Clang**
- **macOS** with **Clang** (Apple Clang or LLVM Clang)

> **Note**: Other platform/toolchain combinations are not currently supported and will result in build errors. Please ensure you are using one of the supported combinations above.

### Dependencies

All dependencies are automatically managed through the build system:

- **LLVM/Clang 19.1.7**: Automatically downloaded and built via CMake FetchContent
- **Kuzu**: Graph database integration (future component)
- **doctest**: Unit testing framework (included)

## 🚀 Quick Start

### 1. Initial Setup

```bash
# Clone the repository
git clone https://github.com/your-org/Dosatsu.git
cd Dosatsu

# Initial environment setup (creates artifact directories, validates tools)
please setup

# Display build environment information
please info
```

### 2. Build the Project

```bash
# Configure and build (debug mode)
please configure --debug
please build --debug

# Or do everything in one step
please rebuild --debug
```

### 3. Run Tests

```bash
# Run all tests
please test

# Run tests with detailed reporting
please test --verbose --report-format html
```

## 📚 Examples and Analysis

Dosatsu includes comprehensive C++ examples and analysis tools to demonstrate its capabilities and verify correct operation.

### C++ Examples

The `Examples/cpp/` directory contains well-documented C++ code demonstrating various language features:

- **Basic Examples** (`Examples/cpp/basic/`): Focused examples of specific C++ features
  - `inheritance.cpp` - Class inheritance and virtual functions
  - `templates.cpp` - Template classes, functions, and specializations
  - `namespaces.cpp` - Namespace usage and scope resolution
  - `control_flow.cpp` - Control flow statements and exception handling
  - `expressions.cpp` - Operators, literals, and type conversions
  - `preprocessor.cpp` - Preprocessor directives and macros

- **Comprehensive Examples** (`Examples/cpp/comprehensive/`): Complex multi-feature examples
  - `complete_example.cpp` - Integration of all major C++ features
  - `no_std_example.cpp` - Examples without standard library dependencies

### Verification Query Tools

The `Examples/queries/` directory provides Python tools for verifying Dosatsu output using a functional approach:

```bash
# Easy way: Use the examples runner
python Examples/run_examples.py --all

# Or run verification queries directly
python Examples/queries/run_queries.py
```

The verification suite:
1. Builds a Kuzu database from the C++ examples
2. Runs verification queries to ensure correct parsing
3. Validates that all major C++ constructs are properly captured
4. Reports detailed results and statistics

### Using Examples

```bash
# Use the examples runner
python Examples/run_examples.py --list                    # List all examples
python Examples/run_examples.py --index comprehensive_no_std_compile_commands.json  # Index examples
python Examples/run_examples.py --verify                  # Run verification queries
python Examples/run_examples.py --all                     # Run complete workflow
```

See `Examples/README.md` for detailed documentation.

## 📊 Database Schema

Dosatsu generates a comprehensive graph database that captures the complete structure of C++ codebases. The database schema is designed to support advanced querying capabilities for AI tools and code analysis.

### Schema Documentation

For detailed information about the database structure, node types, relationships, and query examples, see **[SCHEMA.md](SCHEMA.md)**. This comprehensive document covers:

- **Node Types**: ASTNode, Declaration, Type, Statement, Expression, and specialized nodes
- **Relationships**: Inheritance, template specialization, control flow, and semantic connections  
- **Query Examples**: Ready-to-use Cypher queries for common code analysis tasks
- **C++ Mappings**: How C++ language constructs map to database entities

### Key Schema Features

The database schema supports querying for:
- **Code Navigation**: Find declarations, definitions, usages, and dependencies
- **Architecture Query**: Understand inheritance hierarchies and relationships
- **Template Query**: Track template instantiations and specializations
- **Control Flow**: Analyze function control flow graphs and execution paths
- **Documentation**: Access comments and documentation associated with code elements

## 🔧 Build System Commands

The `please` script provides a unified interface for all development operations:

### Configuration & Building

```bash
# Configuration
please configure [--debug|--release] [--clean]
please reconfigure                           # Clean configure from scratch

# Building
please build [--debug|--release] [--parallel N]
please rebuild                               # Clean + configure + build + test
please clean                                 # Clean build artifacts
```

### Testing & Quality

```bash
# Testing
please test [--verbose] [--parallel N]
please test --target specific_test
please test --ci-mode --coverage            # CI-friendly with coverage

# Code Quality
please format [--check-only]                # Format code with clang-format
please lint [--summary-only]                # Two-phase lint: auto-fix then report
```

### Git Integration

```bash
# Git Operations (with intelligent pre/post checks)
please git-status                           # Enhanced git status
please git-pull [--rebase] [--check-clean]
please git-push [--set-upstream]
please git-commit -m "message"              # With pre-commit checks
please git-clean [--force] [--include-build-artifacts]
```

### Performance & Analysis

```bash
# Performance Analysis
please build-stats                          # Build performance metrics
please cache-mgmt [--clean-deps] [--clean-cmake]
please info                                 # Environment information
```

### Utility Commands

```bash
# Development Tools
please install-git-hooks                    # Install formatting pre-commit hooks
please compile-db [--copy-to-root]          # Manage compilation database
```

## 🏗️ Project Structure

```
Dosatsu/
├── please.py                   # 🎯 Main build orchestrator
├── please.bat                 # 🪟 Windows wrapper script
├── please                     # 🐧 Unix/Linux/macOS wrapper script
├── CMakeLists.txt             # Root CMake configuration
├── .clang-format              # Code formatting rules
├── .clang-tidy                # Static analysis configuration
├── .gitignore                 # Updated for new artifact structure
│
├── source/                    # 📁 Main source code
│   ├── CMakeLists.txt         # Target-specific CMake config
│   ├── Dosatsu.cpp            # Main application
│   ├── KuzuDump.cpp           # Database operations
│   ├── KuzuDump.h             # Database interface
│   └── NoWarningScope_*.h     # Utility headers
│
├── third_party/               # 📦 Dependency management
│   └── dependencies.cmake    # LLVM FetchContent configuration
│
├── scripts/                   # 🛠️ Build helper scripts
│   ├── setup_deps.cmake      # Dependency setup helpers
│   ├── format_config.py      # Formatting configuration
│   ├── validate-ci-quick.py  # Quick CI validation
│   └── validate-ci.py        # Full CI validation
│
├── artifacts/                 # 🗂️ ALL BUILD OUTPUTS (git-ignored)
│   ├── debug/                 # Debug build artifacts
│   │   ├── build/             # CMake/Ninja files
│   │   ├── bin/               # Debug executables
│   │   ├── lib/               # Debug libraries
│   │   └── logs/              # Build logs
│   ├── release/               # Release build artifacts
│   ├── test/                  # Test results & reports
│   ├── lint/                  # Linting results
│   └── format/                # Formatting logs
│
├── third_party/               # 📚 Included dependencies
│   └── include/doctest/       # Testing framework
│
└── .github/workflows/         # 🔄 CI/CD pipeline
    └── ci.yml                 # Multi-platform build & test
```

## 🧪 Testing

The project uses [doctest](https://github.com/doctest/doctest) for unit testing with comprehensive reporting:

### Running Tests

```bash
# Basic test execution
please test

# Advanced test options
please test --verbose --parallel auto
please test --target Dosatsu_SelfTest
please test --ci-mode --historical
please test --coverage --report-format html
```

### Test Artifacts

Tests generate comprehensive reports in `artifacts/test/`:
- `results.xml` - JUnit format for CI integration
- `test-report.html` - Rich HTML report with statistics
- `test-report.json` - Machine-readable results
- `test-history.json` - Historical test tracking
- `test-trends.txt` - Performance trend analysis

## 🔄 Development Workflow

### Recommended Daily Workflow

```bash
# 1. Start with clean environment
please git-status

# 2. Pull latest changes
please git-pull --rebase

# 3. Make your changes...

# 4. Pre-commit workflow
please format              # Auto-format code
please lint                 # Two-phase: auto-fix then report remaining issues
please rebuild             # Full rebuild + test

# 5. Commit with validation
please git-commit -m "Your changes"

# 6. Push changes
please git-push
```

### Code Quality Standards

The build system enforces consistent code quality:

- **Formatting**: Automatic clang-format integration with project-specific style
- **Linting**: Two-phase clang-tidy analysis with automatic fixes followed by remaining issue reports
- **Testing**: Mandatory test execution before commits
- **Pre-commit Hooks**: Optional git hooks for automatic validation

#### Two-Phase Linting

The `please lint` command runs in two phases for optimal developer experience:

1. **Phase 1 (Auto-fix)**: Runs clang-tidy with `--fix` to automatically correct common issues
2. **Phase 2 (Report)**: Runs clang-tidy again to report issues requiring manual attention

This approach reduces developer friction by handling routine fixes automatically while clearly highlighting issues that need thoughtful resolution.

### Performance Optimization

Monitor and optimize build performance:

```bash
# Analyze build performance
please build-stats

# Manage caches (LLVM dependencies can be ~36GB)
please cache-mgmt

# Clean specific caches
please cache-mgmt --clean-cmake --clean-deps
```

## 🚀 CI/CD Pipeline

The project includes a comprehensive GitHub Actions workflow:

### Multi-Platform Testing

- **Platforms**: Windows, Linux, macOS
- **Build Types**: Debug and Release
- **Parallel Jobs**: Optimized for fast feedback

### Automated Workflows

- **Build Validation**: All platforms and configurations
- **Test Execution**: Comprehensive test suite with reporting
- **Code Quality**: Formatting and linting checks
- **Security Scanning**: CodeQL static analysis
- **Artifact Collection**: Build outputs and test results
- **Dependency Caching**: LLVM dependencies cached for faster builds

### CI Commands

```bash
# Simulate CI locally
please rebuild --debug --skip-tests     # Quick build check
please test --ci-mode                   # CI-style testing
please format --check-only              # Format validation
please lint --summary-only              # Quick two-phase lint check
```

## 🔧 Advanced Configuration

### Build Types

```bash
# Debug (default) - fast builds, debug symbols
please configure --debug

# Release - optimized builds
please configure --release

# Custom parallel jobs
please build --parallel 8
```

### Dependency Management

LLVM 19.1.7 is automatically managed via CMake FetchContent:
- **First Build**: Downloads and builds LLVM (~30+ minutes)
- **Subsequent Builds**: Uses cached LLVM build
- **Cache Location**: `artifacts/debug/build/_deps/llvm-*`

### Tool Integration

```bash
# Generate compilation database for IDEs
please compile-db --copy-to-root

# Install git pre-commit hooks
please install-git-hooks

# Environment validation
please info
```



## 🛠️ Troubleshooting

### Common Issues

**Issue**: Build fails with "linker out of heap space"
```bash
# Solution: Ensure 64-bit compiler environment
# On Windows, use: vcvars64.bat or VS 2022 x64 Native Tools Command Prompt
please info  # Check compiler detection
```

**Issue**: LLVM build takes too long
```bash
# Solution: Use cached builds and parallel jobs
please build --parallel 4  # Adjust for your CPU
please cache-mgmt           # Check cache status
```

**Issue**: Git operations fail
```bash
# Solution: Check repository status
please git-status          # Comprehensive status
please git-clean --force   # Clean untracked files
```

**Issue**: Tests fail
```bash
# Solution: Check test output and rebuild
please test --verbose      # Detailed test output
please rebuild             # Clean rebuild
```

### Getting Help

```bash
# Comprehensive environment information
please info

# Command-specific help
please --help
please <command> --help

# Build performance analysis
please build-stats
```

## 📝 Programming Language

This project is written in **C++20** and built with modern development practices:

- **Standards Compliance**: Full C++20 support
- **Cross-Platform**: Windows, Linux, macOS
- **Modern Dependencies**: LLVM 19.1.7 with FetchContent
- **Quality Assurance**: Comprehensive linting and formatting

## 📄 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## 🚧 Project Status

**Status**: Active Development - Modern Build System Complete!

✅ **Completed**: Modern build system migration with Python + CMake + Ninja
✅ **Completed**: Multi-platform CI/CD pipeline
✅ **Completed**: Git integration and development workflows
✅ **Completed**: Performance optimization and caching

🔄 **In Progress**: Core Dosatsu functionality expansion

## 🤝 Contributing

1. **Fork** the repository
2. **Setup** development environment:
   ```bash
   please setup
   please install-git-hooks  # Optional but recommended
   ```
3. **Create** a feature branch:
   ```bash
   git checkout -b feature/amazing-feature
   ```
4. **Develop** with quality checks:
   ```bash
   # Make your changes...
   please format              # Auto-format
   please lint                 # Two-phase: auto-fix then report
   please rebuild             # Build + test
   ```
5. **Commit** with validation:
   ```bash
   please git-commit -m "Add amazing feature"
   ```
6. **Push** and create Pull Request:
   ```bash
   please git-push --set-upstream
   ```

### Development Guidelines

- **Code Style**: Enforced via clang-format (LLVM style with customizations)
- **Quality**: All code must pass clang-tidy analysis
- **Testing**: Maintain or improve test coverage
- **Documentation**: Update documentation for user-facing changes

## 📞 Support

- **Issues**: Report bugs and request features on [GitHub Issues](https://github.com/your-org/Dosatsu/issues)
- **Discussions**: General questions and discussions on [GitHub Discussions](https://github.com/your-org/Dosatsu/discussions)
- **CI Status**: Check build status on [GitHub Actions](https://github.com/your-org/Dosatsu/actions)

---

> **🎯 Goal**: This project aims to bridge the gap between large C++ codebases and AI tools by providing a graph-based representation of code structure that can be queried using natural language.

> **⚡ Performance**: The build system provides fast builds, excellent dependency management, and comprehensive development workflow automation.