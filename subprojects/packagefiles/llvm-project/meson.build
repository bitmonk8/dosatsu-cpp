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