#!/usr/bin/env python3
"""
CppGraphIndex Integrated Build Script

This script provides a comprehensive build wrapper for CppGraphIndex using 
Meson with git subproject dependencies, providing convenient commands for development workflow.
"""

import os
import sys
import argparse
import subprocess
import time
import platform
from pathlib import Path


def run_command(cmd, cwd=None, check=True, env=None):
    """Run a command and return success status."""
    print(f"Running: {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, cwd=cwd, check=check, env=env)
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print(f"Error running command: {' '.join(cmd)}")
        print(f"Exit code: {e.returncode}")
        return False
    except FileNotFoundError:
        print(f"Command not found: {cmd[0]}")
        return False


def check_dependencies():
    """Check if required tools are available."""
    tools = {
        "meson": ["meson", "--version"],
        "ninja": ["ninja", "--version"],
        "cmake": ["cmake", "--version"],
    }
    
    all_available = True
    for tool, cmd in tools.items():
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            version = result.stdout.strip().split('\n')[0]
            print(f"✓ {tool.capitalize()}: {version}")
        except (subprocess.CalledProcessError, FileNotFoundError):
            print(f"✗ {tool.capitalize()}: Not found")
            all_available = False
    
    return all_available


def setup_dependencies(build_type="Release", clean=False):
    """Setup dependencies (now just verification since they're git subprojects)."""
    project_root = Path(__file__).parent.parent
    
    if clean:
        print("[INFO] Cleaning build files...")
        clean_build(None, False)
    
    print("[INFO] Verifying subproject structure...")
    
    # Check subproject structure
    subprojects_dir = project_root / "subprojects"
    llvm_wrap = subprojects_dir / "llvm.wrap"
    llvm_meson = subprojects_dir / "packagefiles" / "llvm-project" / "meson.build"
    
    if not subprojects_dir.exists():
        print("[ERROR] subprojects directory not found")
        return False
    
    if not llvm_wrap.exists():
        print("[ERROR] LLVM wrap file not found")
        return False
    
    if not llvm_meson.exists():
        print("[ERROR] LLVM meson.build not found")
        return False
    
    print("[INFO] ✓ Dependencies verified (git subprojects)")
    return True


def setup_meson(build_dir="builddir", build_type="debug", reconfigure=False):
    """Set up Meson build directory."""
    project_root = Path(__file__).parent.parent
    build_path = project_root / build_dir
    
    if reconfigure and build_path.exists():
        print(f"[INFO] Reconfiguring build directory: {build_dir}")
        import shutil
        shutil.rmtree(build_path)
    
    if build_path.exists():
        print(f"[INFO] Build directory {build_dir} already exists")
        return True
    
    cmd = ["meson", "setup", build_dir, f"--buildtype={build_type}"]
    
    # Add compiler-specific flags for Windows
    if platform.system() == "Windows":
        print("[INFO] Configuring for Windows with MSVC")
        # Meson will auto-detect MSVC if available
    
    print(f"[INFO] Setting up Meson build ({build_type})...")
    success = run_command(cmd, cwd=project_root)
    
    if success:
        print(f"[INFO] ✓ Meson configured in {build_dir}")
    else:
        print(f"[ERROR] Failed to configure Meson")
    
    return success


def build_project(build_dir="builddir", jobs=None, verbose=False):
    """Build the project using Ninja."""
    project_root = Path(__file__).parent.parent
    
    cmd = ["ninja", "-C", build_dir]
    
    if jobs:
        cmd.extend(["-j", str(jobs)])
    
    if verbose:
        cmd.append("-v")
    
    print(f"[INFO] Building project in {build_dir}...")
    success = run_command(cmd, cwd=project_root)
    
    if success:
        print("[INFO] ✓ Build completed successfully")
    else:
        print("[ERROR] Build failed")
    
    return success


def run_tests(build_dir="builddir", verbose=False):
    """Run tests using Meson test."""
    project_root = Path(__file__).parent.parent
    
    cmd = ["meson", "test", "-C", build_dir]
    
    if verbose:
        cmd.append("-v")
    
    print(f"[INFO] Running tests in {build_dir}...")
    success = run_command(cmd, cwd=project_root)
    
    if success:
        print("[INFO] ✓ All tests passed")
    else:
        print("[ERROR] Some tests failed")
    
    return success


def run_format(build_dir="builddir"):
    """Run code formatting."""
    project_root = Path(__file__).parent.parent
    
    cmd = ["meson", "compile", "-C", build_dir, "format"]
    
    print("[INFO] Running code formatting...")
    success = run_command(cmd, cwd=project_root)
    
    if success:
        print("[INFO] ✓ Code formatting completed")
    else:
        print("[ERROR] Code formatting failed")
    
    return success


def run_lint(build_dir="builddir"):
    """Run code linting."""
    project_root = Path(__file__).parent.parent
    
    cmd = ["meson", "compile", "-C", build_dir, "lint"]
    
    print("[INFO] Running code linting...")
    success = run_command(cmd, cwd=project_root)
    
    if success:
        print("[INFO] ✓ Code linting completed")
    else:
        print("[ERROR] Code linting failed")
    
    return success


def clean_build(build_dir=None, clean_deps=False):
    """Clean build artifacts."""
    project_root = Path(__file__).parent.parent
    
    if build_dir:
        build_dirs = [build_dir]
    else:
        build_dirs = ["builddir", "builddir_debug", "builddir_release", "builddir_msvc"]
    
    cleaned = []
    for bd in build_dirs:
        build_path = project_root / bd
        if build_path.exists():
            import shutil
            shutil.rmtree(build_path)
            cleaned.append(bd)
    
    if clean_deps:
        # Clean subproject checkouts
        subprojects_dir = project_root / "subprojects"
        if subprojects_dir.exists():
            for item in subprojects_dir.iterdir():
                if item.is_dir() and item.name != "packagefiles":
                    import shutil
                    shutil.rmtree(item)
                    cleaned.append(f"subprojects/{item.name}")
    
    if cleaned:
        print(f"[INFO] Cleaned: {', '.join(cleaned)}")
    else:
        print("[INFO] Nothing to clean")


def main():
    """Main function."""
    parser = argparse.ArgumentParser(
        description="Integrated build tool for CppGraphIndex",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python tools/build.py full              # Complete build workflow
  python tools/build.py deps              # Setup dependencies only
  python tools/build.py build             # Build only
  python tools/build.py test              # Run tests only
        """
    )
    
    parser.add_argument("--build-dir", default="builddir", help="Build directory")
    parser.add_argument("--build-type", choices=["debug", "release"], default="debug", 
                       help="Build type")
    parser.add_argument("--jobs", "-j", type=int, help="Number of parallel jobs")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    
    subparsers = parser.add_subparsers(dest="command", help="Available commands")
    
    # Full build command
    full_parser = subparsers.add_parser("full", help="Complete build workflow")
    full_parser.add_argument("--reconfigure", action="store_true", help="Reconfigure Meson")
    
    # Individual commands
    subparsers.add_parser("deps", help="Setup dependencies only")
    subparsers.add_parser("setup", help="Setup Meson build directory")
    subparsers.add_parser("build", help="Build project")
    subparsers.add_parser("test", help="Run tests")
    subparsers.add_parser("format", help="Run code formatting")
    subparsers.add_parser("lint", help="Run code linting")
    
    # Clean command
    clean_parser = subparsers.add_parser("clean", help="Clean build artifacts")
    clean_parser.add_argument("--deps", action="store_true", help="Also clean subproject checkouts")
    
    # Status command
    subparsers.add_parser("status", help="Show build system status")
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    # Check dependencies for most commands
    if args.command not in ["clean", "status"]:
        if not check_dependencies():
            print("[ERROR] Missing required tools. Please install meson, ninja, and cmake.")
            return 1
    
    success = True
    
    if args.command == "status":
        print("Build System Status:")
        check_dependencies()
        return 0
    
    elif args.command == "clean":
        clean_build(args.build_dir, args.deps)
        return 0
    
    elif args.command == "deps":
        success = setup_dependencies(build_type=args.build_type.capitalize())
    
    elif args.command == "setup":
        success = setup_meson(args.build_dir, args.build_type)
    
    elif args.command == "build":
        success = build_project(args.build_dir, args.jobs, args.verbose)
    
    elif args.command == "test":
        success = run_tests(args.build_dir, args.verbose)
    
    elif args.command == "format":
        success = run_format(args.build_dir)
    
    elif args.command == "lint":
        success = run_lint(args.build_dir)
    
    elif args.command == "full":
        print("[INFO] Starting full build process...")
        
        # Step 1: Dependencies
        print("\n[STEP 1] Verifying dependencies...")
        success = setup_dependencies(
            build_type=args.build_type.capitalize(),
            clean=args.reconfigure
        )
        
        if success:
            # Step 2: Meson setup
            print("\n[STEP 2] Setting up Meson...")
            success = setup_meson(args.build_dir, args.build_type, args.reconfigure)
        
        if success:
            # Step 3: Build
            print("\n[STEP 3] Building project...")
            success = build_project(args.build_dir, args.jobs, args.verbose)
        
        if success:
            # Step 4: Test
            print("\n[STEP 4] Running tests...")
            success = run_tests(args.build_dir, args.verbose)
        
        if success:
            print("\n[SUCCESS] Full build completed successfully!")
            print("\nNote: LLVM was built from git subproject automatically")
        else:
            print("\n[ERROR] Full build failed!")
    
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())