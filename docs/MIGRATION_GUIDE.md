# XMake to Meson Migration Guide

This guide helps developers migrate from XMake to Meson build system for CppGraphIndex while maintaining full functionality and workflow compatibility.

## Table of Contents

1. [Migration Overview](#migration-overview)
2. [Quick Migration](#quick-migration)
3. [Command Mapping](#command-mapping)
4. [Feature Comparison](#feature-comparison)
5. [Workflow Migration](#workflow-migration)
6. [Dependency Management](#dependency-management)
7. [IDE Integration](#ide-integration)
8. [Troubleshooting Migration](#troubleshooting-migration)
9. [Parallel Usage](#parallel-usage)
10. [Complete Transition](#complete-transition)

## Migration Overview

### Why Migrate to Meson?

- **Modern Build System**: Python-based, actively developed
- **Better Dependency Management**: Automated via Conan
- **Superior IDE Integration**: Excellent LSP and IDE support
- **Performance**: Fast parallel builds with Ninja
- **Cross-Platform**: Robust platform detection and configuration

### Migration Strategy

This project supports **gradual migration** - both build systems work simultaneously:

1. **Phase 1**: Set up Meson alongside XMake (parallel usage)
2. **Phase 2**: Learn Meson workflows and commands
3. **Phase 3**: Gradually shift development to Meson
4. **Phase 4**: Complete transition (optional)

## Quick Migration

### 30-Second Setup

```bash
# Install tools (if not already available)
pip install meson ninja conan

# Windows only: Setup MSVC environment (REQUIRED)
conanvcvars.bat

# Setup Meson build
python tools/build.py full

# You're ready! Both systems now work
```

### First Build Comparison

| Task | XMake | Meson |
|------|-------|-------|
| Build | `xmake` | `ninja -C builddir` |
| Test | `xmake test` | `meson test -C builddir` |
| Format | `xmake run format` | `ninja -C builddir format` |
| Lint | `xmake run lint` | `ninja -C builddir lint` |

## Command Mapping

### Core Build Commands

| Operation | XMake | Meson Equivalent |
|-----------|-------|------------------|
| **Setup/Configure** | `xmake f -m debug` | `meson setup builddir --buildtype=debug` |
| **Build** | `xmake` or `xmake build` | `ninja -C builddir` |
| **Clean** | `xmake clean` | `ninja -C builddir clean` |
| **Rebuild** | `xmake rebuild` | `ninja -C builddir clean && ninja -C builddir` |

### Build Modes

| Mode | XMake | Meson |
|------|-------|-------|
| **Debug** | `xmake f -m debug && xmake` | `meson setup builddir --buildtype=debug && ninja -C builddir` |
| **Release** | `xmake f -m release && xmake` | `meson setup builddir_release --buildtype=release && ninja -C builddir_release` |

### Development Tools

| Tool | XMake | Meson |
|------|-------|-------|
| **Format** | `xmake run format` | `ninja -C builddir format` |
| **Lint** | `xmake run lint` | `ninja -C builddir lint` |
| **Test** | `xmake test` | `meson test -C builddir` |
| **Run** | `xmake run MakeIndex` | `./builddir/MakeIndex/makeindex_exe` |

### Advanced Commands

| Operation | XMake | Meson |
|-----------|-------|-------|
| **Show Config** | `xmake f --show` | `meson configure builddir` |
| **Verbose Build** | `xmake -v` | `ninja -C builddir -v` |
| **Parallel Build** | `xmake -j4` | `ninja -C builddir -j4` |

## Feature Comparison

### Complete Feature Matrix

| Feature | XMake | Meson | Notes |
|---------|-------|-------|-------|
| **C++20 Support** | ✅ | ✅ | Same configuration |
| **Debug/Release** | ✅ | ✅ | Same functionality |
| **Code Formatting** | ✅ | ✅ | Uses same .clang-format |
| **Code Linting** | ✅ | ✅ | Uses same .clang-tidy |
| **Unit Testing** | ✅ | ✅ | Same doctest integration |
| **LLVM Integration** | ✅ | ✅ | Same 241 libraries |
| **Cross-Platform** | ✅ | ✅ | Windows/Linux/macOS |
| **Dependency Mgmt** | Manual | Automated | Conan integration |
| **IDE Integration** | Good | Excellent | compile_commands.json |
| **Build Speed** | Fast | Fast | Ninja backend |
| **Cache Support** | No | Yes | ccache on Linux/macOS |

## Workflow Migration

### Daily Development Workflow

#### XMake Workflow
```bash
# Edit code
xmake                    # Build
xmake test              # Test  
xmake run format        # Format
xmake run lint          # Lint
git add . && git commit # Commit
```

#### Meson Equivalent
```bash
# Edit code
ninja -C builddir           # Build
meson test -C builddir      # Test
ninja -C builddir format    # Format
ninja -C builddir lint      # Lint
git add . && git commit     # Commit
```

#### Meson Convenience Scripts
```bash
# Edit code
python scripts/dev-build.py quick   # Build + test
python scripts/dev-build.py format  # Format
python scripts/dev-build.py lint    # Lint
git add . && git commit             # Commit
```

### Project Setup Workflow

#### New Developer Setup (XMake)
```bash
git clone <repo>
cd CppGraphIndex
xmake                   # Build (downloads deps automatically)
xmake test             # Verify
```

#### New Developer Setup (Meson)
```bash
git clone <repo>
cd CppGraphIndex
# Windows only: Setup MSVC environment (REQUIRED)
conanvcvars.bat
python tools/build.py full  # Setup deps + build
# Or step by step:
python tools/setup-deps.py  # Install dependencies
meson setup builddir        # Configure
ninja -C builddir           # Build
meson test -C builddir      # Verify
```

## Dependency Management

### XMake Approach
- **LLVM**: Built from source automatically
- **DocTest**: Included in 3rdParty/
- **System**: Automatic detection

### Meson Approach  
- **LLVM**: Downloaded via Conan (llvm-core/19.1.7)
- **DocTest**: Downloaded via Conan (doctest/2.4.11)
- **System**: Conan profile detection

### Migration Benefits
- **Faster Setup**: Pre-built LLVM binaries instead of source compilation
- **Version Control**: Explicit dependency versions in conanfile.py
- **Reproducible Builds**: Same dependencies across all environments
- **Easier Updates**: Simple version bumps in configuration

## IDE Integration

### XMake IDE Support
- Basic compile_commands.json generation
- Works with most IDEs
- Manual configuration often needed

### Meson IDE Support
- Excellent compile_commands.json generation
- Native IDE project generation (VS, Xcode)
- Superior LSP integration
- Better IntelliSense/code completion

### IDE Setup Migration

#### VS Code
```bash
# XMake: Manual compile_commands.json
xmake project -k compile_commands

# Meson: Automatic generation
meson setup builddir -Dcompile_commands_json=true
# File automatically available at builddir/compile_commands.json
```

#### Visual Studio
```bash
# Meson: Native VS project generation
meson setup builddir --backend=vs
# Open builddir/CppGraphIndex.sln
```

#### CLion/IntelliJ
- **XMake**: Import via compile_commands.json
- **Meson**: Native Meson project import

## Troubleshooting Migration

### Common Migration Issues

#### 1. Windows Environment Setup (Most Common)
**Problem**: Build fails on Windows with environment or compiler errors
**Solution**: Run `conanvcvars.bat` before any Meson commands
```cmd
# REQUIRED on Windows before any Meson commands
conanvcvars.bat

# Then proceed with migration
python tools/build.py full
```

#### 2. "Command Not Found" Errors
**Problem**: `meson` or `ninja` not in PATH
**Solution**:
```bash
pip install meson ninja
# Or use system package manager
```

#### 3. LLVM Linking Issues
**Problem**: Different LLVM versions or ABI mismatch
**Solution**: Meson automatically handles ABI compatibility
```bash
# Meson enforces correct compiler for LLVM compatibility
# Windows: MSVC required, Linux: GCC preferred, macOS: Clang preferred
```

#### 4. Dependency Resolution Failures
**Problem**: Conan cannot find dependencies
**Solution**:
```bash
# Update Conan profile
conan profile detect --force
python tools/setup-deps.py
```

#### 5. Path/Directory Confusion
**Problem**: Different build directories
**Solution**: 
- XMake uses `build/`
- Meson uses `builddir/` (configurable)
- Both can coexist

### Migration Verification

Verify both systems work identically:

```bash
# Build with both systems
xmake clean && xmake
ninja -C builddir clean && ninja -C builddir

# Compare outputs
xmake test
meson test -C builddir

# Both should pass identically
```

## Parallel Usage

### Running Both Systems

Both build systems can operate simultaneously:

```bash
# XMake build
xmake
ls build/        # XMake artifacts

# Meson build  
ninja -C builddir
ls builddir/     # Meson artifacts

# Independent operation - no conflicts
```

### Choosing Per Task

Use the best tool for each task:

```bash
# Quick format (direct tool, no build system)
python tools/format.py

# Quick lint (direct tool)
python tools/lint.py

# Full development workflow
python scripts/dev-build.py full
```

### Team Migration

For team environments:

1. **Phase 1**: Introduce Meson as optional
2. **Phase 2**: Document both workflows
3. **Phase 3**: Gradually encourage Meson adoption
4. **Phase 4**: Sunset XMake (optional)

## Complete Transition

### When to Fully Migrate

Consider complete migration when:
- Team is comfortable with Meson workflows
- Performance benefits are realized
- Dependency management advantages are clear
- IDE integration improvements are valued

### Transition Steps

1. **Backup**: Ensure XMake still works
```bash
git branch backup-xmake-working
```

2. **Team Training**: Ensure everyone knows Meson commands

3. **CI/CD Update**: Update build scripts to use Meson

4. **Documentation**: Update team documentation

5. **Optional Cleanup**: Remove XMake files (after team consensus)
```bash
# Only after team agreement
rm xmake.lua
rm -rf .xmake/ build/
```

### Rollback Plan

If issues arise:
```bash
# Switch back to XMake
git checkout backup-xmake-working
xmake clean && xmake
```

## Migration Checklist

### Pre-Migration
- [ ] Install Meson, Ninja, Conan
- [ ] Verify compiler setup (MSVC on Windows)
- [ ] Backup current working XMake setup

### Initial Setup
- [ ] Run `python tools/build.py` successfully
- [ ] Verify `meson test -C builddir` passes
- [ ] Compare build outputs with XMake

### Workflow Learning
- [ ] Practice basic Meson commands
- [ ] Test development tools (format, lint)
- [ ] Setup IDE integration

### Team Integration
- [ ] Document team-specific workflows
- [ ] Train team members on Meson
- [ ] Update CI/CD if applicable

### Optional Transition
- [ ] Team consensus on full migration
- [ ] Update primary documentation
- [ ] Remove XMake files (if desired)

## Getting Help

### Resources
- [Meson Build Guide](MESON_BUILD.md) - Detailed Meson documentation
- [Meson Official Docs](https://mesonbuild.com/) - Upstream documentation
- [Conan Documentation](https://docs.conan.io/) - Package management

### Support Channels
- **Project Issues**: GitHub Issues for CppGraphIndex-specific problems
- **General Meson**: Meson community support
- **Migration Questions**: Tag issues with "migration" label

### Common Commands Reference Card

Print or bookmark this quick reference:

```bash
# QUICK REFERENCE CARD
# XMake → Meson Migration

# Windows Prerequisites
# NONE               → conanvcvars.bat (REQUIRED before any Meson commands)

# Build
xmake              → ninja -C builddir
xmake clean        → ninja -C builddir clean
xmake rebuild      → ninja -C builddir clean && ninja -C builddir

# Test
xmake test         → meson test -C builddir

# Tools  
xmake run format   → ninja -C builddir format
xmake run lint     → ninja -C builddir lint

# Setup
xmake f -m debug   → meson setup builddir --buildtype=debug
xmake f -m release → meson setup builddir_release --buildtype=release

# Run
xmake run MakeIndex → ./builddir/MakeIndex/makeindex_exe
```

Happy migrating! Both build systems provide excellent development experiences.