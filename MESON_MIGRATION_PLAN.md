# CppGraphIndex: Meson, Ninja & Conan Integration Plan

## Progress Tracking

### Overall Progress
- **Phase 1 (Foundation Setup)**: 2/4 steps complete
- **Phase 2 (Core Build System)**: 0/4 steps complete  
- **Phase 3 (Development Tools)**: 0/4 steps complete
- **Phase 4 (Advanced Features)**: 0/4 steps complete
- **Phase 5 (Documentation & CI)**: 0/2 steps complete
- **Total Progress**: 2/18 steps complete (11%)

### Phase Progress Summary
| Phase | Steps | Completed | Status |
|-------|-------|-----------|--------|
| Phase 1: Foundation Setup | 1-4 | 2/4 | 🔄 In Progress |
| Phase 2: Core Build System | 5-8 | 0/4 | ❌ Not Started |
| Phase 3: Development Tools | 9-12 | 0/4 | ❌ Not Started |
| Phase 4: Advanced Features | 13-16 | 0/4 | ❌ Not Started |
| Phase 5: Documentation & CI | 17-18 | 0/2 | ❌ Not Started |

### Legend
- ❌ Not Started
- 🔄 In Progress  
- ✅ Complete
- ⚠️ Blocked/Issues

---

## Overview

This document outlines an incremental plan to add Meson, Ninja, and Conan support to the CppGraphIndex project while maintaining existing xmake functionality. The goal is to support both build systems during a transition period.

## Current State Analysis

### Existing xmake Configuration
- **Language**: C++20
- **Primary Target**: MakeIndex binary
- **Key Dependencies**: 
  - libllvm 19.1.7 (built from source)
  - doctest (testing framework)
  - Clang (for AST parsing)
  - Kuzu (graph database)
- **Build Modes**: Debug (default) and Release
- **Additional Targets**: format (clang-format), lint (clang-tidy), test
- **Platform Support**: Windows, macOS, Ubuntu

### Source Structure
```
MakeIndex/
├── MakeIndex.cpp     # Main entry point with test support
├── KuzuDump.cpp      # Core functionality
├── KuzuDump.h        # Interface definitions
├── NoWarningScope_Enter.h
└── NoWarningScope_Leave.h
```

## Migration Strategy

### Phase 1: Foundation Setup (Steps 1-4)
Basic Meson and Conan infrastructure without breaking existing workflow.

### Phase 2: Core Build System (Steps 5-8)
Implement main target compilation with dependency management.

### Phase 3: Development Tools (Steps 9-12)
Add formatting, linting, and testing capabilities.

### Phase 4: Advanced Features (Steps 13-16)
Complete feature parity and optimization.

### Phase 5: Documentation & CI (Steps 17-18)
Update documentation and continuous integration.

---

## Incremental Implementation Steps

### Phase 1: Foundation Setup

#### Step 1: Install and Verify Conan
**Progress**: ✅ Complete  
**Goal**: Set up Conan package manager for dependency management.

**Actions**:
- Install Conan 2.x if not present
- Create `conanfile.py` with basic configuration
- Add `.conan/` to `.gitignore`

**Files to Create**:
- `conanfile.py` (basic structure)
- `conan/` directory for profiles

**Verification**:
- Run `conan --version`
- Verify Conan can detect system settings

**Rollback**: Remove conanfile.py if issues arise

---

#### Step 2: Create Basic Conan Profile
**Progress**: ✅ Complete  
**Goal**: Configure Conan for the target development environment.

**Actions**:
- Create default Conan profile matching current compiler settings
- Configure C++20 standard
- Set up Windows-specific settings (matching xmake config)

**Files to Create**:
- `conan/profiles/default` (compiler, runtime settings)

**Verification**:
- Run `conan profile detect --force`
- Verify profile matches current development environment

**Dependencies**: Step 1

---

#### Step 3: Install Meson and Ninja
**Progress**: ❌ Not Started  
**Goal**: Set up the new build system tools.

**Actions**:
- Install Meson build system
- Install Ninja build backend
- Verify compatibility with existing toolchain

**Verification**:
- Run `meson --version`
- Run `ninja --version`
- Test basic Meson functionality with empty project

**Rollback**: Tools are standalone, no rollback needed

---

#### Step 4: Create Basic Meson Structure
**Progress**: ❌ Not Started  
**Goal**: Create minimal meson.build files without functionality.

**Actions**:
- Create root `meson.build` with project declaration
- Set C++20 standard
- Add basic compiler configuration

**Files to Create**:
- `meson.build` (root level)
- `meson.options` (build options)

**Verification**:
- Run `meson setup builddir --dry-run`
- Verify no syntax errors in Meson configuration

**Dependencies**: Step 3

---

### Phase 2: Core Build System

#### Step 5: Configure LibLLVM in Conan
**Progress**: ❌ Not Started  
**Goal**: Add libllvm as a Conan dependency.

**Actions**:
- Research libllvm availability in ConanCenter
- Add libllvm/19.1.7 to conanfile.py requirements
- Configure version constraints and options

**Files to Modify**:
- `conanfile.py` (add libllvm requirement)

**Verification**:
- Run `conan search libllvm`
- Test dependency resolution: `conan install . --build=missing`

**Dependencies**: Step 2

**Fallback Plan**: If libllvm not available in ConanCenter, create custom recipe or use system installation

---

#### Step 6: Add DocTest Dependency
**Progress**: ❌ Not Started  
**Goal**: Configure testing framework through Conan.

**Actions**:
- Add doctest to conanfile.py requirements
- Configure as test-only dependency

**Files to Modify**:
- `conanfile.py` (add doctest requirement)

**Verification**:
- Test dependency resolution includes doctest
- Verify doctest headers are accessible

**Dependencies**: Step 5

---

#### Step 7: Implement MakeIndex Target
**Progress**: ❌ Not Started  
**Goal**: Build the main executable using Meson.

**Actions**:
- Add MakeIndex source files to meson.build
- Configure include directories
- Link libllvm dependency
- Set up C++20 compilation flags

**Files to Modify**:
- `meson.build` (add executable target)

**Files to Create**:
- `MakeIndex/meson.build` (subdirectory configuration)

**Verification**:
- Run `meson setup builddir`
- Run `ninja -C builddir`
- Verify MakeIndex executable is created and runs

**Dependencies**: Step 6

---

#### Step 8: Configure Build Modes
**Progress**: ❌ Not Started  
**Goal**: Implement debug and release build configurations.

**Actions**:
- Add build type options to meson.options
- Configure debug symbols and optimization levels
- Match xmake runtime settings (MDd for debug, MT for release)

**Files to Modify**:
- `meson.build` (build type configuration)
- `meson.options` (add build mode options)

**Verification**:
- Test debug build: `meson setup builddir --buildtype=debug`
- Test release build: `meson setup builddir_release --buildtype=release`
- Verify optimization flags are applied correctly

**Dependencies**: Step 7

---

### Phase 3: Development Tools

#### Step 9: Implement Format Target
**Progress**: ❌ Not Started  
**Goal**: Add code formatting capability using clang-format.

**Actions**:
- Create custom Meson target for formatting
- Use existing .clang-format configuration
- Match current xmake format behavior (scan MakeIndex/ directory)

**Files to Modify**:
- `meson.build` (add format target)

**Files to Create**:
- `tools/format.py` (Python script for formatting)

**Verification**:
- Run `ninja -C builddir format`
- Verify same files are formatted as with xmake
- Compare output with `xmake run format`

**Dependencies**: Step 8

---

#### Step 10: Implement Lint Target  
**Progress**: ❌ Not Started  
**Goal**: Add code linting capability using clang-tidy.

**Actions**:
- Create custom Meson target for linting
- Generate compile_commands.json
- Use existing .clang-tidy configuration
- Support both full project and single file linting

**Files to Modify**:
- `meson.build` (add lint target)

**Files to Create**:
- `tools/lint.py` (Python script for linting)

**Verification**:
- Run `ninja -C builddir lint`
- Verify compile_commands.json is generated
- Compare linting output with `xmake run lint`

**Dependencies**: Step 9

---

#### Step 11: Implement Test Target
**Progress**: ❌ Not Started  
**Goal**: Add testing capability with doctest integration.

**Actions**:
- Configure Meson test framework
- Add selftest functionality (--selftest flag)
- Integrate with doctest framework
- Match xmake test timeout and behavior

**Files to Modify**:
- `meson.build` (add test configuration)
- `MakeIndex/meson.build` (test integration)

**Verification**:
- Run `meson test -C builddir`
- Run `ninja -C builddir && ./builddir/MakeIndex --selftest`
- Compare test output with `xmake test`

**Dependencies**: Step 10

---

#### Step 12: Cross-Platform Configuration
**Progress**: ❌ Not Started  
**Goal**: Ensure builds work on Windows, macOS, and Linux.

**Actions**:
- Add platform-specific compiler flags
- Configure Windows-specific warnings (match /wd4146)
- Test runtime library configuration
- Add platform detection logic

**Files to Modify**:
- `meson.build` (platform-specific configuration)
- `conanfile.py` (platform-specific requirements)

**Verification**:
- Test build on available platforms
- Verify compiler flags match xmake configuration
- Check runtime library linkage

**Dependencies**: Step 11

---

### Phase 4: Advanced Features

#### Step 13: Conan Integration Script
**Progress**: ❌ Not Started  
**Goal**: Automate Conan dependency management.

**Actions**:
- Create wrapper script for dependency installation
- Integrate Conan with Meson setup
- Add dependency caching configuration

**Files to Create**:
- `scripts/setup-deps.py` (dependency management)
- `scripts/build.py` (integrated build script)

**Verification**:
- Run `python scripts/setup-deps.py`
- Verify dependencies are correctly installed and cached
- Test clean build from scratch

**Dependencies**: Step 12

---

#### Step 14: Development Environment Scripts
**Progress**: ❌ Not Started  
**Goal**: Create convenience scripts for common development tasks.

**Actions**:
- Create build wrapper scripts
- Add clean/rebuild functionality  
- Create development setup script
- Add IDE integration helpers

**Files to Create**:
- `scripts/dev-build.py` (development build)
- `scripts/clean.py` (clean builds)
- `scripts/setup-dev.py` (development environment)

**Verification**:
- Test all scripts work correctly
- Verify they provide same functionality as xmake commands
- Check script error handling

**Dependencies**: Step 13

---

#### Step 15: Performance Optimization
**Progress**: ❌ Not Started  
**Goal**: Optimize build performance and dependency management.

**Actions**:
- Configure Ninja parallel builds
- Optimize Conan cache settings
- Add incremental build support
- Configure ccache if available

**Files to Modify**:
- `meson.build` (optimization settings)
- `conanfile.py` (cache configuration)

**Files to Create**:
- `.ninja_deps` exclusion in .gitignore
- `conan.conf` (performance settings)

**Verification**:
- Measure build times vs xmake
- Test incremental build correctness
- Verify parallel builds work correctly

**Dependencies**: Step 14

---

#### Step 16: Feature Parity Validation
**Progress**: ❌ Not Started  
**Goal**: Ensure complete feature parity with xmake.

**Actions**:
- Create validation checklist
- Test all build modes and targets
- Verify output consistency
- Document any differences

**Files to Create**:
- `FEATURE_PARITY.md` (parity checklist)
- `scripts/validate-parity.py` (automated validation)

**Verification**:
- All xmake targets have Meson equivalents
- Build outputs are equivalent
- Development workflow is preserved

**Dependencies**: Step 15

---

### Phase 5: Documentation & CI

#### Step 17: Update Documentation
**Progress**: ❌ Not Started  
**Goal**: Document the new build system options.

**Actions**:
- Update README.md with Meson instructions
- Add build system comparison section
- Create migration guide for developers
- Document troubleshooting steps

**Files to Modify**:
- `README.md` (add Meson sections)

**Files to Create**:
- `docs/MESON_BUILD.md` (detailed Meson guide)
- `docs/MIGRATION_GUIDE.md` (xmake to Meson migration)

**Verification**:
- Documentation builds successfully
- Instructions are clear and accurate
- Examples work correctly

**Dependencies**: Step 16

---

#### Step 18: CI/CD Integration
**Progress**: ❌ Not Started  
**Goal**: Add Meson builds to continuous integration.

**Actions**:
- Add Meson build jobs to existing CI
- Test both build systems in parallel
- Configure artifact generation
- Add performance comparisons

**Files to Create/Modify**:
- `.github/workflows/meson-ci.yml` (if using GitHub Actions)
- Update existing CI files to include Meson tests

**Verification**:
- CI builds pass for both systems
- Artifacts are generated correctly
- Build times are acceptable

**Dependencies**: Step 17

---

## Risk Management

### Low Risk Steps (1-4, 9-10, 17-18)
- Can be implemented without affecting existing workflow
- Easy to rollback if needed
- Minimal integration complexity

### Medium Risk Steps (5-8, 11-12)
- Core functionality changes
- Dependency on external packages
- May require troubleshooting platform differences

### High Risk Steps (13-16)
- Integration and optimization
- Performance implications
- May affect development workflow

## Rollback Strategy

1. **Individual Step Rollback**: Each step can be individually reverted by removing created files
2. **Git Branching**: Use feature branches for each phase
3. **Parallel Systems**: Keep xmake functional throughout migration
4. **Validation Points**: Test xmake still works after each step

## Success Metrics

- [ ] All xmake functionality replicated in Meson
- [ ] Build times comparable or better
- [ ] Developer workflow preserved
- [ ] Cross-platform compatibility maintained
- [ ] Dependencies managed through Conan
- [ ] CI/CD pipeline functional
- [ ] Documentation complete and accurate

## Next Steps

1. **Phase 1**: Start with foundation setup (Steps 1-4)
2. **Verification**: Ensure xmake still works after each step
3. **Testing**: Validate each step before proceeding
4. **Documentation**: Update docs as you progress
5. **Team Communication**: Keep team informed of progress

## Notes

- This plan assumes libllvm is available through ConanCenter or can be built from source
- Platform-specific adjustments may be needed during implementation
- Consider creating a dedicated branch for this migration
- Regular testing with the existing xmake system ensures parallel functionality
- Some steps may be parallelizable depending on team size and expertise