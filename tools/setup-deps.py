#!/usr/bin/env python3
"""
CppGraphIndex Dependency Setup Script

This script automates Conan dependency management for the CppGraphIndex project.
It handles dependency installation, caching, and Meson integration.
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


def check_conan_available():
    """Check if Conan is available and get version."""
    try:
        result = run_command(["conan", "--version"])
        print(f"Found Conan: {result.stdout.strip()}")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Error: Conan is not available. Please install Conan 2.x")
        return False


def detect_profile():
    """Detect the best profile to use."""
    project_root = Path(__file__).parent.parent
    profiles_dir = project_root / "conan" / "profiles"
    
    default_profile = profiles_dir / "default"
    release_profile = profiles_dir / "release"
    
    if default_profile.exists():
        return str(default_profile)
    elif release_profile.exists():
        return str(release_profile)
    else:
        print("Warning: No project-specific profiles found, using Conan's default profile")
        return None


def install_dependencies(profile=None, build_type="Release", force_build=False):
    """Install dependencies using Conan."""
    project_root = Path(__file__).parent.parent
    
    # Prepare conan install command
    cmd = ["conan", "install", "."]
    
    if profile:
        cmd.extend(["--profile", profile])
    
    # Set build type
    cmd.extend(["-s", f"build_type={build_type}"])
    
    # Force building missing packages if requested
    if force_build:
        cmd.append("--build=missing")
    
    print(f"Installing dependencies for build type: {build_type}")
    run_command(cmd, cwd=project_root)


def setup_meson_integration(build_dir="builddir"):
    """Set up Meson integration with Conan-generated files."""
    project_root = Path(__file__).parent.parent
    conan_dir = project_root / "conan"
    
    # Check if Meson files exist (now in conan directory)
    toolchain_file = conan_dir / "conan_meson_native.ini"
    
    if not toolchain_file.exists():
        print("Warning: Conan Meson toolchain file not found. Dependencies may not be configured.")
        return False
    
    # Check for PkgConfig files (now in conan directory)
    pc_files = list(conan_dir.glob("*.pc"))
    
    if not pc_files:
        print("Warning: No PkgConfig files found. Dependencies may not be configured.")
        return False
    
    print("Conan-Meson integration files found:")
    print(f"  - Toolchain: {toolchain_file}")
    print(f"  - PkgConfig files: {len(pc_files)} dependency files")
    
    return True


def clean_conan_files():
    """Clean Conan-generated files."""
    project_root = Path(__file__).parent.parent
    conan_dir = project_root / "conan"
    
    # Files that might be in root directory
    root_files_to_clean = [
        "conanbuild.sh",
        "conanbuild.ps1",
        "conanrun.sh", 
        "conanrun.ps1",
        "conandata.yml"
    ]
    
    # Files that are now in conan directory
    conan_files_to_clean = [
        "conan_meson_native.ini",
        "conanbuild.bat",
        "conanrun.bat",
        "deactivate_conanbuild.bat",
        "deactivate_conanrun.bat",
        "conanvcvars.bat",
        "deactivate_conanvcvars.bat"
    ]
    
    cleaned = []
    
    # Clean root directory files
    for file_name in root_files_to_clean:
        file_path = project_root / file_name
        if file_path.exists():
            file_path.unlink()
            cleaned.append(file_name)
    
    # Clean conan directory files
    if conan_dir.exists():
        for file_name in conan_files_to_clean:
            file_path = conan_dir / file_name
            if file_path.exists():
                file_path.unlink()
                cleaned.append(f"conan/{file_name}")
        
        # Clean all .pc files from conan directory
        pc_files = list(conan_dir.glob("*.pc"))
        for pc_file in pc_files:
            pc_file.unlink()
            cleaned.append(f"conan/{pc_file.name}")
    
    if cleaned:
        print(f"Cleaned Conan files: {', '.join(cleaned[:5])}{'...' if len(cleaned) > 5 else ''} ({len(cleaned)} total)")
    else:
        print("No Conan files to clean")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description="Setup dependencies for CppGraphIndex")
    parser.add_argument("--profile", help="Conan profile to use (auto-detected if not specified)")
    parser.add_argument("--build-type", default="Release", choices=["Debug", "Release"],
                        help="Build type (default: Release)")
    parser.add_argument("--force-build", action="store_true", 
                        help="Force building of missing packages")
    parser.add_argument("--clean", action="store_true",
                        help="Clean Conan-generated files before setup")
    parser.add_argument("--meson-build-dir", default="builddir",
                        help="Meson build directory (default: builddir)")
    
    args = parser.parse_args()
    
    # Check if Conan is available
    if not check_conan_available():
        return 1
    
    # Clean if requested
    if args.clean:
        clean_conan_files()
    
    # Detect profile if not specified
    profile = args.profile
    if not profile:
        profile = detect_profile()
        if profile:
            print(f"Using detected profile: {profile}")
    
    try:
        # Install dependencies
        install_dependencies(profile, args.build_type, args.force_build)
        
        # Verify Meson integration
        if setup_meson_integration(args.meson_build_dir):
            print("\n[SUCCESS] Dependencies installed successfully!")
            print("Next steps:")
            print(f"  1. Run: meson setup {args.meson_build_dir}")
            print(f"  2. Run: ninja -C {args.meson_build_dir}")
        else:
            print("\n[WARNING] Dependencies installed but Meson integration may have issues")
            print("Try running 'conan install .' again or check the conanfile.py configuration")
            return 1
            
    except subprocess.CalledProcessError:
        print("\n[ERROR] Failed to install dependencies")
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())