project('CppGraphIndex', 'cpp',
  version : '1.0.0',
  default_options : [
    'cpp_std=c++20',
    'warning_level=3',
    'werror=true',
    'buildtype=debug'
  ],
  meson_version : '>=1.0.0'
)

# Compiler setup
cpp = meson.get_compiler('cpp')

# Performance optimization settings
# Configure Ninja parallel builds based on CPU count
if meson.backend() == 'ninja'
  # Enable parallel compilation - Ninja will auto-detect CPU count
  # Add compiler cache if available (but avoid on Windows for MSVC compatibility)
  if target_machine.system() != 'windows'
    ccache = find_program('ccache', required : false)
    if ccache.found()
      message('Found ccache, enabling compilation caching')
      # Note: ccache configuration happens through environment or config files
    endif
  endif
  
  # Enable incremental build support
  # Generate compile_commands.json for better dependency tracking
  if get_option('compile_commands_json')
    message('Generating compile_commands.json for improved dependency tracking')
  endif
endif

# Additional performance optimizations
# Enable LTO (Link Time Optimization) for release builds if supported
if get_option('buildtype') == 'release' and get_option('enable_lto')
  if cpp.has_link_argument('-flto') and cpp.get_id() != 'msvc'
    add_project_link_arguments('-flto', language : 'cpp')
    add_project_arguments('-flto', language : 'cpp')
    message('Link Time Optimization (LTO) enabled for release build')
  elif cpp.get_id() == 'msvc' and cpp.has_link_argument('/LTCG')
    add_project_link_arguments('/LTCG', language : 'cpp')
    add_project_arguments('/GL', language : 'cpp')
    message('Link Time Code Generation (LTCG) enabled for release build')
  endif
endif

# Platform and compiler-specific configurations
if target_machine.system() == 'windows'
  # On Windows, prefer MSVC for LLVM compatibility
  # Handle ccache-wrapped compilers by checking the underlying compiler
  compiler_cmd = cpp.cmd_array()
  actual_compiler_id = cpp.get_id()
  
  # If using ccache, check the actual underlying compiler
  if compiler_cmd.length() > 1 and 'ccache' in compiler_cmd[0]
    # For ccache-wrapped compilers, check the actual backend compiler
    message('Detected ccache-wrapped compiler, checking underlying compiler...')
    # Note: Meson should still report the correct compiler ID for the underlying compiler
  endif
  
  if actual_compiler_id == 'msvc'
    # Suppress common LLVM/Clang header warnings with MSVC
    add_project_arguments('/wd4146', language : 'cpp')  # unary minus operator applied to unsigned type
    add_project_arguments('/wd4267', language : 'cpp')  # conversion from size_t to smaller type
    add_project_arguments('/wd4324', language : 'cpp')  # structure was padded due to alignment specifier  
    add_project_arguments('/wd4100', language : 'cpp')  # unreferenced formal parameter
    add_project_arguments('/wd4189', language : 'cpp')  # local variable is initialized but not referenced
    add_project_arguments('/wd4245', language : 'cpp')  # signed/unsigned mismatch in conversion
    add_project_arguments('/wd4389', language : 'cpp')  # signed/unsigned mismatch in comparison
    add_project_arguments('/wd5054', language : 'cpp')  # deprecated operator between different enum types
  else
    # Provide more helpful error message for ccache scenarios
    if compiler_cmd.length() > 1 and 'ccache' in compiler_cmd[0]
      error('On Windows, MSVC compiler is required for LLVM compatibility. Detected ccache with @0@ compiler. Please configure ccache to use MSVC or set up Meson to use MSVC directly.'.format(actual_compiler_id))
    else
      error('On Windows, MSVC compiler is required for LLVM compatibility. Detected @0@ compiler. Use: meson setup builddir --native-file native-msvc.txt'.format(actual_compiler_id))
    endif
  endif
elif target_machine.system() == 'linux'
  # On Linux, use GCC
  if cpp.get_id() != 'gcc'
    warning('On Linux, GCC is recommended')
  endif
elif target_machine.system() == 'darwin'
  # On macOS, use Clang
  if cpp.get_id() != 'clang'
    warning('On macOS, Clang is recommended')
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

# Complete LLVM and Clang libraries (matching xmake linking output)
clang_libs = [
  'clangInterpreter', 'clangFrontendTool', 'clangStaticAnalyzerFrontend', 'clangStaticAnalyzerCheckers', 
  'clangTransformer', 'clangStaticAnalyzerCore', 'clangToolingRefactoring', 'clangExtractAPI', 
  'clangCrossTU', 'clangDependencyScanning', 'clangIndex', 'clangTooling', 'clangToolingSyntax', 
  'clangRewriteFrontend', 'clangARCMigrate', 'clangCodeGen', 'clangFrontend', 'clangAnalysisFlowSensitiveModels', 
  'clangSerialization', 'clangParse', 'clangFormat', 'clangAnalysisFlowSensitive', 'clangSema', 
  'clangToolingInclusions', 'clangAnalysis', 'clangDynamicASTMatchers', 'clangToolingCore', 'clangInstallAPI', 
  'clangToolingASTDiff', 'clangToolingInclusionsStdlib', 'clangEdit', 'clangASTMatchers', 'clangRewrite', 
  'clangAST', 'clangIndexSerialization', 'clangDriver', 'clangLex', 'clangAPINotes', 'clangSupport', 
  'clangDirectoryWatcher', 'clangBasic'
]

llvm_libs = [
  'LLVMExegesisMips', 'LLVMExegesisPowerPC', 'LLVMExegesisAArch64', 'LLVMExegesisX86', 'LLVMOptDriver', 
  'LLVMExegesis', 'LLVMOrcDebugging', 'LLVMBPFCodeGen', 'LLVMAMDGPUCodeGen', 'LLVMOrcJIT', 'LLVMLTO', 
  'LLVMPasses', 'LLVMX86CodeGen', 'LLVMRISCVCodeGen', 'LLVMPowerPCCodeGen', 'LLVMMipsCodeGen', 'LLVMARMCodeGen', 
  'LLVMAArch64CodeGen', 'LLVMNVPTXCodeGen', 'LLVMHexagonCodeGen', 'LLVMCoroutines', 'LLVMWebAssemblyCodeGen', 
  'LLVMDWARFLinkerParallel', 'LLVMDWARFLinkerClassic', 'LLVMXCoreCodeGen', 'LLVMVECodeGen', 'LLVMSystemZCodeGen', 
  'LLVMSparcCodeGen', 'LLVMMSP430CodeGen', 'LLVMLoongArchCodeGen', 'LLVMLanaiCodeGen', 'LLVMAVRCodeGen', 
  'LLVMGlobalISel', 'LLVMipo', 'LLVMWebAssemblyUtils', 'LLVMInterpreter', 'LLVMDWARFLinker', 'LLVMMIRParser', 
  'LLVMAsmPrinter', 'LLVMSelectionDAG', 'LLVMFrontendOpenMP', 'LLVMCodeGen', 'LLVMFuzzMutate', 'LLVMAMDGPUTargetMCA', 
  'LLVMAMDGPUDisassembler', 'LLVMAMDGPUAsmParser', 'LLVMMCJIT', 'LLVMScalarOpts', 'LLVMAMDGPUDesc', 'LLVMExecutionEngine', 
  'LLVMLinker', 'LLVMHipStdPar', 'LLVMObjCARCOpts', 'LLVMVectorize', 'LLVMInstCombine', 'LLVMAggressiveInstCombine', 
  'LLVMInstrumentation', 'LLVMFrontendOffloading', 'LLVMAMDGPUUtils', 'LLVMTarget', 'LLVMTransformUtils', 
  'LLVMFrontendDriver', 'LLVMBitWriter', 'LLVMIRPrinter', 'LLVMCoverage', 'LLVMAnalysis', 'LLVMCFIVerify', 
  'LLVMProfileData', 'LLVMDebuginfod', 'LLVMARMDisassembler', 'LLVMARMAsmParser', 'LLVMSymbolize', 
  'LLVMDebugInfoLogicalView', 'LLVMTextAPIBinaryReader', 'LLVMDWP', 'LLVMDebugInfoGSYM', 'LLVMXRay', 'LLVMLibDriver', 
  'LLVMDlltoolDriver', 'LLVMARMDesc', 'LLVMRuntimeDyld', 'LLVMJITLink', 'LLVMDebugInfoPDB', 'LLVMDebugInfoDWARF', 
  'LLVMObjectYAML', 'LLVMObjCopy', 'LLVMCodeGenData', 'LLVMInterfaceStub', 'LLVMX86TargetMCA', 'LLVMX86AsmParser', 
  'LLVMWebAssemblyDisassembler', 'LLVMWebAssemblyAsmParser', 'LLVMVEAsmParser', 'LLVMSystemZDisassembler', 
  'LLVMSystemZAsmParser', 'LLVMSparcAsmParser', 'LLVMRISCVTargetMCA', 'LLVMRISCVDisassembler', 'LLVMRISCVAsmParser', 
  'LLVMPowerPCAsmParser', 'LLVMMSP430AsmParser', 'LLVMMipsAsmParser', 'LLVMLoongArchDisassembler', 
  'LLVMLoongArchAsmParser', 'LLVMLanaiDisassembler', 'LLVMLanaiAsmParser', 'LLVMHexagonDisassembler', 
  'LLVMHexagonAsmParser', 'LLVMBPFAsmParser', 'LLVMAVRAsmParser', 'LLVMAArch64Disassembler', 'LLVMAArch64AsmParser', 
  'LLVMObject', 'LLVMXCoreDesc', 'LLVMXCoreDisassembler', 'LLVMX86Desc', 'LLVMX86Disassembler', 'LLVMWebAssemblyDesc', 
  'LLVMVEDesc', 'LLVMVEDisassembler', 'LLVMSystemZDesc', 'LLVMSparcDesc', 'LLVMSparcDisassembler', 'LLVMRISCVDesc', 
  'LLVMPowerPCDesc', 'LLVMPowerPCDisassembler', 'LLVMNVPTXDesc', 'LLVMMSP430Disassembler', 'LLVMMSP430Desc', 
  'LLVMMipsDesc', 'LLVMMipsDisassembler', 'LLVMLoongArchDesc', 'LLVMLanaiDesc', 'LLVMHexagonDesc', 'LLVMBPFDesc', 
  'LLVMBPFDisassembler', 'LLVMAVRDesc', 'LLVMAVRDisassembler', 'LLVMAArch64Desc', 'LLVMIRReader', 'LLVMXCoreInfo', 
  'LLVMX86Info', 'LLVMWebAssemblyInfo', 'LLVMVEInfo', 'LLVMSystemZInfo', 'LLVMSparcInfo', 'LLVMRISCVInfo', 
  'LLVMPowerPCInfo', 'LLVMNVPTXInfo', 'LLVMMSP430Info', 'LLVMMipsInfo', 'LLVMLoongArchInfo', 'LLVMLanaiInfo', 
  'LLVMHexagonInfo', 'LLVMBPFInfo', 'LLVMAVRInfo', 'LLVMARMInfo', 'LLVMAMDGPUInfo', 'LLVMAArch64Info', 'LLVMMCA', 
  'LLVMMCDisassembler', 'LLVMMCParser', 'LLVMDiff', 'LLVMAsmParser', 'LLVMSandboxIR', 'LLVMAArch64Utils', 
  'LLVMCFGuard', 'LLVMFrontendHLSL', 'LLVMBitReader', 'LLVMTextAPI', 'LLVMMC', 'LLVMCore', 'LLVMTableGenCommon', 
  'LLVMWindowsDriver', 'LLVMOrcTargetProcess', 'LLVMBinaryFormat', 'LLVMFuzzerCLI', 'LLVMRemarks', 'LLVMTableGenBasic', 
  'LLVMWindowsManifest', 'LLVMTargetParser', 'LLVMLineEditor', 'LLVMARMUtils', 'LLVMOrcShared', 'LLVMDebugInfoBTF', 
  'LLVMDebugInfoCodeView', 'LLVMDebugInfoMSF', 'LLVMOption', 'LLVMFrontendOpenACC', 'LLVMExtensions', 
  'LLVMBitstreamReader', 'LLVMCodeGenTypes', 'LLVMFileCheck', 'LLVMTableGen', 'LLVMSupport', 'LLVMDemangle'
]

# Additional LLVM libraries
additional_llvm_libs = ['Remarks', 'LTO', 'LLVM-C']

# Combine all libraries
all_libs = clang_libs + llvm_libs + additional_llvm_libs

# Create LLVM dependency using library directories (like xmake)
# This avoids individual library resolution and lets the linker handle it
system_libs = []
if target_machine.system() == 'windows'
  system_deps = [
    'psapi', 'shell32', 'ole32', 'uuid', 'advapi32', 
    'ws2_32', 'ntdll', 'version'
  ]
  
  foreach lib : system_deps
    dep = cpp.find_library(lib, required : false)
    if dep.found()
      system_libs += dep
    endif
  endforeach
endif

# Create LLVM dependency using MSVC-compatible approach
# Find individual libraries needed by MakeIndex
required_llvm_libs = [
  'clangInterpreter', 'clangFrontendTool', 'clangStaticAnalyzerFrontend', 'clangStaticAnalyzerCheckers',
  'clangTransformer', 'clangStaticAnalyzerCore', 'clangToolingRefactoring', 'clangExtractAPI',
  'clangCrossTU', 'clangDependencyScanning', 'clangIndex', 'clangTooling', 'clangToolingSyntax',
  'clangRewriteFrontend', 'clangARCMigrate', 'clangCodeGen', 'clangFrontend', 'clangAnalysisFlowSensitiveModels',
  'clangSerialization', 'clangParse', 'clangFormat', 'clangAnalysisFlowSensitive', 'clangSema',
  'clangToolingInclusions', 'clangAnalysis', 'clangDynamicASTMatchers', 'clangToolingCore', 'clangInstallAPI',
  'clangToolingASTDiff', 'clangToolingInclusionsStdlib', 'clangEdit', 'clangASTMatchers', 'clangRewrite',
  'clangAST', 'clangIndexSerialization', 'clangDriver', 'clangLex', 'clangAPINotes', 'clangSupport',
  'clangDirectoryWatcher', 'clangBasic',
  'LLVMExegesisMips', 'LLVMExegesisPowerPC', 'LLVMExegesisAArch64', 'LLVMExegesisX86', 'LLVMOptDriver',
  'LLVMExegesis', 'LLVMOrcDebugging', 'LLVMBPFCodeGen', 'LLVMAMDGPUCodeGen', 'LLVMOrcJIT', 'LLVMLTO',
  'LLVMPasses', 'LLVMX86CodeGen', 'LLVMRISCVCodeGen', 'LLVMPowerPCCodeGen', 'LLVMMipsCodeGen', 'LLVMARMCodeGen',
  'LLVMAArch64CodeGen', 'LLVMNVPTXCodeGen', 'LLVMHexagonCodeGen', 'LLVMCoroutines', 'LLVMWebAssemblyCodeGen',
  'LLVMDWARFLinkerParallel', 'LLVMDWARFLinkerClassic', 'LLVMXCoreCodeGen', 'LLVMVECodeGen', 'LLVMSystemZCodeGen',
  'LLVMSparcCodeGen', 'LLVMMSP430CodeGen', 'LLVMLoongArchCodeGen', 'LLVMLanaiCodeGen', 'LLVMAVRCodeGen',
  'LLVMGlobalISel', 'LLVMipo', 'LLVMWebAssemblyUtils', 'LLVMInterpreter', 'LLVMDWARFLinker', 'LLVMMIRParser',
  'LLVMAsmPrinter', 'LLVMSelectionDAG', 'LLVMFrontendOpenMP', 'LLVMCodeGen', 'LLVMFuzzMutate', 'LLVMAMDGPUTargetMCA',
  'LLVMAMDGPUDisassembler', 'LLVMAMDGPUAsmParser', 'LLVMMCJIT', 'LLVMScalarOpts', 'LLVMAMDGPUDesc',
  'LLVMExecutionEngine', 'LLVMLinker', 'LLVMHipStdPar', 'LLVMObjCARCOpts', 'LLVMVectorize', 'LLVMInstCombine',
  'LLVMAggressiveInstCombine', 'LLVMInstrumentation', 'LLVMFrontendOffloading', 'LLVMAMDGPUUtils', 'LLVMTarget',
  'LLVMTransformUtils', 'LLVMFrontendDriver', 'LLVMBitWriter', 'LLVMIRPrinter', 'LLVMCoverage', 'LLVMAnalysis',
  'LLVMCFIVerify', 'LLVMProfileData', 'LLVMDebuginfod', 'LLVMARMDisassembler', 'LLVMARMAsmParser', 'LLVMSymbolize',
  'LLVMDebugInfoLogicalView', 'LLVMTextAPIBinaryReader', 'LLVMDWP', 'LLVMDebugInfoGSYM', 'LLVMXRay',
  'LLVMLibDriver', 'LLVMDlltoolDriver', 'LLVMARMDesc', 'LLVMRuntimeDyld', 'LLVMJITLink', 'LLVMDebugInfoPDB',
  'LLVMDebugInfoDWARF', 'LLVMObjectYAML', 'LLVMObjCopy', 'LLVMCodeGenData', 'LLVMInterfaceStub', 'LLVMX86TargetMCA',
  'LLVMX86AsmParser', 'LLVMWebAssemblyDisassembler', 'LLVMWebAssemblyAsmParser', 'LLVMVEAsmParser',
  'LLVMSystemZDisassembler', 'LLVMSystemZAsmParser', 'LLVMSparcAsmParser', 'LLVMRISCVTargetMCA',
  'LLVMRISCVDisassembler', 'LLVMRISCVAsmParser', 'LLVMPowerPCAsmParser', 'LLVMMSP430AsmParser', 'LLVMMipsAsmParser',
  'LLVMLoongArchDisassembler', 'LLVMLoongArchAsmParser', 'LLVMLanaiDisassembler', 'LLVMLanaiAsmParser',
  'LLVMHexagonDisassembler', 'LLVMHexagonAsmParser', 'LLVMBPFAsmParser', 'LLVMAVRAsmParser',
  'LLVMAArch64Disassembler', 'LLVMAArch64AsmParser', 'LLVMObject', 'LLVMXCoreDesc', 'LLVMXCoreDisassembler',
  'LLVMX86Desc', 'LLVMX86Disassembler', 'LLVMWebAssemblyDesc', 'LLVMVEDesc', 'LLVMVEDisassembler',
  'LLVMSystemZDesc', 'LLVMSparcDesc', 'LLVMSparcDisassembler', 'LLVMRISCVDesc', 'LLVMPowerPCDesc',
  'LLVMPowerPCDisassembler', 'LLVMNVPTXDesc', 'LLVMMSP430Disassembler', 'LLVMMSP430Desc', 'LLVMMipsDesc',
  'LLVMMipsDisassembler', 'LLVMLoongArchDesc', 'LLVMLanaiDesc', 'LLVMHexagonDesc', 'LLVMBPFDesc',
  'LLVMBPFDisassembler', 'LLVMAVRDesc', 'LLVMAVRDisassembler', 'LLVMAArch64Desc', 'LLVMIRReader',
  'LLVMXCoreInfo', 'LLVMX86Info', 'LLVMWebAssemblyInfo', 'LLVMVEInfo', 'LLVMSystemZInfo', 'LLVMSparcInfo',
  'LLVMRISCVInfo', 'LLVMPowerPCInfo', 'LLVMNVPTXInfo', 'LLVMMSP430Info', 'LLVMMipsInfo', 'LLVMLoongArchInfo',
  'LLVMLanaiInfo', 'LLVMHexagonInfo', 'LLVMBPFInfo', 'LLVMAVRInfo', 'LLVMARMInfo', 'LLVMAMDGPUInfo',
  'LLVMAArch64Info', 'LLVMMCA', 'LLVMMCDisassembler', 'LLVMMCParser', 'LLVMDiff', 'LLVMAsmParser',
  'LLVMSandboxIR', 'LLVMAArch64Utils', 'LLVMCFGuard', 'LLVMFrontendHLSL', 'LLVMBitReader', 'LLVMTextAPI',
  'LLVMMC', 'LLVMCore', 'LLVMTableGenCommon', 'LLVMWindowsDriver', 'LLVMOrcTargetProcess', 'LLVMBinaryFormat',
  'LLVMFuzzerCLI', 'LLVMRemarks', 'LLVMTableGenBasic', 'LLVMWindowsManifest', 'LLVMTargetParser',
  'LLVMLineEditor', 'LLVMARMUtils', 'LLVMOrcShared', 'LLVMDebugInfoBTF', 'LLVMDebugInfoCodeView',
  'LLVMDebugInfoMSF', 'LLVMOption', 'LLVMFrontendOpenACC', 'LLVMExtensions', 'LLVMBitstreamReader',
  'LLVMCodeGenTypes', 'LLVMFileCheck', 'LLVMTableGen', 'LLVMSupport', 'LLVMDemangle',
  'Remarks', 'LTO', 'LLVM-C'
]

# Find all LLVM libraries
llvm_deps = []
foreach lib : required_llvm_libs
  dep = cpp.find_library(lib, dirs : llvm_lib_dir, required : false)
  if dep.found()
    llvm_deps += dep
  else
    warning('Library @0@ not found in @1@'.format(lib, llvm_lib_dir))
  endif
endforeach

# Create the comprehensive LLVM dependency  
llvm_dep = declare_dependency(
  include_directories : include_directories(llvm_inc_dir),
  dependencies : llvm_deps + system_libs
)

# DocTest dependency (header-only, just need include path)
doctest_dep = declare_dependency()

# Development tools targets

# Format target - code formatting with clang-format
python3 = find_program('python3', 'python', required : true)
format_script = files('tools/format.py')

run_target('format',
  command : [python3, format_script]
)

# Lint target - code linting with clang-tidy
lint_script = files('tools/lint.py')

run_target('lint',
  command : [python3, lint_script, '--build-dir', meson.current_build_dir()]
)

# Add subdirectories
subdir('MakeIndex')