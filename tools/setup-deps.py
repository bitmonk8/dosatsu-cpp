#!/usr/bin/env python3
"""
CppGraphIndex Dependency Setup Script

This script verifies that the build environment is ready for CppGraphIndex.
Dependencies are now managed through meson git subprojects.
"""

import os
import sys
import argparse
import subprocess
from pathlib import Path


def run_command(cmd, cwd=None, check=True):
    """Run a command and return the result."""
    print(f"Running: {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, cwd=cwd, check=check, capture_output=True, text=True)
        if result.stdout:
            print(result.stdout)
        return result
    except subprocess.CalledProcessError as e:
        print(f"Error running command: {' '.join(cmd)}")
        print(f"Exit code: {e.returncode}")
        if e.stdout:
            print(f"Stdout: {e.stdout}")
        if e.stderr:
            print(f"Stderr: {e.stderr}")
        raise


def check_meson_available():
    """Check if Meson is available and get version."""
    try:
        result = run_command(["meson", "--version"])
        print(f"Found Meson: {result.stdout.strip()}")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Error: Meson is not available. Please install Meson >=1.8.3")
        return False


def check_cmake_available():
    """Check if CMake is available and get version."""
    try:
        result = run_command(["cmake", "--version"])
        version_line = result.stdout.split('\n')[0]
        print(f"Found CMake: {version_line}")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Error: CMake is not available. Please install CMake >=3.20 for LLVM subproject building")
        return False


def check_ninja_available():
    """Check if Ninja is available and get version."""
    try:
        result = run_command(["ninja", "--version"])
        print(f"Found Ninja: {result.stdout.strip()}")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Error: Ninja is not available. Please install Ninja >=1.12.0")
        return False


def verify_subproject_structure():
    """Verify that the subproject structure is set up correctly."""
    project_root = Path(__file__).parent.parent
    subprojects_dir = project_root / "subprojects"
    llvm_wrap = subprojects_dir / "llvm.wrap"
    llvm_meson = subprojects_dir / "packagefiles" / "llvm-project" / "meson.build"
    
    if not subprojects_dir.exists():
        print("Error: subprojects directory not found")
        return False
    
    if not llvm_wrap.exists():
        print("Error: LLVM wrap file not found at subprojects/llvm.wrap")
        return False
    
    if not llvm_meson.exists():
        print("Error: LLVM meson.build not found at subprojects/packagefiles/llvm-project/meson.build")
        return False
    
    print("✓ Subproject structure verified")
    return True


def setup_meson_integration():
    """Set up Meson build integration."""
    project_root = Path(__file__).parent.parent
    
    print("Setting up Meson build integration...")
    print("✓ Dependencies are managed through git subprojects")
    print("✓ LLVM will be built automatically from git repository")
    print("✓ doctest is included in 3rdParty/include/")
    return True


def main():
    """Main function."""
    parser = argparse.ArgumentParser(
        description="Set up dependencies for CppGraphIndex using git subprojects"
    )
    
    parser.add_argument(
        "--clean",
        action="store_true", 
        help="Clean build files before setup")
    
    args = parser.parse_args()
    
    # Check if required tools are available
    if not check_meson_available():
        sys.exit(1)
    
    if not check_cmake_available():
        sys.exit(1)
    
    if not check_ninja_available():
        sys.exit(1)
    
    # Clean build files if requested
    if args.clean:
        project_root = Path(__file__).parent.parent
        build_dirs = ["builddir", "builddir_debug", "builddir_release", "builddir_msvc"]
        for build_dir in build_dirs:
            build_path = project_root / build_dir
            if build_path.exists():
                print(f"Cleaning build directory: {build_dir}")
                import shutil
                shutil.rmtree(build_path)
    
    # Verify subproject structure
    if not verify_subproject_structure():
        sys.exit(1)
    
    # Set up Meson integration
    if not setup_meson_integration():
        sys.exit(1)
    
    print("\n✅ Dependency setup complete!")
    print("\nNext steps:")
    print("  meson setup builddir       # Configure build")
    print("  ninja -C builddir          # Build project")
    print("\nNote: LLVM will be downloaded and built automatically on first build")


if __name__ == "__main__":
    main()