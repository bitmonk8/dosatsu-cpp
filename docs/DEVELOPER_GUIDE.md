# CppGraphIndex Developer Guide

This guide provides comprehensive information for developers working on CppGraphIndex using the modern Python + CMake + Ninja build system.

## 📋 Table of Contents

- [Development Environment Setup](#development-environment-setup)
- [Daily Development Workflow](#daily-development-workflow)
- [Build System Deep Dive](#build-system-deep-dive)
- [Testing Framework](#testing-framework)
- [Code Quality Standards](#code-quality-standards)
- [Git Integration](#git-integration)
- [Performance Optimization](#performance-optimization)
- [CI/CD Integration](#cicd-integration)
- [Troubleshooting](#troubleshooting)
- [Advanced Topics](#advanced-topics)

## 🚀 Development Environment Setup

### Prerequisites

Ensure you have the required tools installed:

```bash
# Check prerequisites
please info
```

**Required Tools:**
- **Python 3.8+**: Build orchestration
- **CMake 3.24+**: Build configuration  
- **Ninja**: Fast parallel builds
- **Git**: Version control and dependency management
- **Platform-specific compiler**:
  - Windows: MSVC (Visual Studio 2022+ recommended)
  - Linux: GCC or Clang
  - macOS: Clang (Xcode Command Line Tools)

### Initial Setup

```bash
# 1. Clone and navigate
git clone https://github.com/your-org/CppGraphIndex.git
cd CppGraphIndex

# 2. Environment setup (creates directories, validates tools)
please setup

# 3. Install git hooks (optional but recommended)
please install-git-hooks

# 4. Initial build
please rebuild --debug
```

### IDE Configuration

#### VS Code Setup

1. **Install Extensions:**
   - C/C++ Extension Pack
   - CMake Tools
   - Python

2. **Generate Compilation Database:**
   ```bash
   please compile-db --copy-to-root
   ```

3. **VS Code Settings** (`.vscode/settings.json`):
   ```json
   {
     "cmake.buildDirectory": "${workspaceFolder}/artifacts/debug/build",
     "C_Cpp.default.compileCommands": "${workspaceFolder}/compile_commands.json",
     "python.defaultInterpreterPath": "python"
   }
   ```

#### CLion Setup

1. **Open Project**: CLion will automatically detect CMakeLists.txt
2. **Build Directory**: Set to `artifacts/debug/build`
3. **Toolchain**: Ensure correct compiler is selected

### Environment Validation

```bash
# Comprehensive environment check
please info

# Expected output should show:
# ✓ Platform detection
# ✓ All required tools found
# ✓ Compiler information
# ✓ Build directories configured
```

## 🔄 Daily Development Workflow

### Standard Development Cycle

```bash
# 1. Start with status check
please git-status

# 2. Pull latest changes (with safety checks)
please git-pull --rebase

# 3. Make your code changes...

# 4. Code quality workflow
please format                    # Auto-format all code
please lint                       # Two-phase: auto-fix then report remaining issues
please build --debug             # Incremental build

# 5. Test changes
please test                       # Run full test suite
please test --target specific    # Run specific tests

# 6. Commit with validation
please git-commit -m "Your descriptive message"

# 7. Push changes
please git-push
```

### Rapid Development Mode

For quick iteration during development:

```bash
# Quick build + test cycle
please build --debug --parallel 8
please test --target MakeIndex_SelfTest

# Format check without auto-fix
please format --check-only

# Quick lint without detailed reports
please lint --summary-only
```

### Clean Build Workflow

When you need a clean slate:

```bash
# Full clean and rebuild
please rebuild --debug

# Or step by step:
please clean
please reconfigure --debug
please build --debug
please test
```

## 🏗️ Build System Deep Dive

### Architecture Overview

```
Python Build Orchestrator (please.py)
├── CMake + FetchContent + Ninja (Core Infrastructure)
├── Tools: clang-format, clang-tidy, git (Specialist Tools)
└── Artifact Management (Organized Output)
```

### Command Categories

#### Configuration Commands

```bash
# Basic configuration
please configure [--debug|--release] [--clean]

# Fresh configuration (cleans first)
please reconfigure [--debug|--release]

# Environment setup
please setup
```

#### Build Commands

```bash
# Standard build
please build [--debug|--release] [--target TARGET] [--parallel N]

# Integrated workflows
please rebuild                    # Clean + configure + build + test
please clean                      # Clean all artifacts
```

#### Analysis Commands

```bash
# Performance analysis
please build-stats                # Build size and performance metrics
please cache-mgmt                 # Cache management and optimization
please info                       # Environment information
```

### Artifact Organization

All build outputs are organized under `artifacts/`:

```
artifacts/
├── debug/
│   ├── build/              # CMake/Ninja build files
│   ├── bin/               # Debug executables
│   ├── lib/               # Debug libraries
│   └── logs/              # Build logs
├── release/               # Same structure for release
├── test/                  # Test results and reports
├── lint/                  # Linting outputs and reports
└── format/                # Formatting logs
```

### Dependency Management

LLVM 19.1.7 is managed via CMake FetchContent:

- **Location**: `artifacts/debug/build/_deps/llvm-*`
- **Size**: ~36GB when built
- **Build Time**: 30-60 minutes on first build
- **Caching**: Subsequent builds reuse cached LLVM

```bash
# Monitor dependency status
please cache-mgmt

# Clean dependency cache (forces rebuild)
please cache-mgmt --clean-deps
```

## 🧪 Testing Framework

### Test Types

The project uses **doctest** for C++ unit testing with multiple test configurations:

1. **MakeIndex_SelfTest**: Main unit tests
2. **MakeIndex_SelfTest_Verbose**: Detailed test output
3. **MakeIndex_BasicRun**: Smoke test (basic functionality)

### Running Tests

```bash
# Basic test execution
please test

# Advanced options
please test --verbose              # Detailed output
please test --parallel auto        # Parallel execution
please test --target "SelfTest"    # Specific test pattern
please test --ci-mode              # CI-friendly output
```

### Test Reporting

Tests generate comprehensive reports:

```bash
# Enable detailed reporting
please test --report-format html --historical

# Generated reports:
# - artifacts/test/results.xml         (JUnit format)
# - artifacts/test/test-report.html    (Rich HTML report)
# - artifacts/test/test-report.json    (Machine-readable)
# - artifacts/test/test-history.json   (Historical tracking)
# - artifacts/test/test-trends.txt     (Performance trends)
```

### Writing Tests

Tests are embedded in source files using doctest macros:

```cpp
#include "doctest/doctest.h"

TEST_CASE("Example test") {
    CHECK(1 + 1 == 2);
    REQUIRE_NOTHROW(some_function());
}
```

## 🎯 Code Quality Standards

### Formatting Standards

The project uses **clang-format** with LLVM style and customizations:

```bash
# Auto-format all code
please format

# Check formatting without changes
please format --check-only

# Format specific files
please format --files src/file1.cpp src/file2.h
```

**Configuration**: `.clang-format` (LLVM base with project customizations)

### Linting Standards

**clang-tidy** enforces modern C++ guidelines and project-specific rules using a two-phase approach:

```bash
# Run two-phase linting (auto-fix then report)
please lint

# Quick summary without detailed output
please lint --summary-only

# Generate markdown report
please lint --report-format markdown

# Target specific files
please lint --files src/file1.cpp src/file2.cpp
please lint --target src/specific_file.cpp
```

**Configuration**: `.clang-tidy` (comprehensive rule set)

#### Two-Phase Linting Process

The lint command automatically runs in two phases for optimal developer experience:

**Phase 1: Automatic Fixes**
- Runs `clang-tidy` with `--fix` and `--fix-errors` flags
- Automatically fixes issues that can be safely corrected
- Output saved to `artifacts/lint/clang-tidy-phase1-autofix.txt`

**Phase 2: Report Remaining Issues**  
- Runs `clang-tidy` without fix flags to identify remaining problems
- Reports issues that require manual developer attention
- Output saved to `artifacts/lint/clang-tidy-raw.txt`
- Comprehensive analysis report generated in `artifacts/lint/analysis-report.md`

**Example Output Flow**:
```bash
please lint

# Phase 1: Applied automatic fixes to 12 issues
# Phase 2: 3 issues require manual attention
#   - 2 warnings about code style
#   - 1 error requiring code restructuring
```

**Benefits**:
- **Reduced Friction**: Common issues fixed automatically
- **Clear Focus**: Developers only see issues requiring attention  
- **Full Transparency**: Both phases logged for review
- **No Lost Information**: All original issues captured before fixes

### Pre-commit Hooks

Install git hooks for automatic quality checks:

```bash
# Install hooks
please install-git-hooks

# Hooks will automatically:
# 1. Check formatting on staged C++ files
# 2. Prevent commits with formatting issues
# 3. Provide fix suggestions
```

### Quality Metrics

Monitor code quality with detailed reports:

```bash
# View linting statistics
please lint --summary-only

# Generates reports:
# - artifacts/lint/analysis-report.md    (Comprehensive analysis)
# - artifacts/lint/clang-tidy-raw.txt    (Raw tool output)
```

## 🔧 Git Integration

### Enhanced Git Commands

The build system provides intelligent git integration:

```bash
# Enhanced status with build context
please git-status

# Safe pull with clean directory checks
please git-pull --check-clean --rebase

# Smart push with validation
please git-push --set-upstream

# Commit with pre-commit checks
please git-commit -m "Message" [--skip-checks] [--force]

# Repository cleanup
please git-clean --force --include-build-artifacts
```

### Git Workflow Integration

```bash
# Typical workflow
please git-status                 # Check current state
please git-pull --rebase          # Get latest changes
# ... make changes ...
please format                     # Format code
please git-commit -m "Fix bug"    # Commit with validation
please git-push                   # Push changes
```

### Pre-commit Validation

When using `please git-commit`, automatic checks run:

1. **Format Check**: Validates code formatting
2. **Build Check**: Ensures code compiles
3. **Lint Check**: Basic static analysis

Override with `--skip-checks` or `--force` if needed.

## ⚡ Performance Optimization

### Build Performance

Monitor and optimize build performance:

```bash
# Analyze build performance
please build-stats

# Sample output:
# Debug Build Statistics:
#   Build directory size: 35.99 GB
#   Dependencies size: 35.95 GB
#   Output size: 693.44 MB
#   Configuration time: 2025-08-11 17:24:52
#   Last build time: 2025-08-11 17:24:55
```

### Cache Management

Optimize disk usage and build times:

```bash
# View cache status
please cache-mgmt

# Clean specific caches
please cache-mgmt --clean-cmake    # CMake caches
please cache-mgmt --clean-deps     # Dependency caches

# Optimize cache storage
please cache-mgmt --optimize
```

### Parallel Build Configuration

```bash
# Use all available cores
please build --parallel auto

# Specify core count
please build --parallel 8

# For CI/low-memory systems
please build --parallel 2
```

### Incremental Build Optimization

```bash
# For development, use incremental builds
please build --debug               # Only builds changes

# For clean builds when needed
please rebuild --debug             # Full clean rebuild
```

## 🚀 CI/CD Integration

### GitHub Actions Workflow

The project includes comprehensive CI/CD:

**Platforms**: Windows, Linux, macOS
**Build Types**: Debug, Release
**Features**:
- Multi-platform matrix builds
- LLVM dependency caching
- Test execution with reporting
- Code quality checks
- Security scanning (CodeQL)
- Artifact collection

### Local CI Simulation

Test CI-like conditions locally:

```bash
# Simulate CI build process
please clean
please configure --debug --clean
please build --debug --parallel 2
please test --ci-mode
please format --check-only
please lint --summary-only
```

### CI-Specific Commands

```bash
# CI-friendly test execution
please test --ci-mode --historical --report-format html

# Quick validation for PR checks
please format --check-only --files changed_files.cpp
please build --debug --target MakeIndex
```

## 🛠️ Troubleshooting

### Common Build Issues

#### Issue: Linker Heap Space Error (Windows)

**Symptoms**: Build fails with "linker out of heap space"
**Cause**: 32-bit compiler environment with large LLVM dependency

**Solution**:
```bash
# Use 64-bit Visual Studio environment
# Option 1: VS 2022 x64 Native Tools Command Prompt

# Option 2: Manual setup
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
please reconfigure --debug
```

#### Issue: LLVM Build Too Slow

**Symptoms**: Initial build takes 1+ hours
**Cause**: LLVM is large (~36GB) and compilation-intensive

**Solutions**:
```bash
# Use maximum parallel jobs
please build --parallel auto

# Monitor build progress
please build-stats

# Use cached CI artifacts (if available)
# Download artifacts from successful CI runs
```

#### Issue: Git Operations Fail

**Symptoms**: Git commands return errors
**Cause**: Repository state issues

**Solutions**:
```bash
# Check repository status
please git-status

# Clean repository
please git-clean --force --include-directories

# Reset to clean state (CAREFUL - loses changes)
git reset --hard HEAD
please git-clean --force
```

#### Issue: Test Failures

**Symptoms**: Tests fail unexpectedly
**Cause**: Build issues or environment problems

**Solutions**:
```bash
# Get detailed test output
please test --verbose

# Clean rebuild and test
please rebuild --debug

# Check test artifacts
ls artifacts/test/
cat artifacts/test/test-output.log
```

### Performance Issues

#### Issue: Slow Builds

**Analysis**:
```bash
# Check build statistics
please build-stats

# Check cache status
please cache-mgmt
```

**Solutions**:
```bash
# Increase parallel jobs
please build --parallel 8

# Clean unnecessary caches
please cache-mgmt --clean-cmake

# Use incremental builds
please build  # Instead of rebuild
```

#### Issue: Large Disk Usage

**Analysis**:
```bash
# Check artifact sizes
please build-stats
please cache-mgmt
```

**Solutions**:
```bash
# Clean build artifacts
please clean

# Clean dependency caches (will trigger rebuild)
please cache-mgmt --clean-deps

# Clean unused configurations
rm -rf artifacts/release  # If only using debug
```

### Environment Issues

#### Issue: Tool Detection Failures

**Symptoms**: `please info` shows missing tools
**Cause**: Tools not in PATH or incorrect versions

**Solutions**:
```bash
# Check PATH and tool versions
cmake --version          # Should be 3.24+
ninja --version          # Should be 1.11+
git --version            # Any recent version
python --version         # Should be 3.8+

# On Windows, ensure proper VS environment
where cl                 # Should show MSVC compiler
```

#### Issue: Permission Errors

**Symptoms**: Build fails with permission denied
**Cause**: File system permissions or antivirus interference

**Solutions**:
```bash
# Check directory permissions
# Ensure artifacts/ directory is writable

# Windows: Add build directory to antivirus exclusions
# Path to exclude: C:\path\to\CppGraphIndex\artifacts
```

## 📚 Advanced Topics

### Custom Build Configurations

#### Adding New Build Types

Extend CMakeLists.txt for custom configurations:

```cmake
# In root CMakeLists.txt
if(CMAKE_BUILD_TYPE STREQUAL "ProfileRelease")
    set(CMAKE_CXX_FLAGS_PROFILERELEASE "-O2 -g -pg")
endif()
```

```bash
# Use custom build type
please configure --release
# Then manually: cmake -DCMAKE_BUILD_TYPE=ProfileRelease artifacts/release/build
```

#### Custom Compiler Flags

```bash
# Environment variable approach
export CXXFLAGS="-march=native"
please reconfigure --release
```

### Integration with External Tools

#### ccache Integration (Linux/macOS)

```bash
# Install ccache
# Linux: sudo apt-get install ccache
# macOS: brew install ccache

# Configure CMake to use ccache
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
please reconfigure --debug
```

#### Custom Linting Rules

Modify `.clang-tidy` for project-specific rules:

```yaml
# Add custom checks
Checks: '-*,your-custom-check,modernize-*'

# Custom check options
CheckOptions:
  your-custom-check.Option: value
```

### Debugging Build Issues

#### CMake Debug Mode

```bash
# Verbose CMake configuration
cmake -S . -B artifacts/debug/build --debug-output

# Or with build script
please configure --debug -v  # If verbose flag exists
```

#### Ninja Debug Mode

```bash
# Verbose Ninja builds
ninja -C artifacts/debug/build -v

# Check build dependency graph
ninja -C artifacts/debug/build -t graph | dot -Tpng > deps.png
```

#### Dependency Analysis

```bash
# Check LLVM build status
ls -la artifacts/debug/build/_deps/

# Monitor dependency build
tail -f artifacts/debug/logs/build.log
```

### Performance Profiling

#### Build Time Analysis

```bash
# Time individual commands
time please configure --debug
time please build --debug

# Profile with detailed timing
cmake --build artifacts/debug/build --verbose
```

#### Memory Usage Monitoring

```bash
# Monitor during build (Linux/macOS)
top -p $(pgrep ninja)

# Windows Task Manager or Process Monitor
# Monitor cmake.exe and ninja.exe processes
```

### Contributing to Build System

The build system itself is modular and extensible:

#### Adding New Commands

1. **Add command method** to `BuildOrchestrator` class in `please.py`
2. **Add argument parser** in `create_parser()` function  
3. **Add command mapping** in `command_map` dictionary
4. **Test new functionality**
5. **Update documentation**

#### Extending CI/CD

Modify `.github/workflows/ci.yml` for additional:
- **Platforms**: Add new OS configurations
- **Tools**: Integrate additional analysis tools
- **Reporting**: Add new report formats
- **Deployment**: Add release automation

---

This developer guide provides comprehensive information for working effectively with the CppGraphIndex build system. For specific issues not covered here, please check the [Troubleshooting](#troubleshooting) section or create an issue on GitHub.
