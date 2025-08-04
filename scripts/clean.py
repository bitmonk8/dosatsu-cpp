#!/usr/bin/env python3
"""
Clean Build Script

A simple convenience wrapper for cleaning build artifacts and dependencies.
"""

import os
import sys
import argparse
import subprocess
from pathlib import Path


def get_project_root():
    """Get the project root directory."""
    return Path(__file__).parent.parent


def run_build_tool(command, *args):
    """Run the main build tool with specified command."""
    project_root = get_project_root()
    build_script = project_root / "tools" / "build.py"
    
    cmd = [sys.executable, str(build_script), command] + list(args)
    
    print(f"Running: {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, cwd=project_root, check=True)
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print(f"❌ Clean failed with exit code: {e.returncode}")
        return False


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description="Clean build artifacts convenience script")
    parser.add_argument("--deps", action="store_true", 
                       help="Also clean dependency files (Conan cache)")
    parser.add_argument("--build-dir", default="builddir",
                       help="Build directory to clean (default: builddir)")
    parser.add_argument("--all", action="store_true",
                       help="Clean everything: build dirs + dependencies")
    
    args = parser.parse_args()
    
    # Determine what to clean
    if args.all:
        print("🧹 Cleaning everything: build directories and dependencies")
        clean_args = ["--deps"]
    elif args.deps:
        print("🧹 Cleaning build artifacts and dependencies")
        clean_args = ["--deps"]
    else:
        print("🧹 Cleaning build artifacts")
        clean_args = []
    
    # Add build directory if specified
    if args.build_dir != "builddir":
        clean_args.extend(["--build-dir", args.build_dir])
    
    # Execute the clean command
    success = run_build_tool("clean", *clean_args)
    
    if success:
        print("✅ Clean completed successfully!")
    else:
        print("❌ Clean failed!")
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())