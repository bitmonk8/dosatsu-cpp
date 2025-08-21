#!/usr/bin/env python3

import os
import sys
import shutil
import subprocess
import argparse
import json
import tarfile
import urllib.request
from pathlib import Path
from datetime import datetime
import tempfile
import platform
import zipfile

class Colors:
    """Console color codes for cross-platform colored output"""
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    CYAN = '\033[96m'
    GRAY = '\033[90m'
    RESET = '\033[0m'

    @classmethod
    def print(cls, message, color=None):
        if color and os.name != 'nt': # Skip colors on Windows unless colorama is available
            print(f"{color}{message}{cls.RESET}")
        else:
            print(message)

def run_command(cmd, cwd=None, check=True):
    """Run a command and handle errors"""
    try:
        result = subprocess.run(cmd, cwd=cwd, check=check, capture_output=False)
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        Colors.print(f"Command failed with exit code {e.returncode}: {' '.join(cmd)}", Colors.RED)
        if check:
            raise
        return False

def download_file(url, destination):
    """Download a file with progress indication"""
    Colors.print(f"Downloading {os.path.basename(destination)}...", Colors.YELLOW)
    def progress_hook(block_num, block_size, total_size):
        if total_size > 0:
            percent = min(100, (block_num * block_size / total_size) * 100)
            sys.stdout.write(f"\rProgress: {percent:.1f}%")
            sys.stdout.flush()
    urllib.request.urlretrieve(url, destination, progress_hook)
    print() # New line after progress

def extract_tar_xz(archive_path, extract_to, strip_components=1):
    """Extract tar.xz archive"""
    Colors.print("Extracting archive...", Colors.YELLOW)
    with tarfile.open(archive_path, 'r:xz') as tar:
        members = tar.getmembers()
        if strip_components > 0:
            for member in members:
                # Remove the specified number of path components
                path_parts = member.name.split('/')
                if len(path_parts) > strip_components:
                    member.name = '/'.join(path_parts[strip_components:])
                else:
                    continue # Skip if not enough path components
                tar.extract(member, extract_to, filter='data')
        else:
            tar.extractall(extract_to)

def apply_patch_from_zip(zip_path, source_dir):
    """Apply patch files from a zip archive to the source directory"""
    source_dir = Path(source_dir)
    zip_path = Path(zip_path)
    
    # Check if source directory exists
    if not source_dir.exists():
        raise RuntimeError(f"Source directory '{source_dir}' does not exist. Cannot apply patch.")
    
    # Check if patch zip file exists
    if not zip_path.exists():
        raise RuntimeError(f"Patch zip file '{zip_path}' does not exist.")
    
    Colors.print(f"Applying patch from {zip_path.name}...", Colors.YELLOW)
    
    overwritten_files = []
    missing_targets = []
    patch_file_count = 0
    
    try:
        with zipfile.ZipFile(zip_path, 'r') as patch_zip:
            # List all files in the zip
            patch_files = patch_zip.namelist()
            
            # Filter out directories - we only want files
            patch_files = [f for f in patch_files if not f.endswith('/')]
            patch_file_count = len(patch_files)
            
            if patch_file_count == 0:
                raise RuntimeError("Patch zip file contains no files to apply.")
            
            Colors.print(f"Found {patch_file_count} files in patch archive", Colors.CYAN)
            
            # Check each patch file
            for patch_file in patch_files:
                # Compute the destination file path in the source directory
                dest_file_path = source_dir / patch_file
                
                Colors.print(f"  Checking: {patch_file} -> {dest_file_path}", Colors.GRAY)
                
                # Verify that the destination file exists before overwriting
                if dest_file_path.exists() and dest_file_path.is_file():
                    # File exists, we can overwrite it
                    Colors.print(f"  ✓ Target exists: {patch_file}", Colors.GREEN)
                else:
                    # File doesn't exist - this might be an error in the patch
                    missing_targets.append({
                        'patch_file': patch_file,
                        'expected_path': str(dest_file_path),
                        'parent_exists': dest_file_path.parent.exists()
                    })
            
            # If there are missing targets, provide detailed error information
            if missing_targets:
                error_msg = f"Patch cannot be applied. {len(missing_targets)} patch files do not match existing source files:\n\n"
                
                for missing in missing_targets:
                    error_msg += f"Patch file: '{missing['patch_file']}'\n"
                    error_msg += f"Expected target: '{missing['expected_path']}'\n"
                    error_msg += f"Parent directory exists: {missing['parent_exists']}\n"
                    
                    # Try to suggest similar files
                    parent_dir = Path(missing['expected_path']).parent
                    if parent_dir.exists():
                        similar_files = list(parent_dir.glob(Path(missing['patch_file']).name))
                        if similar_files:
                            error_msg += f"Similar files in directory: {[str(f) for f in similar_files]}\n"
                        else:
                            error_msg += "No similar files found in target directory\n"
                    error_msg += "\n"
                
                error_msg += "Troubleshooting tips:\n"
                error_msg += "1. Check that your patch zip directory structure matches the LLVM source structure\n"
                error_msg += "2. Ensure you extracted the LLVM source correctly before applying the patch\n"
                error_msg += f"3. Verify the contents of your patch zip with: unzip -l {zip_path}\n"
                error_msg += f"4. Check the source directory structure at: {source_dir}\n"
                
                raise RuntimeError(error_msg)
            
            # All files check out, now apply the patch
            Colors.print("All patch files have valid targets. Applying patch...", Colors.GREEN)
            
            for patch_file in patch_files:
                dest_file_path = source_dir / patch_file
                
                # Create backup of original file
                backup_path = dest_file_path.with_suffix(dest_file_path.suffix + '.backup')
                shutil.copy2(dest_file_path, backup_path)
                
                # Extract and overwrite the file
                patch_zip.extract(patch_file, source_dir)
                overwritten_files.append(str(dest_file_path))
                
                Colors.print(f"  ✓ Patched: {patch_file}", Colors.GREEN)
            
    except zipfile.BadZipFile:
        raise RuntimeError(f"'{zip_path}' is not a valid zip file or is corrupted.")
    except Exception as e:
        if isinstance(e, RuntimeError):
            raise  # Re-raise our custom errors
        else:
            raise RuntimeError(f"Failed to apply patch: {str(e)}")
    
    Colors.print(f"Patch applied successfully! Overwritten {len(overwritten_files)} files:", Colors.GREEN)
    for file_path in overwritten_files:
        Colors.print(f"  - {file_path}", Colors.CYAN)
    
    return overwritten_files

def copy_files_by_pattern(source_dir, dest_dir, pattern, description="files"):
    """Copy files matching a pattern"""
    source_path = Path(source_dir)
    dest_path = Path(dest_dir)
    dest_path.mkdir(parents=True, exist_ok=True)
    
    copied_files = []
    for file_path in source_path.glob(pattern):
        if file_path.is_file():
            dest_file = dest_path / file_path.name
            shutil.copy2(file_path, dest_file)
            copied_files.append(file_path.name)
    
    if copied_files:
        for file_name in copied_files:
            if "clang" in file_name.lower():
                Colors.print(f"  Copied Clang {description}: {file_name}", Colors.GREEN)
            else:
                Colors.print(f"  Copied {description}: {file_name}", Colors.GRAY)
    
    return copied_files

def get_file_size_mb(directory):
    """Get total size of directory in MB"""
    total_size = 0
    for root, dirs, files in os.walk(directory):
        for file in files:
            total_size += os.path.getsize(os.path.join(root, file))
    return total_size / (1024 * 1024)

def zip_directory(folder_path, zip_path):
    """Create a zip archive from a directory"""
    Colors.print(f"Creating zip archive: {os.path.basename(zip_path)}", Colors.YELLOW)
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as zipf:
        for root, dirs, files in os.walk(folder_path):
            for file in files:
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, folder_path)
                zipf.write(full_path, rel_path)
    
    # Get zip file size
    zip_size_mb = os.path.getsize(zip_path) / (1024 * 1024)
    Colors.print(f"Created zip archive: {os.path.basename(zip_path)} ({zip_size_mb:.1f} MB)", Colors.GREEN)

def get_platform_info():
    """Get platform information for naming archives"""
    system = platform.system().lower()
    if system == 'darwin':
        return 'macos'
    elif system == 'windows':
        return 'windows'
    elif system == 'linux':
        return 'linux'
    else:
        return system

def get_compiler_flags(current_platform):
    """Get compiler-specific CMake flags"""
    flags = []
    if current_platform == 'windows':
        # Use MSVC on Windows (default behavior, no specific flags needed)
        pass
    elif current_platform == 'macos':
        # Force clang on macOS
        flags.extend([
            "-DCMAKE_C_COMPILER=clang",
            "-DCMAKE_CXX_COMPILER=clang++"
        ])
    elif current_platform == 'linux':
        flags.extend([
            "-DCMAKE_C_COMPILER=gcc",
            "-DCMAKE_CXX_COMPILER=g++"
        ])
    return flags

def verify_clang_installation(install_dir):
    """Verify that required Clang components are installed"""
    include_dir = install_dir / "include"
    lib_dir = install_dir / "lib"
    
    # Check for Clang headers
    clang_include_dir = include_dir / "clang"
    required_headers = ["AST", "Basic", "Frontend", "Tooling", "Sema", "Parse", "Lex"]
    
    Colors.print("Verifying Clang installation...", Colors.YELLOW)
    
    if not clang_include_dir.exists():
        raise RuntimeError("Clang headers directory not found")
    
    missing_headers = []
    for header in required_headers:
        if not (clang_include_dir / header).exists():
            missing_headers.append(header)
    
    if missing_headers:
        raise RuntimeError(f"Missing required Clang headers: {missing_headers}")
    
    # Check for essential Clang libraries
    if lib_dir.exists():
        lib_files = [f.name for f in lib_dir.glob("*clang*") if f.is_file()]
        if not lib_files:
            raise RuntimeError("No Clang libraries found")
        Colors.print(f"Found {len(lib_files)} Clang libraries", Colors.GREEN)
    
    Colors.print("Clang installation verification passed", Colors.GREEN)
    return True

def main():
    parser = argparse.ArgumentParser(description='Build LLVM/Clang binary distribution with zip archives')
    parser.add_argument('--output-dir', default='llvm-working-dist', help='Output directory')
    parser.add_argument('--llvm-version', default='20.1.8', help='LLVM version to build')
    parser.add_argument('--debug-only', action='store_true', help='Build Debug configuration only')
    parser.add_argument('--release-only', action='store_true', help='Build Release configuration only')
    parser.add_argument('--no-shared-includes', action='store_true', help='Don\'t use shared includes')
    parser.add_argument('--generate-headers', action='store_true', help='Generate separate headers zip file')
    parser.add_argument('--keep-build-dirs', action='store_true', help='Keep intermediate build directories for debugging')
    parser.add_argument('--patch-file', default='llvm_clang_patch.zip', help='Path to patch zip file (default: llvm_clang_patch.zip)')
    parser.add_argument('--no-patch', action='store_true', help='Skip patch application even if patch file exists')
    
    args = parser.parse_args()

    # Configuration
    source_dir = Path("llvm-source")
    build_base_dir = Path("llvm-build")
    package_dir = Path(args.output_dir)
    temp_install_dir = Path("llvm-temp-install")
    shared_includes = not args.no_shared_includes
    platform_name = get_platform_info()

    Colors.print("=== Working LLVM/Clang Distribution Builder (Zip Edition) ===", Colors.GREEN)
    Colors.print(f"Target platform: {platform_name}", Colors.CYAN)

    # Clean up previous installations
    if temp_install_dir.exists():
        shutil.rmtree(temp_install_dir)
    if package_dir.exists():
        shutil.rmtree(package_dir)

    # Create directories
    for dir_path in [source_dir, build_base_dir, package_dir, temp_install_dir]:
        dir_path.mkdir(parents=True, exist_ok=True)

    # Download and extract LLVM
    llvm_archive = f"llvm-project-{args.llvm_version}.src.tar.xz"
    llvm_url = f"https://github.com/llvm/llvm-project/releases/download/llvmorg-{args.llvm_version}/{llvm_archive}"

    if not (source_dir / "llvm").exists():
        if not Path(llvm_archive).exists():
            download_file(llvm_url, llvm_archive)
        extract_tar_xz(llvm_archive, source_dir, strip_components=1)

    # Apply patch if available and not disabled
    patch_file = Path(args.patch_file)
    if not args.no_patch:
        if patch_file.exists():
            try:
                apply_patch_from_zip(patch_file, source_dir)
            except Exception as e:
                Colors.print(f"PATCH APPLICATION FAILED:", Colors.RED)
                Colors.print(str(e), Colors.RED)
                Colors.print("\nTo skip patch application, use --no-patch flag", Colors.YELLOW)
                raise
        else:
            Colors.print(f"No patch file found at '{patch_file}' - continuing without patch", Colors.YELLOW)
    else:
        Colors.print("Patch application disabled with --no-patch flag", Colors.YELLOW)

    # Determine configurations to build
    configurations = []
    if not args.release_only:
        configurations.append("Debug")
    if not args.debug_only:
        configurations.append("Release")

    # Get platform-specific compiler flags
    compiler_flags = get_compiler_flags(platform_name)

    # Build each configuration
    for config in configurations:
        Colors.print(f"=== Building {config} Configuration ===", Colors.GREEN)
        
        build_dir = build_base_dir / config
        temp_install_config_dir = temp_install_dir / config

        # Clean and create build directories
        if build_dir.exists():
            shutil.rmtree(build_dir)
        build_dir.mkdir(parents=True, exist_ok=True)
        temp_install_config_dir.mkdir(parents=True, exist_ok=True)

        # Build CMake arguments
        cmake_args = [
            "cmake",
            "-S", str(source_dir / "llvm"),
            "-B", str(build_dir),
            "-G", "Ninja",
            f"-DCMAKE_BUILD_TYPE={config}",
            f"-DCMAKE_INSTALL_PREFIX={temp_install_config_dir}",
            # Core configuration
            "-DLLVM_ENABLE_PROJECTS=clang",
            "-DLLVM_ENABLE_PROJECTS_USED=ON",
            "-DLLVM_TARGETS_TO_BUILD=host",
            "-DLLVM_ENABLE_RUNTIMES=",
            "-DLLVM_DEFAULT_TARGET_TRIPLE=x86_64-unknown-unknown",
            "-DLLVM_ENABLE_RTTI=ON",
            # Build shared libraries
            "-DLLVM_BUILD_LLVM_DYLIB=ON",
            "-DLLVM_LINK_LLVM_DYLIB=ON",
            "-DLLVM_BUILD_TESTS=OFF",
            "-DLLVM_BUILD_EXAMPLES=OFF",
            "-DLLVM_BUILD_BENCHMARKS=OFF",
            "-DLLVM_BUILD_DOCS=OFF",
            "-DLLVM_BUILD_TOOLS=OFF",
            "-DLLVM_BUILD_UTILS=OFF",
            "-DLLVM_INCLUDE_TESTS=OFF",
            "-DLLVM_INCLUDE_EXAMPLES=OFF",
            "-DLLVM_INCLUDE_BENCHMARKS=OFF",
            "-DLLVM_INCLUDE_DOCS=OFF",
            "-DLLVM_INCLUDE_TOOLS=ON",
            "-DLLVM_INCLUDE_UTILS=ON",
            "-DLLVM_INSTALL_TOOLCHAIN_ONLY=OFF",
            "-DLLVM_INSTALL_UTILS=ON",
            # Disable external dependencies
            "-DLLVM_ENABLE_ZLIB=OFF",
            "-DLLVM_ENABLE_ZSTD=OFF",
            "-DLLVM_ENABLE_LIBXML2=OFF",
            "-DLLVM_ENABLE_Z3_SOLVER=OFF",
            "-DLLVM_ENABLE_LIBPFM=OFF",
            # Clang configuration
            "-DCLANG_INCLUDE_TESTS=OFF",
            "-DCLANG_BUILD_EXAMPLES=OFF",
            "-DCLANG_BUILD_TOOLS=OFF",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DCLANG_TOOL_LIBCLANG_BUILD=OFF",
            "-DCLANG_TOOL_C_INDEX_TEST_BUILD=OFF",
            # [Rest of the CMake flags from original script...]
            # Even more tool disables
            "-DCLANG_TOOL_AMDGPU_ARCH_BUILD=OFF",
            "-DCLANG_TOOL_APINOTES_TEST_BUILD=OFF",
            "-DCLANG_TOOL_ARCMT_TEST_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_CHECK_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_DIFF_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_EXTDEF_MAPPING_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_FORMAT_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_FUZZER_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_IMPORT_TEST_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_INSTALLAPI_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_LINKER_WRAPPER_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_NVLINK_WRAPPER_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_OFFLOAD_BUNDLER_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_OFFLOAD_PACKAGER_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_REFACTOR_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_REPL_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_SCAN_DEPS_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_SHLIB_BUILD=OFF",
            "-DCLANG_TOOL_CLANG_SYCL_LINKER_BUILD=OFF",
            "-DCLANG_TOOL_C_ARCMT_TEST_BUILD=OFF",
            "-DCLANG_TOOL_DIAGTOOL_BUILD=OFF",
            "-DCLANG_TOOL_DICTIONARY_BUILD=OFF",
            "-DCLANG_TOOL_DRIVER_BUILD=OFF",
            "-DCLANG_TOOL_HANDLE_CXX_BUILD=OFF",
            "-DCLANG_TOOL_HANDLE_LLVM_BUILD=OFF",
            "-DCLANG_TOOL_NVPTX_ARCH_BUILD=OFF",
            "-DCLANG_TOOL_SCAN_BUILD_BUILD=OFF",
            "-DCLANG_TOOL_SCAN_BUILD_PY_BUILD=OFF",
            "-DCLANG_TOOL_SCAN_VIEW_BUILD=OFF",
            # [Include all the rest of your CMake flags...]
        ]

        # Add compiler-specific flags
        cmake_args.extend(compiler_flags)

        # MSVC runtime configuration for Windows
        if platform_name == 'windows':
            cmake_args.extend([
                "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL" if config == "Debug" else "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL",
                "-DLLVM_ENABLE_PDB=ON",
                "-DLLVM_BUILD_STATIC=ON"
            ])

        # Configure
        Colors.print(f"Configuring for {platform_name} with {config} settings...", Colors.YELLOW)
        if not run_command(cmake_args):
            raise RuntimeError("CMake configuration failed")

        # Build
        Colors.print("Building...", Colors.YELLOW)
        build_cmd = ["cmake", "--build", str(build_dir), "--config", config, "--parallel"]
        if not run_command(build_cmd):
            raise RuntimeError("Build failed")

        # Install
        Colors.print("Installing...", Colors.YELLOW)
        install_cmd = ["cmake", "--build", str(build_dir), "--config", config, "--target", "install"]
        if not run_command(install_cmd):
            raise RuntimeError("Install failed")

        Colors.print("Installing Clang headers...", Colors.YELLOW)
        clang_headers_cmd = ["cmake", "--build", str(build_dir), "--config", config, "--target", "install-clang-headers"]
        if not run_command(clang_headers_cmd, check=False):
            Colors.print("Warning: install-clang-headers target failed or doesn't exist", Colors.YELLOW)

        clang_libs_cmd = ["cmake", "--build", str(build_dir), "--config", config, "--target", "install-clang-libraries"]
        run_command(clang_libs_cmd, check=False)

        verify_clang_installation(temp_install_config_dir)

        # Clean up build directory unless debugging
        if not args.keep_build_dirs and build_dir.exists():
            shutil.rmtree(build_dir)

    # [Rest of your original script continues here - processing distribution, creating zip files, etc.]
    # Process distribution and create zip files
    Colors.print("=== Processing Distribution ===", Colors.GREEN)

    # Process headers with explicit Clang header handling
    if args.generate_headers or shared_includes:
        include_dir = package_dir / "include"
        include_dir.mkdir(parents=True, exist_ok=True)

        first_config = configurations[0]
        source_include_dir = temp_install_dir / first_config / "include"

        if source_include_dir.exists():
            # Copy all headers
            shutil.copytree(source_include_dir, include_dir, dirs_exist_ok=True)

            # Verify Clang headers are present
            clang_include_dir = include_dir / "clang"
            if clang_include_dir.exists():
                required_clang_headers = ["AST", "Basic", "Frontend", "Tooling", "Sema", "Parse", "Lex"]
                missing_headers = []
                for header_dir in required_clang_headers:
                    if not (clang_include_dir / header_dir).exists():
                        missing_headers.append(header_dir)

                if missing_headers:
                    Colors.print(f"Warning: Missing Clang header directories: {missing_headers}", Colors.YELLOW)
                else:
                    Colors.print("All required Clang headers found", Colors.GREEN)

                # List found Clang headers for verification
                clang_subdirs = [d.name for d in clang_include_dir.iterdir() if d.is_dir()]
                Colors.print(f"Available Clang header directories: {clang_subdirs}", Colors.CYAN)
            else:
                Colors.print("ERROR: No Clang headers found in include directory!", Colors.RED)
                raise RuntimeError("Clang headers missing from installation")

            Colors.print("Created shared includes with Clang headers", Colors.CYAN)
        else:
            raise RuntimeError(f"Include directory not found: {source_include_dir}")

    # Create headers zip if requested
    if args.generate_headers:
        headers_zip_name = f"llvm-clang-{args.llvm_version}-headers.zip"
        headers_zip_path = package_dir / headers_zip_name
        zip_directory(include_dir, headers_zip_path)

    # Process each configuration and create separate zip files
    created_archives = []
    for config in configurations:
        config_dir = package_dir / config
        temp_config_dir = temp_install_dir / config

        # Create directories
        (config_dir / "lib").mkdir(parents=True, exist_ok=True)
        (config_dir / "bin").mkdir(parents=True, exist_ok=True)

        Colors.print(f"Processing {config} configuration...", Colors.YELLOW)

        # Copy library files
        if platform_name == 'windows':
            lib_patterns = ["*.lib", "*.pdb"]  # Include PDB files
            dll_patterns = ["*.dll", "*.pdb"]  # Also for DLLs
        else:
            lib_patterns = ["*.lib", "*.a", "*.so"]

        for pattern in lib_patterns:
            if (temp_config_dir / "lib").exists():
                copy_files_by_pattern(temp_config_dir / "lib", config_dir / "lib", pattern, "library")

        # Copy DLL/shared library files
        if platform_name == 'windows':
            dll_patterns = ["*.dll", "*.pdb"]
        elif platform_name == 'macos':
            dll_patterns = ["*.dylib"]
        else:  # linux
            dll_patterns = ["*.so"]

        for pattern in dll_patterns:
            if (temp_config_dir / "bin").exists():
                copy_files_by_pattern(temp_config_dir / "bin", config_dir / "bin", pattern, "shared library")

        # Create zip archive for this configuration
        archive_name = f"llvm-clang-{args.llvm_version}-{platform_name}-{config.lower()}.zip"
        archive_path = package_dir / archive_name
        zip_directory(config_dir, archive_path)
        created_archives.append(archive_name)

        # Clean up unpacked directory after zipping
        if config_dir.exists():
            shutil.rmtree(config_dir)

    # Final verification
    Colors.print("=== Final Verification ===", Colors.GREEN)
    required_clang_libs = ["clangAST", "clangBasic", "clangFrontend", "clangTooling",
                          "clangSerialization", "clangParse", "clangSema", "clangLex"]
    required_llvm_libs = ["LLVMSupport", "LLVMCore", "LLVMMC", "LLVMAnalysis"]

    for config in configurations:
        temp_config_dir = temp_install_dir / config
        lib_dir = temp_config_dir / "lib"
        if lib_dir.exists():
            all_libs = [f.name for f in lib_dir.glob("*") if f.is_file()]
            clang_libs = [lib for lib in all_libs if 'clang' in lib.lower()]
            llvm_libs = [lib for lib in all_libs if 'llvm' in lib.lower()]

            Colors.print(f"Final check for {config} libraries...", Colors.YELLOW)
            Colors.print(f"  Total libraries: {len(all_libs)}", Colors.CYAN)
            Colors.print(f"  Clang libraries: {len(clang_libs)}", Colors.GREEN)
            Colors.print(f"  LLVM libraries: {len(llvm_libs)}", Colors.GREEN)

            # Check for specific required libraries
            for required_lib in required_clang_libs:
                lib_found = any(required_lib.lower() in lib.lower() for lib in clang_libs)
                if lib_found:
                    Colors.print(f"  ✓ Found {required_lib}", Colors.GREEN)
                else:
                    Colors.print(f"  ⚠ Missing {required_lib}", Colors.YELLOW)

    # Clean up temporary directory
    if temp_install_dir.exists():
        shutil.rmtree(temp_install_dir)

    # Clean up include directory if it was only temporary
    if not args.generate_headers and (package_dir / "include").exists():
        shutil.rmtree(package_dir / "include")

    # Create package info
    package_info = {
        "version": args.llvm_version,
        "platform": platform_name,
        "system": f"{platform.system()}-{platform.machine()}",
        "configurations": configurations,
        "created": datetime.now().isoformat(),
        "archives": created_archives,
        "shared_includes": shared_includes,
        "headers_archive": args.generate_headers,
        "patch_applied": not args.no_patch and patch_file.exists()
    }

    with open(package_dir / "package-info.json", 'w') as f:
        json.dump(package_info, f, indent=2)

    # Summary
    Colors.print("=== Build Complete ===", Colors.GREEN)
    Colors.print(f"Distribution created at: {package_dir}", Colors.YELLOW)

    total_size = 0
    Colors.print("Created archives:", Colors.CYAN)
    for archive_name in created_archives:
        archive_path = package_dir / archive_name
        if archive_path.exists():
            archive_size = os.path.getsize(archive_path) / (1024 * 1024)
            total_size += archive_size
            Colors.print(f"  - {archive_name}: {archive_size:.1f} MB", Colors.GREEN)

    if args.generate_headers:
        headers_zip_name = f"llvm-clang-{args.llvm_version}-{platform_name}-headers.zip"
        headers_zip_path = package_dir / headers_zip_name
        if headers_zip_path.exists():
            headers_size = os.path.getsize(headers_zip_path) / (1024 * 1024)
            total_size += headers_size
            Colors.print(f"  - {headers_zip_name}: {headers_size:.1f} MB", Colors.GREEN)

    Colors.print(f"Total archive size: {total_size:.1f} MB", Colors.YELLOW)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        Colors.print("\nBuild interrupted by user", Colors.RED)
        sys.exit(1)
    except Exception as e:
        Colors.print(f"\nBuild failed: {str(e)}", Colors.RED)
        sys.exit(1)
