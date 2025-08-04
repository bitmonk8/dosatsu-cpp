# MakeIndex subdirectory build configuration

# Source files for the MakeIndex executable
makeindex_sources = [
  'MakeIndex.cpp',
  'KuzuDump.cpp'
]

# Create the MakeIndex executable
makeindex_exe = executable('MakeIndex',
  makeindex_sources,
  include_directories : inc_dir,
  dependencies : all_deps,
  install : false
)

# Add selftest as a test target
test('selftest', makeindex_exe,
  args : ['--selftest'],
  timeout : 10,
  suite : 'unit'
)