#!/usr/bin/env python3
"""
Development Environment Setup Script

A simple convenience wrapper for setting up the development environment.
This script handles dependency installation and initial build setup.
"""

import os
import sys
import argparse
import subprocess
from pathlib import Path


def get_project_root():
    """Get the project root directory."""
    return Path(__file__).parent.parent


def run_script(script_path, *args):
    """Run a script with specified arguments."""
    project_root = get_project_root()
    
    cmd = [sys.executable, str(script_path)] + list(args)
    
    print(f"Running: {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, cwd=project_root, check=True)
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print(f"❌ Script failed with exit code: {e.returncode}")
        return False


def run_build_tool(command, build_type=None, *args):
    """Run the main build tool with specified command."""
    project_root = get_project_root()
    build_script = project_root / "tools" / "build.py"
    
    # Build the command with global arguments first
    cmd_args = []
    if build_type:
        cmd_args.extend(["--build-type", build_type])
    cmd_args.append(command)
    cmd_args.extend(args)
    
    return run_script(build_script, *cmd_args)


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description="Development environment setup convenience script")
    parser.add_argument("--deps-only", action="store_true",
                       help="Only setup dependencies, don't configure build")
    parser.add_argument("--build-type", default="debug", choices=["debug", "release"],
                       help="Build type to configure (default: debug)")
    parser.add_argument("--force-build", action="store_true",
                       help="Force rebuild of dependencies")
    parser.add_argument("--clean", action="store_true",
                       help="Clean before setup")
    
    args = parser.parse_args()
    
    project_root = get_project_root()
    
    print("🔧 Setting up development environment...")
    print(f"Project root: {project_root}")
    
    # Step 1: Clean if requested
    if args.clean:
        print("\n[STEP 1] Cleaning existing build...")
        success = run_build_tool("clean", "--deps")
        if not success:
            print("❌ Clean failed!")
            return 1
    
    # Step 2: Setup dependencies
    print("\n[STEP 2] Setting up dependencies...")
    setup_deps_script = project_root / "tools" / "setup-deps.py"
    
    deps_args = []
    if args.force_build:
        deps_args.append("--force-build")
    
    success = run_script(setup_deps_script, *deps_args)
    if not success:
        print("❌ Dependency setup failed!")
        return 1
    
    # Step 3: Configure build (unless deps-only)
    if not args.deps_only:
        print("\n[STEP 3] Configuring build system...")
        
        success = run_build_tool("setup", build_type=args.build_type)
        if not success:
            print("❌ Build setup failed!")
            return 1
        
        print(f"\n✅ Development environment ready!")
        print(f"Build type: {args.build_type}")
        print(f"To build the project, run: python scripts/dev-build.py")
        print(f"To run tests, run: python scripts/dev-build.py test")
    else:
        print(f"\n✅ Dependencies setup complete!")
        print(f"To configure build, run: python scripts/setup-dev.py")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())