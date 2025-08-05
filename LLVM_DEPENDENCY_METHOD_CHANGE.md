
I'll guide you through setting up LLVM 19.1.17 as a git dependency in your Meson project.

## Setting up LLVM as a Git Dependency in Meson

### Step 1: Create the Wrap File

Create a directory called `subprojects` in your project root if it doesn't exist, then create a file named `llvm.wrap` inside it:

```bash
mkdir -p subprojects
```

Create `subprojects/llvm.wrap` with the following content:

```ini
[wrap-git]
url = https://github.com/llvm/llvm-project.git
revision = llvmorg-19.1.17
directory = llvm-project

[provide]
llvm = llvm_dep
```

### Step 2: Create a Meson Build File for LLVM

Since LLVM doesn't natively use Meson as its build system, you'll need to create a `meson.build` file in the LLVM subproject. Create `subprojects/packagefiles/llvm-project/meson.build`:

```bash
mkdir -p subprojects/packagefiles/llvm-project
```

Here's a basic `meson.build` file for LLVM:

```meson
project('llvm', 'cpp', 'c')

cmake = find_program('cmake')

# Configure LLVM with CMake
llvm_build = custom_target('llvm_configure',
  output: 'CMakeCache.txt',
  command: [cmake, '-S', meson.current_source_dir() / 'llvm', '-B', '@OUTDIR@',
            '-DCMAKE_BUILD_TYPE=Release',
            '-DLLVM_TARGETS_TO_BUILD=X86', # Adjust targets as needed
            '-DLLVM_ENABLE_PROJECTS=clang', # Add other projects if needed
            '-DCMAKE_INSTALL_PREFIX=@PREFIX@'],
  install: false
)

# Build LLVM
llvm_lib = custom_target('llvm_build',
  output: 'lib/libLLVM.a', # Adjust based on your needs
  depends: llvm_build,
  command: [cmake, '--build', '@BUILD_ROOT@'],
  install: false
)

# Create the dependency
llvm_dep = declare_dependency(
  dependencies: dependency('threads'),
  include_directories: include_directories('llvm/include'),
  link_with: llvm_lib
)
```


### Step 3: Alternative Approach - External Project

A more robust approach might be to use Meson's external project module[^1_3]. Create `subprojects/llvm.wrap`:

```ini
[wrap-git]
url = https://github.com/llvm/llvm-project.git
revision = llvmorg-19.1.17
directory = llvm-project
```

Then in your main `meson.build`:

```meson
project('your_project', 'cpp')

# Try to find system LLVM first, fallback to subproject
llvm_dep = dependency('llvm', version: '>=19.0', required: false)

if not llvm_dep.found()
  llvm_proj = subproject('llvm')
  llvm_dep = llvm_proj.get_variable('llvm_dep')
endif

your_exe = executable('your_program',
  'main.cpp',
  dependencies: [llvm_dep]
)
```


### Step 4: Verify the Git Tag

Before proceeding, verify that the LLVM version tag exists:

```bash
git ls-remote --tags https://github.com/llvm/llvm-project.git | grep 19
```

### Step 5: Build Your Project

Once everything is set up, build your project:

```bash
meson setup builddir
meson compile -C builddir
```


## Important Considerations

1. **LLVM Size**: LLVM is a very large project. Building it from source will take significant time and disk space.
2. **System Dependencies**: LLVM has its own dependencies (CMake, Python, etc.) that must be available.
3. **CMake Integration**: Since LLVM uses CMake, Meson needs to interface with it, which can be complex[^1_4].
4. **Alternative Approach**: Consider using a system-installed LLVM or pre-built binaries if available, as they're much faster to work with.

The wrap dependency system[^1_5] in Meson is designed exactly for this use case - automatically downloading and building dependencies from git repositories. The key is ensuring that either the dependency has native Meson support or you provide the necessary build files to make it work with Meson.
