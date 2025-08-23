# CppGraphIndex

CppGraphIndex is a tool that scans C++ projects using Clang and builds a graph database in Kuzu containing parts or all of the Abstract Syntax Tree (AST) of the project. The resulting database assists AI tools in navigating large C++ codebases by enabling natural language queries that are converted to Cypher queries, with results converted back to natural language.

## 🚀 Features

The project consists of two main components:

- **MakeIndex**: Scans C++ projects and builds the graph database
- **MCP**: Implements Natural Language to Cypher query conversion and Result to Natural Language logic, exposed as an MCP (Model Context Protocol) service

## 📋 Requirements

### System Requirements

- **Python 3.8+** for build orchestration
- **CMake 3.24+** for build configuration
- **Ninja** for fast parallel builds
- **C++20 compatible compiler**
- **Git** for dependency management

### Supported Platforms and Toolchains

CppGraphIndex supports the following platform and toolchain combinations:

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
git clone https://github.com/your-org/CppGraphIndex.git
cd CppGraphIndex

# Initial environment setup (creates artifact directories, validates tools)
python please.py setup

# Display build environment information
python please.py info
```

### 2. Build the Project

```bash
# Configure and build (debug mode)
python please.py configure --debug
python please.py build --debug

# Or do everything in one step
python please.py rebuild --debug
```

### 3. Run Tests

```bash
# Run all tests
python please.py test

# Run tests with detailed reporting
python please.py test --verbose --report-format html
```

## 🔧 Build System Commands

The `please.py` script provides a unified interface for all development operations:

### Configuration & Building

```bash
# Configuration
python please.py configure [--debug|--release] [--clean]
python please.py reconfigure                           # Clean configure from scratch

# Building
python please.py build [--debug|--release] [--parallel N]
python please.py rebuild                               # Clean + configure + build + test
python please.py clean                                 # Clean build artifacts
```

### Testing & Quality

```bash
# Testing
python please.py test [--verbose] [--parallel N]
python please.py test --target specific_test
python please.py test --ci-mode --coverage            # CI-friendly with coverage

# Code Quality
python please.py format [--check-only]                # Format code with clang-format
python please.py lint [--summary-only]                # Two-phase lint: auto-fix then report
```

### Git Integration

```bash
# Git Operations (with intelligent pre/post checks)
python please.py git-status                           # Enhanced git status
python please.py git-pull [--rebase] [--check-clean]
python please.py git-push [--set-upstream]
python please.py git-commit -m "message"              # With pre-commit checks
python please.py git-clean [--force] [--include-build-artifacts]
```

### Performance & Analysis

```bash
# Performance Analysis
python please.py build-stats                          # Build performance metrics
python please.py cache-mgmt [--clean-deps] [--clean-cmake]
python please.py info                                 # Environment information
```

### Utility Commands

```bash
# Development Tools
python please.py install-git-hooks                    # Install formatting pre-commit hooks
python please.py compile-db [--copy-to-root]          # Manage compilation database
```

## 🏗️ Project Structure

```
CppGraphIndex/
├── please.py                   # 🎯 Main build orchestrator (single entry point)
├── CMakeLists.txt             # Root CMake configuration
├── .clang-format              # Code formatting rules
├── .clang-tidy                # Static analysis configuration
├── .gitignore                 # Updated for new artifact structure
│
├── MakeIndex/                 # 📁 Main source code
│   ├── CMakeLists.txt         # Target-specific CMake config
│   ├── MakeIndex.cpp          # Main application
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
├── 3rdParty/                  # 📚 Included dependencies
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
python please.py test

# Advanced test options
python please.py test --verbose --parallel auto
python please.py test --target MakeIndex_SelfTest
python please.py test --ci-mode --historical
python please.py test --coverage --report-format html
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
python please.py git-status

# 2. Pull latest changes
python please.py git-pull --rebase

# 3. Make your changes...

# 4. Pre-commit workflow
python please.py format              # Auto-format code
python please.py lint                 # Two-phase: auto-fix then report remaining issues
python please.py rebuild             # Full rebuild + test

# 5. Commit with validation
python please.py git-commit -m "Your changes"

# 6. Push changes
python please.py git-push
```

### Code Quality Standards

The build system enforces consistent code quality:

- **Formatting**: Automatic clang-format integration with project-specific style
- **Linting**: Two-phase clang-tidy analysis with automatic fixes followed by remaining issue reports
- **Testing**: Mandatory test execution before commits
- **Pre-commit Hooks**: Optional git hooks for automatic validation

#### Two-Phase Linting

The `python please.py lint` command runs in two phases for optimal developer experience:

1. **Phase 1 (Auto-fix)**: Runs clang-tidy with `--fix` to automatically correct common issues
2. **Phase 2 (Report)**: Runs clang-tidy again to report issues requiring manual attention

This approach reduces developer friction by handling routine fixes automatically while clearly highlighting issues that need thoughtful resolution.

### Performance Optimization

Monitor and optimize build performance:

```bash
# Analyze build performance
python please.py build-stats

# Manage caches (LLVM dependencies can be ~36GB)
python please.py cache-mgmt

# Clean specific caches
python please.py cache-mgmt --clean-cmake --clean-deps
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
python please.py rebuild --debug --skip-tests     # Quick build check
python please.py test --ci-mode                   # CI-style testing
python please.py format --check-only              # Format validation
python please.py lint --summary-only              # Quick two-phase lint check
```

## 🔧 Advanced Configuration

### Build Types

```bash
# Debug (default) - fast builds, debug symbols
python please.py configure --debug

# Release - optimized builds
python please.py configure --release

# Custom parallel jobs
python please.py build --parallel 8
```

### Dependency Management

LLVM 19.1.7 is automatically managed via CMake FetchContent:
- **First Build**: Downloads and builds LLVM (~30+ minutes)
- **Subsequent Builds**: Uses cached LLVM build
- **Cache Location**: `artifacts/debug/build/_deps/llvm-*`

### Tool Integration

```bash
# Generate compilation database for IDEs
python please.py compile-db --copy-to-root

# Install git pre-commit hooks
python please.py install-git-hooks

# Environment validation
python please.py info
```



## 🛠️ Troubleshooting

### Common Issues

**Issue**: Build fails with "linker out of heap space"
```bash
# Solution: Ensure 64-bit compiler environment
# On Windows, use: vcvars64.bat or VS 2022 x64 Native Tools Command Prompt
python please.py info  # Check compiler detection
```

**Issue**: LLVM build takes too long
```bash
# Solution: Use cached builds and parallel jobs
python please.py build --parallel 4  # Adjust for your CPU
python please.py cache-mgmt           # Check cache status
```

**Issue**: Git operations fail
```bash
# Solution: Check repository status
python please.py git-status          # Comprehensive status
python please.py git-clean --force   # Clean untracked files
```

**Issue**: Tests fail
```bash
# Solution: Check test output and rebuild
python please.py test --verbose      # Detailed test output
python please.py rebuild             # Clean rebuild
```

### Getting Help

```bash
# Comprehensive environment information
python please.py info

# Command-specific help
python please.py --help
python please.py <command> --help

# Build performance analysis
python please.py build-stats
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

🔄 **In Progress**: Core MakeIndex functionality expansion
📋 **Planned**: MCP service component

## 🤝 Contributing

1. **Fork** the repository
2. **Setup** development environment:
   ```bash
   python please.py setup
   python please.py install-git-hooks  # Optional but recommended
   ```
3. **Create** a feature branch:
   ```bash
   git checkout -b feature/amazing-feature
   ```
4. **Develop** with quality checks:
   ```bash
   # Make your changes...
   python please.py format              # Auto-format
   python please.py lint                 # Two-phase: auto-fix then report
   python please.py rebuild             # Build + test
   ```
5. **Commit** with validation:
   ```bash
   python please.py git-commit -m "Add amazing feature"
   ```
6. **Push** and create Pull Request:
   ```bash
   python please.py git-push --set-upstream
   ```

### Development Guidelines

- **Code Style**: Enforced via clang-format (LLVM style with customizations)
- **Quality**: All code must pass clang-tidy analysis
- **Testing**: Maintain or improve test coverage
- **Documentation**: Update documentation for user-facing changes

## 📞 Support

- **Issues**: Report bugs and request features on [GitHub Issues](https://github.com/your-org/CppGraphIndex/issues)
- **Discussions**: General questions and discussions on [GitHub Discussions](https://github.com/your-org/CppGraphIndex/discussions)
- **CI Status**: Check build status on [GitHub Actions](https://github.com/your-org/CppGraphIndex/actions)

---

> **🎯 Goal**: This project aims to bridge the gap between large C++ codebases and AI tools by providing a graph-based representation of code structure that can be queried using natural language.

> **⚡ Performance**: The build system provides fast builds, excellent dependency management, and comprehensive development workflow automation.