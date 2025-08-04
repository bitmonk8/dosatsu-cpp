# CppGraphIndex: Meson Build System Guide

This guide provides comprehensive documentation for building CppGraphIndex using the Meson build system with Conan package management.

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Quick Start](#quick-start)
4. [Detailed Setup](#detailed-setup)
5. [Build Configuration](#build-configuration)
6. [Development Tools](#development-tools)
7. [Development Scripts](#development-scripts)
8. [Troubleshooting](#troubleshooting)
9. [Advanced Features](#advanced-features)
10. [Performance Optimization](#performance-optimization)

## Overview

The Meson build system provides a modern, fast, and cross-platform alternative to XMake with the following advantages:

- **Automated Dependency Management**: Uses Conan for LLVM, DocTest, and other dependencies
- **Fast Builds**: Ninja backend with parallel compilation
- **Excellent IDE Integration**: Generate compile_commands.json for superior IDE support
- **Cross-Platform**: Automatic compiler detection and platform-specific configuration
- **Performance Features**: ccache support, LTO, and incremental builds

## Prerequisites

### Required Tools

1. **Meson** (≥1.8.3): Modern build system
   ```bash
   pip install meson
   # Or: https://mesonbuild.com/Getting-meson.html
   ```

2. **Ninja** (≥1.12.0): Fast build backend
   ```bash
   # Windows: Install via pip or download binary
   pip install ninja
   # Linux: sudo apt install ninja-build
   # macOS: brew install ninja
   ```

3. **Conan** (≥2.0): Package manager for C++ dependencies
   ```bash
   pip install conan
   ```

4. **Python** (≥3.8): For build scripts and Conan

### Compiler Requirements

The build system automatically detects and enforces the correct compiler for LLVM compatibility:

- **Windows**: MSVC (required for LLVM ABI compatibility)
- **Linux**: GCC (recommended)
- **macOS**: Clang (recommended)

#### Windows MSVC Setup

For Windows builds, ensure MSVC is properly configured. **This is required before any Meson build commands:**

```cmd
# REQUIRED: Setup Conan environment variables (includes MSVC setup)
conanvcvars.bat

# Alternative options:
# Option 1: Visual Studio Developer Command Prompt
# Launch "Developer Command Prompt" from Visual Studio

# Option 2: Manual vcvars setup
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# Option 3: Use the dev environment script
dev-env.bat
```

> **⚠️ Important**: On Windows, you MUST run `conanvcvars.bat` in each new command prompt before running any Meson build commands. This sets up the necessary MSVC environment variables.

## Quick Start

### Automated Build (Recommended)

```bash
# Windows: REQUIRED first step
conanvcvars.bat

# Then complete setup and build in one command
python tools/build.py full

# Or for release build
python tools/build.py full --build-type release
```

### Manual Step-by-Step

```bash
# Windows: REQUIRED first step
conanvcvars.bat

# 1. Install dependencies
python tools/setup-deps.py

# 2. Configure build
meson setup builddir

# 3. Build project
ninja -C builddir

# 4. Run tests
meson test -C builddir
```

## Detailed Setup

### 1. Dependency Installation

The setup script automatically handles Conan profile detection and dependency installation:

```bash
python tools/setup-deps.py
```

This will:
- Detect your system's Conan profile
- Install LLVM 19.1.7 and DocTest dependencies
- Generate Meson toolchain files
- Configure platform-specific settings

### 2. Meson Configuration

Configure the build system:

```bash
# Debug build (default)
meson setup builddir

# Release build
meson setup builddir_release --buildtype=release

# With specific options
meson setup builddir -Denable_lto=true -Dcompile_commands_json=true
```

### 3. Building

Build the project using Ninja:

```bash
# Build all targets
ninja -C builddir

# Build specific target
ninja -C builddir makeindex_exe

# Parallel build (automatic)
ninja -C builddir -j $(nproc)
```

## Build Configuration

### Build Types

- **debug**: Debug symbols, no optimization, runtime checks
- **release**: Optimized, minimal debug info
- **debugoptimized**: Debug symbols with optimization
- **minsize**: Size-optimized build

```bash
# Configure specific build type
meson setup builddir --buildtype=debug
meson setup builddir_release --buildtype=release
```

### Build Options

Configure build features via `meson_options.txt`:

```bash
# Enable all development features
meson setup builddir \
  -Denable_tests=true \
  -Denable_format=true \
  -Denable_lint=true \
  -Denable_lto=false \
  -Dcompile_commands_json=true \
  -Ddebug_symbols=true \
  -Doptimization_level=2
```

Available options:
- `enable_tests`: Build test targets (default: true)
- `enable_format`: Enable code formatting (default: true)
- `enable_lint`: Enable code linting (default: true)
- `enable_lto`: Link Time Optimization for release (default: false)
- `compile_commands_json`: Generate compilation database (default: true)
- `debug_symbols`: Include debug symbols (default: true)
- `optimization_level`: Optimization level 0-3 (default: 2)
- `llvm_version`: LLVM version to use (default: 19.1.7)

## Development Tools

### Code Formatting

Format source code using clang-format:

```bash
# Via Meson target
ninja -C builddir format

# Direct tool usage
python tools/format.py
```

### Code Linting

Run clang-tidy analysis:

```bash
# Via Meson target
ninja -C builddir lint

# Direct tool usage
python tools/lint.py

# Lint specific file
python tools/lint.py path/to/file.cpp
```

### Testing

Run the test suite:

```bash
# All tests
meson test -C builddir

# Verbose output
meson test -C builddir -v

# Direct executable
./builddir/MakeIndex/makeindex_exe --selftest
```

## Development Scripts

Convenience scripts in the `scripts/` directory provide simplified workflows:

### scripts/dev-build.py

Development build wrapper with multiple actions:

```bash
# Full build workflow (deps, setup, build, test)
python scripts/dev-build.py full

# Quick build (no deps/setup)
python scripts/dev-build.py quick

# Individual actions
python scripts/dev-build.py format
python scripts/dev-build.py lint
python scripts/dev-build.py test
python scripts/dev-build.py status
```

### scripts/setup-dev.py

Development environment setup:

```bash
# Complete setup
python scripts/setup-dev.py

# Dependencies only
python scripts/setup-dev.py --deps-only

# Specific build type
python scripts/setup-dev.py --type release
```

### scripts/clean.py

Clean build artifacts:

```bash
# Clean all build directories
python scripts/clean.py

# Clean dependencies too
python scripts/clean.py --deps
```

## Troubleshooting

### Common Issues

#### 1. Windows Build Failures (Most Common)

**Symptom**: Build fails with compiler not found or environment errors
**Solution**: Run `conanvcvars.bat` before any Meson commands

```cmd
# REQUIRED on Windows before any build commands
conanvcvars.bat

# Then proceed with build
python tools/build.py full
```

#### 2. LLVM Linking Errors

**Symptom**: Undefined symbols or ABI errors during linking
**Solution**: Ensure correct compiler is used

```bash
# Windows: Verify MSVC is active
cl.exe
# Should show Microsoft C/C++ Compiler

# Linux/macOS: Check compiler
gcc --version  # or clang --version
```

#### 3. Conan Profile Issues

**Symptom**: Conan cannot detect compiler or settings
**Solution**: Manual profile configuration

```bash
# Re-detect profile
conan profile detect --force

# Check profile
conan profile show default
```

#### 4. Meson Setup Errors

**Symptom**: Meson configuration fails
**Solution**: Clean and reconfigure

```bash
# Clean build directory
rm -rf builddir

# Clean Conan cache (if needed)
python scripts/clean.py --deps

# Reconfigure
python tools/setup-deps.py
meson setup builddir
```

#### 5. Ninja Build Failures

**Symptom**: Compilation errors during ninja build
**Solution**: Check compiler and dependencies

```bash
# Verify toolchain
meson setup builddir --wipe

# Check compile commands
ninja -C builddir -v
```

### Debug Information

Enable verbose output for troubleshooting:

```bash
# Verbose Meson setup
meson setup builddir --verbose

# Verbose build
ninja -C builddir -v

# Verbose tests
meson test -C builddir -v
```

### Environment Verification

Use the build tool's status command:

```bash
python tools/build.py status
```

This checks:
- Tool availability (meson, ninja, conan)
- Compiler configuration
- Dependency status
- Build directory state

## Advanced Features

### Cross-Compilation

The build system supports cross-compilation via Conan profiles:

```bash
# Create cross-compilation profile
conan profile detect --force --name cross-mingw

# Setup with cross profile
conan install . --profile:host=cross-mingw --profile:build=default
meson setup builddir --cross-file conan_meson_cross.ini
```

### Custom LLVM Versions

Override the LLVM version:

```bash
# Use different LLVM version
meson setup builddir -Dllvm_version=18.1.8
```

### IDE Integration

Generate IDE project files:

```bash
# VS Code with compile_commands.json
meson setup builddir -Dcompile_commands_json=true

# Visual Studio project
meson setup builddir --backend=vs

# Xcode project (macOS)
meson setup builddir --backend=xcode
```

## Performance Optimization

### Parallel Builds

Ninja automatically detects CPU cores and builds in parallel:

```bash
# Explicit parallel jobs
ninja -C builddir -j 8

# Use all available cores (default)
ninja -C builddir
```

### Compiler Cache (ccache)

Enable ccache for faster rebuilds (Linux/macOS):

```bash
# Install ccache
sudo apt install ccache  # Linux
brew install ccache      # macOS

# Meson automatically detects and uses ccache
meson setup builddir
```

**Note**: ccache is disabled on Windows to maintain MSVC compatibility.

### Link Time Optimization (LTO)

Enable LTO for release builds:

```bash
meson setup builddir_release --buildtype=release -Denable_lto=true
ninja -C builddir_release
```

### Incremental Builds

Ninja provides excellent incremental build support:

```bash
# Only rebuild changed files
ninja -C builddir

# Check what would be built
ninja -C builddir -n
```

### Build Performance Monitoring

Monitor build performance:

```bash
# Build with timing
time ninja -C builddir

# Ninja build statistics
ninja -C builddir -t graph | dot -Tpng > build-graph.png
```

## Environment Variables

Useful environment variables:

```bash
# Ninja settings
export NINJA_STATUS="[%f/%t] "  # Progress format

# Conan settings
export CONAN_USER_HOME="/path/to/conan"  # Custom Conan cache

# Meson settings
export MESON_TESTTHREADS=4  # Test parallelism
```

## Integration with XMake

Both build systems can coexist:

```bash
# Meson build
meson setup builddir && ninja -C builddir

# XMake build (in same project)
xmake
```

Build artifacts are in separate directories:
- Meson: `builddir/`
- XMake: `build/`

## Support and Resources

- **Meson Documentation**: https://mesonbuild.com/
- **Conan Documentation**: https://docs.conan.io/
- **Project Issues**: [GitHub Issues](https://github.com/your-repo/issues)
- **Migration Guide**: [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md)

For project-specific issues, see the troubleshooting section or open an issue on GitHub.