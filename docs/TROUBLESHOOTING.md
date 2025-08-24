# Dosatsu Troubleshooting Guide

This guide helps resolve common issues with the Dosatsu build system and development environment.

## 📋 Quick Diagnostics

Before diving into specific issues, run these diagnostic commands:

```bash
# Environment validation
please info

# Git repository status
please git-status

# Build performance analysis
please build-stats

# Cache status
please cache-mgmt
```

## 🛠️ Common Build Issues

### Issue: Linker Heap Space Error (Windows)

**Error Message:**
```
LINK : the 32-bit linker ran out of heap space and is going to restart linking with a 64-bit linker
LINK : fatal error LNK1318: Unexpected PDB error; OK (0)
```

**Root Cause:** Using 32-bit compiler environment with large LLVM dependency (~36GB)

**Solutions:**

1. **Use 64-bit Visual Studio Environment** (Recommended):
   ```bash
   # Option 1: Open "x64 Native Tools Command Prompt for VS 2022"
   # Then navigate to project and run builds
   
   # Option 2: Activate 64-bit environment in current shell
   call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
   please reconfigure --debug
   please build --debug
   ```

2. **Verify 64-bit Environment:**
   ```bash
   please info
   # Look for: "cl.exe" path contains "HostX64\x64"
   # Should NOT contain "HostX86\x86"
   ```

3. **Alternative: Use Visual Studio IDE:**
   - Open Visual Studio 2022
   - Ensure platform is set to "x64"
   - Build through IDE instead of command line

### Issue: LLVM Build Takes Too Long

**Symptoms:** Initial build takes 30+ minutes to several hours

**Root Cause:** LLVM is a large dependency (~36GB compiled) that needs to be built from source

**Solutions:**

1. **Optimize Parallel Building:**
   ```bash
   # Use all available CPU cores
   please build --parallel auto
   
   # Or specify core count (recommended: cores - 1)
   please build --parallel 7  # For 8-core system
   ```

2. **Monitor Build Progress:**
   ```bash
   # Check build statistics
   please build-stats
   
   # Monitor build logs (separate terminal)
   tail -f artifacts/debug/logs/build.log  # Linux/macOS
   Get-Content artifacts/debug/logs/build.log -Wait  # Windows PowerShell
   ```

3. **System Optimization:**
   ```bash
   # Ensure sufficient RAM (16GB+ recommended)
   # Use SSD storage for faster I/O
   # Close unnecessary applications during build
   ```

4. **Use Cached Builds:**
   ```bash
   # Check if cache exists
   please cache-mgmt
   
   # On CI: Download artifacts from successful builds
   # Copy _deps directory from successful build machine
   ```

### Issue: CMake Configuration Fails

**Error Examples:**
```
CMake Error: Could not find CMAKE_C_COMPILER
CMake Error: The C compiler is not able to compile a simple test program
```

**Solutions:**

1. **Windows - MSVC Not Found:**
   ```bash
   # Install Visual Studio 2022 with C++ workload
   # Or Visual Studio Build Tools
   
   # Verify installation
   where cl
   # Should show: C:\Program Files\Microsoft Visual Studio\...\cl.exe
   ```

2. **Linux - GCC Not Found:**
   ```bash
   # Ubuntu/Debian
   sudo apt-get update
   sudo apt-get install build-essential cmake ninja-build
   
   # CentOS/RHEL/Fedora
   sudo yum groupinstall "Development Tools"
   sudo yum install cmake ninja-build
   ```

3. **macOS - Xcode Tools Missing:**
   ```bash
   # Install Xcode Command Line Tools
   xcode-select --install
   
   # Verify installation
   clang --version
   ```

4. **Path Issues:**
   ```bash
   # Verify tools in PATH
   please info
   
   # Check specific tools
   cmake --version    # Should be 3.24+
   ninja --version    # Should be 1.11+
   ```

### Issue: Git Operations Fail

**Error Examples:**
```
fatal: not a git repository
warning: LF will be replaced by CRLF
```

**Solutions:**

1. **Repository Not Initialized:**
   ```bash
   # Check if in git repository
   please git-status
   
   # If not, initialize
   git init
   git remote add origin <repository-url>
   ```

2. **Line Ending Issues (Windows):**
   ```bash
   # Configure git line endings
   git config --global core.autocrlf true
   
   # Or for this repository only
   git config core.autocrlf true
   ```

3. **Permission Issues:**
   ```bash
   # Check git configuration
   git config --list
   
   # Set user if needed
   git config user.name "Your Name"
   git config user.email "your.email@example.com"
   ```

## 🧪 Test Issues

### Issue: Tests Fail to Execute

**Error Message:**
```
Unable to find executable: dosatsu_cpp.exe
Errors while running CTest
```

**Root Cause:** Executable not built or not in expected location

**Solutions:**

1. **Build First:**
   ```bash
   please build --debug
   
   # Verify executable exists
   ls artifacts/debug/bin/Dosatsu*
   ```

2. **Check Build Configuration:**
   ```bash
   please configure --debug
   please build --debug
   please test
   ```

3. **Manual Test Execution:**
   ```bash
   # Run executable directly
   artifacts/debug/bin/dosatsu_cpp.exe --selftest  # Windows
   ./artifacts/debug/bin/Dosatsu --selftest     # Linux/macOS
   ```

### Issue: Test Failures

**Symptoms:** Tests run but fail with assertion errors

**Solutions:**

1. **Get Detailed Output:**
   ```bash
   please test --verbose
   
   # Check test logs
   cat artifacts/test/test-output.log
   ```

2. **Run Specific Tests:**
   ```bash
   # Run individual test
   please test --target Dosatsu_SelfTest
   
   # Run with maximum verbosity
   artifacts/debug/bin/dosatsu_cpp.exe --selftest --verbose
   ```

3. **Clean Rebuild:**
   ```bash
   please rebuild --debug
   ```

## ⚡ Performance Issues

### Issue: Slow Incremental Builds

**Symptoms:** Even small changes trigger long builds

**Analysis:**
```bash
# Check what's being rebuilt
please build --debug --verbose

# Check dependencies
ninja -C artifacts/debug/build -t graph
```

**Solutions:**

1. **Optimize Dependencies:**
   ```bash
   # Check for circular dependencies
   # Minimize header includes
   # Use forward declarations where possible
   ```

2. **Parallel Building:**
   ```bash
   # Increase parallel jobs
   please build --parallel 8
   ```

3. **Incremental vs Full Builds:**
   ```bash
   # Use incremental builds for development
   please build --debug
   
   # Use full rebuilds only when necessary
   please rebuild --debug
   ```

### Issue: Excessive Disk Usage

**Symptoms:** Build directory grows very large (>50GB)

**Analysis:**
```bash
please build-stats
please cache-mgmt
```

**Solutions:**

1. **Clean Unnecessary Artifacts:**
   ```bash
   # Clean build artifacts
   please clean
   
   # Clean specific caches
   please cache-mgmt --clean-cmake
   ```

2. **Selective Cache Cleaning:**
   ```bash
   # Clean LLVM cache (will trigger rebuild)
   please cache-mgmt --clean-deps
   
   # Clean only CMake files (faster)
   please cache-mgmt --clean-cmake
   ```

3. **Storage Optimization:**
   ```bash
   # Use only needed build types
   # Remove release builds if only debugging
   rm -rf artifacts/release
   
   # Use external storage for large caches
   # Symlink artifacts/debug/build/_deps to external drive
   ```

## 🔧 Environment Issues

### Issue: Python Version Problems

**Error Message:**
```
Python 3.8+ required, found 3.7.x
```

**Solutions:**

1. **Install Python 3.8+:**
   ```bash
   # Windows: Download from python.org
   # Linux: sudo apt-get install python3.8 python3.8-pip
   # macOS: brew install python@3.8
   ```

2. **Virtual Environment:**
   ```bash
   # Create virtual environment with correct Python
   python3.8 -m venv venv
   source venv/bin/activate  # Linux/macOS
   venv\Scripts\activate.bat # Windows
   
   # Then use build system
   please setup
   ```

### Issue: Tool Version Mismatches

**Symptoms:** Build fails with version-related errors

**Solutions:**

1. **Check Tool Versions:**
   ```bash
   please info
   
   # Manual verification
   cmake --version    # Needs 3.24+
   ninja --version    # Needs 1.11+
   python --version   # Needs 3.8+
   ```

2. **Update Tools:**
   ```bash
   # Windows: Use installer or package manager
   # Linux: sudo apt-get update && sudo apt-get upgrade
   # macOS: brew update && brew upgrade
   ```

### Issue: Permission Denied Errors

**Error Examples:**
```
Permission denied: artifacts/debug/build
PermissionError: [Errno 13] Permission denied
```

**Solutions:**

1. **Windows - Antivirus Interference:**
   ```bash
   # Add build directory to antivirus exclusions
   # Path: C:\path\to\Dosatsu\artifacts
   ```

2. **Linux/macOS - File Permissions:**
   ```bash
   # Check permissions
   ls -la artifacts/
   
   # Fix permissions
   chmod -R u+w artifacts/
   ```

3. **File Locks:**
   ```bash
   # Close IDE and running processes
   # Windows: Use Process Explorer to find file locks
   # Linux: lsof | grep Dosatsu
   ```

## 🔍 Advanced Diagnostics

### Debug Mode Builds

Enable verbose output for detailed diagnostics:

```bash
# CMake debug output
cmake -S . -B artifacts/debug/build --debug-output

# Ninja verbose builds
ninja -C artifacts/debug/build -v

# Build system debug (if available)
please build --debug --verbose
```

### Log Analysis

Examine build logs for specific issues:

```bash
# Build logs
cat artifacts/debug/logs/build.log

# Test logs
cat artifacts/test/test-output.log

# Lint logs
cat artifacts/lint/clang-tidy-raw.txt

# Format logs
cat artifacts/format/format-check.log
```

### Network and Proxy Issues

If dependency downloads fail:

```bash
# Check network connectivity
curl -I https://github.com/llvm/llvm-project/releases/

# Configure git proxy (if needed)
git config --global http.proxy http://proxy:port
git config --global https.proxy https://proxy:port

# Configure CMake proxy (if needed)
export https_proxy=https://proxy:port
```

## 🚨 Emergency Recovery

### Complete Clean Start

If everything is broken, start fresh:

```bash
# 1. Stop all build processes
# Ctrl+C or close terminals

# 2. Clean all artifacts
please clean
# Or manually: rm -rf artifacts/

# 3. Clean git (CAREFUL - loses changes)
please git-clean --force --include-directories
# Or manually: git clean -fdx

# 4. Fresh start
please setup
please rebuild --debug
```

### Reset to Known Good State

```bash
# 1. Reset to last known good commit
git log --oneline  # Find good commit hash
git reset --hard <good-commit-hash>

# 2. Clean rebuild
please clean
please rebuild --debug
```

## 📞 Getting Help

### Information Gathering

When reporting issues, include:

```bash
# System information
please info > system-info.txt

# Build statistics
please build-stats > build-stats.txt

# Repository status
please git-status > git-status.txt

# Cache status
please cache-mgmt > cache-status.txt
```

### Common Support Channels

1. **GitHub Issues**: For bugs and feature requests
2. **GitHub Discussions**: For questions and general help
3. **Documentation**: Check README.md and docs/
4. **CI Logs**: Check GitHub Actions for similar failures

### Issue Report Template

```markdown
## Problem Description
[Brief description of the issue]

## Environment
- OS: [Windows 10/Ubuntu 20.04/macOS 12.0]
- Python: [3.9.7]
- CMake: [3.24.0]
- Compiler: [MSVC 19.43/GCC 11.2/Clang 14.0]

## Steps to Reproduce
1. [First step]
2. [Second step]
3. [Issue occurs]

## Expected Behavior
[What should happen]

## Actual Behavior
[What actually happens]

## Logs and Output
```
[paste relevant log output]
```

## System Information
[Output of: please info]
```

This troubleshooting guide covers the most common issues. For specific problems not covered here, please create a GitHub issue with detailed information about your environment and the specific error you're encountering.
