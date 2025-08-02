project('CppGraphIndex', 'cpp',
  version : '1.0.0',
  default_options : [
    'cpp_std=c++20',
    'warning_level=3',
    'werror=true',
    'buildtype=debug'
  ],
  meson_version : '>=0.56.0'
)

# Compiler setup
cpp = meson.get_compiler('cpp')

# Windows-specific warning suppressions (matching xmake /wd4146)
if target_machine.system() == 'windows'
  if cpp.get_id() == 'msvc'
    add_project_arguments('/wd4146', language : 'cpp')
  elif cpp.get_id() == 'gcc'
    # GCC equivalents for LLVM compatibility
    add_project_arguments('-Wno-unused-parameter', language : 'cpp')
    add_project_arguments('-Wno-unused-variable', language : 'cpp')
    add_project_arguments('-Wno-deprecated-enum-enum-conversion', language : 'cpp')
    add_project_arguments('-Wno-deprecated', language : 'cpp')
  endif
endif

# Build configuration - match xmake runtime settings
# Debug: MDd runtime (dynamic CRT, debug)
# Release: MT runtime (static CRT, release)

# Get the build type
build_type = get_option('buildtype')

# Configure debug/release specific settings
if build_type == 'debug' or build_type == 'debugoptimized'
  # Debug build configuration
  if target_machine.system() == 'windows' and cpp.get_id() == 'msvc'
    add_project_arguments('/MDd', language : 'cpp')
    if get_option('debug_symbols')
      add_project_arguments('/Zi', language : 'cpp')  # Generate debug info
    endif
  elif cpp.get_id() == 'gcc' or cpp.get_id() == 'clang'
    if get_option('debug_symbols')
      add_project_arguments('-g', language : 'cpp')  # Generate debug info
    endif
  endif
  
elif build_type == 'release' or build_type == 'minsize'
  # Release build configuration
  if target_machine.system() == 'windows' and cpp.get_id() == 'msvc'
    add_project_arguments('/MT', language : 'cpp')
    # Set optimization level
    opt_level = get_option('optimization_level')
    if opt_level == '0'
      add_project_arguments('/Od', language : 'cpp')
    elif opt_level == '1'
      add_project_arguments('/O1', language : 'cpp')
    elif opt_level == '2'
      add_project_arguments('/O2', language : 'cpp')
    elif opt_level == '3' or opt_level == 's'
      add_project_arguments('/Ox', language : 'cpp')  # Maximum optimization
    endif
  elif cpp.get_id() == 'gcc' or cpp.get_id() == 'clang'
    # GCC/Clang optimization levels
    opt_level = get_option('optimization_level')
    if opt_level == '0'
      add_project_arguments('-O0', language : 'cpp')
    elif opt_level == '1'
      add_project_arguments('-O1', language : 'cpp')
    elif opt_level == '2'
      add_project_arguments('-O2', language : 'cpp')
    elif opt_level == '3'
      add_project_arguments('-O3', language : 'cpp')
    elif opt_level == 's'
      add_project_arguments('-Os', language : 'cpp')  # Optimize for size
    endif
  endif
  
endif

# Include directories
inc_dir = include_directories('3rdParty/include')

# Dependencies from Conan
# TODO: Add proper dependency resolution in Step 7 refinement
# For now, use find_library to locate the libraries manually

# Get LLVM paths from xmake package (working configuration)
llvm_inc_dir = 'C:/Users/thomasa/AppData/Local/.xmake/packages/l/libllvm/19.1.7/1846a5ba8f6f4ddfae98aa55f7f30064/include'
llvm_lib_dir = 'C:/Users/thomasa/AppData/Local/.xmake/packages/l/libllvm/19.1.7/1846a5ba8f6f4ddfae98aa55f7f30064/lib'

# Core LLVM libraries that MakeIndex needs (matching xmake output)
llvm_libs = [
  # Core LLVM support
  'LLVMCore', 'LLVMSupport', 'LLVMDemangle',
  # Clang AST libraries
  'clangAST', 'clangBasic', 'clangLex', 'clangDriver', 'clangSema',
  'clangAnalysis', 'clangEdit', 'clangFrontend', 'clangParse',
  # Additional LLVM libraries used by the code
  'LLVMBitReader', 'LLVMBitstreamReader', 'LLVMBinaryFormat'
]

# Find all required libraries
llvm_deps = []
foreach lib : llvm_libs
  dep = cpp.find_library(lib, dirs : llvm_lib_dir, required : false)
  if dep.found()
    llvm_deps += dep
  else
    warning('Library @0@ not found in @1@'.format(lib, llvm_lib_dir))
  endif
endforeach

# Create the LLVM dependency
if llvm_deps.length() > 0
  llvm_dep = declare_dependency(
    dependencies : llvm_deps,
    include_directories : include_directories(llvm_inc_dir)
  )
else
  # Fallback for when libraries aren't found
  warning('No LLVM libraries found, creating dummy dependency')
  llvm_dep = declare_dependency()
endif

# DocTest dependency (header-only, just need include path)
doctest_dep = declare_dependency()

# Add subdirectories
subdir('MakeIndex')