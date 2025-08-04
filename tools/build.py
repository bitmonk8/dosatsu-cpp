#!/usr/bin/env python3
"""
CppGraphIndex Integrated Build Script

This script provides a comprehensive build wrapper that integrates Conan dependency 
management with Meson builds, providing convenient commands for development workflow.
"""

import os
import sys
import argparse
import subprocess
import time
import platform
from pathlib import Path


def setup_vs_environment():
    """Set up Visual Studio environment on Windows."""
    if platform.system() != "Windows":
        return {}  # No environment changes needed on non-Windows
    
    project_root = Path(__file__).parent.parent
    conanbuild_script = project_root / "conanbuild.bat"
    
    if not conanbuild_script.exists():
        print("[WARNING] conanbuild.bat not found. VS environment may not be set up properly.")
        return {}
    
    # Run conanbuild.bat and capture environment variables
    try:
        # Create a batch script to call conanbuild.bat and then echo all environment variables
        temp_script = project_root / "temp_env_capture.bat"
        with open(temp_script, 'w') as f:
            f.write('@echo off\n')
            f.write(f'call "{conanbuild_script}"\n')
            f.write('set\n')  # Output all environment variables
        
        result = subprocess.run([str(temp_script)], capture_output=True, text=True, 
                              cwd=project_root, shell=True)
        
        # Clean up temp script
        temp_script.unlink(missing_ok=True)
        
        if result.returncode != 0:
            print(f"[WARNING] Failed to set up VS environment: {result.stderr}")
            return {}
        
        # Parse environment variables from output
        env_vars = {}
        for line in result.stdout.split('\n'):
            if '=' in line and not line.startswith('echo '):
                key, _, value = line.partition('=')
                env_vars[key.strip()] = value.strip()
        
        print("[INFO] Visual Studio environment configured")
        return env_vars
        
    except Exception as e:
        print(f"[WARNING] Error setting up VS environment: {e}")
        return {}


def run_command(cmd, cwd=None, check=True, show_output=True, use_vs_env=True):
    """Run a command and return the result."""
    if show_output:
        print(f"Running: {' '.join(cmd)}")
    
    # Set up environment with Visual Studio tools if on Windows
    env = os.environ.copy()
    if use_vs_env and platform.system() == "Windows":
        vs_env = setup_vs_environment()
        env.update(vs_env)
    
    try:
        if show_output:
            # Stream output in real-time
            process = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE, 
                                     stderr=subprocess.STDOUT, text=True, bufsize=1, env=env)
            
            output_lines = []
            for line in process.stdout:
                print(line, end='')
                output_lines.append(line)
            
            process.wait()
            if check and process.returncode != 0:
                raise subprocess.CalledProcessError(process.returncode, cmd)
            
            return subprocess.CompletedProcess(cmd, process.returncode, 
                                             stdout=''.join(output_lines), stderr='')
        else:
            return subprocess.run(cmd, cwd=cwd, check=check, capture_output=True, text=True, env=env)
            
    except subprocess.CalledProcessError as e:
        print(f"\n❌ Command failed: {' '.join(cmd)}")
        print(f"Exit code: {e.returncode}")
        if hasattr(e, 'stdout') and e.stdout:
            print(f"Stdout: {e.stdout}")
        if hasattr(e, 'stderr') and e.stderr:
            print(f"Stderr: {e.stderr}")
        raise


def check_dependencies():
    """Check if required tools are available."""
    tools = {
        "conan": ["conan", "--version"],
        "meson": ["meson", "--version"], 
        "ninja": ["ninja", "--version"]
    }
    
    missing = []
    for tool, cmd in tools.items():
        try:
            result = run_command(cmd, show_output=False)
            print(f"[OK] {tool}: {result.stdout.strip()}")
        except (subprocess.CalledProcessError, FileNotFoundError):
            print(f"[MISSING] {tool}: Not found")
            missing.append(tool)
    
    if missing:
        print(f"\nMissing required tools: {', '.join(missing)}")
        print("Please install the missing tools before continuing.")
        return False
    
    return True


def setup_dependencies(force_build=False, build_type="Release", clean=False):
    """Set up dependencies using the setup-deps script."""
    project_root = Path(__file__).parent.parent
    setup_script = project_root / "tools" / "setup-deps.py"
    
    if not setup_script.exists():
        print("[ERROR] setup-deps.py script not found")
        return False
    
    cmd = [sys.executable, str(setup_script), "--build-type", build_type]
    
    if force_build:
        cmd.append("--force-build")
    
    if clean:
        cmd.append("--clean")
    
    try:
        run_command(cmd, cwd=project_root)
        return True
    except subprocess.CalledProcessError:
        return False


def setup_meson(build_dir="builddir", build_type="release", reconfigure=False):
    """Set up Meson build directory."""
    project_root = Path(__file__).parent.parent
    build_path = project_root / build_dir
    
    # Remove build directory if reconfiguring
    if reconfigure and build_path.exists():
        print(f"Removing existing build directory: {build_path}")
        import shutil
        shutil.rmtree(build_path)
    
    cmd = ["meson", "setup", build_dir, f"--buildtype={build_type}"]
    
    # Use Conan-generated toolchain file if available (now in conan directory)
    toolchain_file = project_root / "conan" / "conan_meson_native.ini"
    if toolchain_file.exists():
        cmd.extend(["--native-file", "conan/conan_meson_native.ini"])
        print(f"Using Conan toolchain: {toolchain_file}")
    
    try:
        run_command(cmd, cwd=project_root)
        return True
    except subprocess.CalledProcessError:
        return False


def build_project(build_dir="builddir", jobs=None, verbose=False):
    """Build the project using Ninja."""
    project_root = Path(__file__).parent.parent
    build_path = project_root / build_dir
    
    if not build_path.exists():
        print(f"[ERROR] Build directory {build_path} does not exist. Run setup first.")
        return False
    
    cmd = ["ninja", "-C", build_dir]
    
    if jobs:
        cmd.extend(["-j", str(jobs)])
    
    if verbose:
        cmd.append("-v")
    
    try:
        start_time = time.time()
        run_command(cmd, cwd=project_root)
        build_time = time.time() - start_time
        print(f"\n[SUCCESS] Build completed in {build_time:.2f} seconds")
        return True
    except subprocess.CalledProcessError:
        print(f"\n[ERROR] Build failed")
        return False


def run_tests(build_dir="builddir", verbose=False):
    """Run tests using Meson."""
    project_root = Path(__file__).parent.parent
    build_path = project_root / build_dir
    
    if not build_path.exists():
        print(f"[ERROR] Build directory {build_path} does not exist. Run setup and build first.")
        return False
    
    cmd = ["meson", "test", "-C", build_dir]
    
    if verbose:
        cmd.append("-v")
    
    try:
        start_time = time.time()
        run_command(cmd, cwd=project_root)
        test_time = time.time() - start_time
        print(f"\n[SUCCESS] Tests completed in {test_time:.2f} seconds")
        return True
    except subprocess.CalledProcessError:
        print(f"\n[ERROR] Tests failed")
        return False


def run_format(build_dir="builddir"):
    """Run code formatting."""
    project_root = Path(__file__).parent.parent
    
    cmd = ["ninja", "-C", build_dir, "format"]
    
    try:
        run_command(cmd, cwd=project_root)
        print(f"\n[SUCCESS] Code formatting completed")
        return True
    except subprocess.CalledProcessError:
        print(f"\n[ERROR] Code formatting failed")
        return False


def run_lint(build_dir="builddir"):
    """Run code linting."""
    project_root = Path(__file__).parent.parent
    
    cmd = ["ninja", "-C", build_dir, "lint"]
    
    try:
        run_command(cmd, cwd=project_root)
        print(f"\n[SUCCESS] Code linting completed")
        return True
    except subprocess.CalledProcessError:
        print(f"\n[ERROR] Code linting failed")
        return False


def clean_build(build_dir="builddir", deps=False):
    """Clean build artifacts."""
    project_root = Path(__file__).parent.parent
    build_path = project_root / build_dir
    
    if build_path.exists():
        print(f"Removing build directory: {build_path}")
        import shutil
        shutil.rmtree(build_path)
    
    if deps:
        # Clean Conan files
        setup_script = project_root / "tools" / "setup-deps.py"
        if setup_script.exists():
            cmd = [sys.executable, str(setup_script), "--clean"]
            try:
                run_command(cmd, cwd=project_root)
            except subprocess.CalledProcessError:
                print("Warning: Failed to clean dependency files")
    
    print("[SUCCESS] Clean completed")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description="Integrated build script for CppGraphIndex")
    parser.add_argument("--build-dir", default="builddir", help="Build directory (default: builddir)")
    parser.add_argument("--build-type", default="release", choices=["debug", "release"], 
                        help="Build type (default: release)")
    parser.add_argument("--jobs", "-j", type=int, help="Number of parallel jobs")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    
    subparsers = parser.add_subparsers(dest="command", help="Available commands")
    
    # Full build command
    full_parser = subparsers.add_parser("full", help="Full build: deps + setup + build + test")
    full_parser.add_argument("--force-build", action="store_true", help="Force rebuild dependencies")
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
    clean_parser.add_argument("--deps", action="store_true", help="Also clean dependency files")
    
    # Status command
    subparsers.add_parser("status", help="Show build system status")
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    # Check dependencies for most commands
    if args.command not in ["clean", "status"]:
        if not check_dependencies():
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
        print("\n[STEP 1] Setting up dependencies...")
        success = setup_dependencies(
            force_build=args.force_build, 
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
        else:
            print("\n[ERROR] Full build failed!")
    
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())