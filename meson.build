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
  add_project_arguments('/wd4146', language : 'cpp')
endif

# Build configuration - match xmake runtime settings
# Debug: MDd runtime (dynamic CRT, debug)
# Release: MT runtime (static CRT, release)
if get_option('buildtype') == 'debug'
  if target_machine.system() == 'windows'
    add_project_arguments('/MDd', language : 'cpp')
  endif
elif get_option('buildtype') == 'release'
  if target_machine.system() == 'windows'
    add_project_arguments('/MT', language : 'cpp')
  endif
endif

# Include directories
inc_dir = include_directories('3rdParty/include')

# TODO: Add dependencies (libllvm, etc.) in later steps
# TODO: Add subdirectories and targets in later steps

# Placeholder message
message('Basic Meson structure created - Foundation step 4 complete')