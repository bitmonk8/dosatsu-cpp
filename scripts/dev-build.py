#!/usr/bin/env python3
"""
Development Build Script

A simple convenience wrapper for common development build tasks.
This script provides easy access to the most frequently used build operations.
"""

import os
import sys
import argparse
import subprocess
from pathlib import Path


def get_project_root():
    """Get the project root directory."""
    return Path(__file__).parent.parent


def run_build_tool(command, global_args=None, *args):
    """Run the main build tool with specified command."""
    project_root = get_project_root()
    build_script = project_root / "tools" / "build.py"
    
    # Build command with global args first, then command, then command args
    cmd = [sys.executable, str(build_script)]
    if global_args:
        cmd.extend(global_args)
    cmd.append(command)
    cmd.extend(args)
    
    print(f"Running: {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, cwd=project_root, check=True)
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print(f"❌ Build failed with exit code: {e.returncode}")
        return False


def run_direct_tool(tool_name, *args):
    """Run a tool directly from the tools directory."""
    project_root = get_project_root()
    tool_script = project_root / "tools" / f"{tool_name}.py"
    
    cmd = [sys.executable, str(tool_script)] + list(args)
    
    print(f"Running: {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, cwd=project_root, check=True)
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print(f"❌ Tool failed with exit code: {e.returncode}")
        return False


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description="Development build convenience script")
    parser.add_argument("action", nargs="?", default="full", 
                       choices=["full", "quick", "test", "format", "lint", "status"],
                       help="Build action to perform (default: full)")
    
    parser.add_argument("--debug", action="store_true", 
                       help="Use debug build type instead of release")
    parser.add_argument("--verbose", "-v", action="store_true", 
                       help="Verbose output")
    parser.add_argument("--jobs", "-j", type=int, 
                       help="Number of parallel jobs")
    
    args = parser.parse_args()
    
    # Map convenience actions to build tool commands
    action_map = {
        "full": "full",      # Complete build: deps + setup + build + test
        "quick": "build",    # Just build the project
        "test": "test",      # Run tests only
        "format": "format",  # Format code
        "lint": "lint",      # Lint code
        "status": "status"   # Show build status
    }
    
    # Build command arguments
    build_args = []
    
    if args.debug:
        build_args.extend(["--build-type", "debug"])
    
    if args.verbose:
        build_args.append("--verbose")
    
    if args.jobs:
        build_args.extend(["--jobs", str(args.jobs)])
    
    # Add action-specific arguments
    if args.action == "full":
        if args.debug:
            print("🔧 Running full development build (debug mode)")
        else:
            print("🚀 Running full development build (release mode)")
    elif args.action == "quick":
        print("⚡ Running quick build")
    elif args.action == "test":
        print("🧪 Running tests")
    elif args.action == "format":
        print("✨ Formatting code")
    elif args.action == "lint":
        print("🔍 Linting code")
    elif args.action == "status":
        print("📊 Checking build status")
    
    # Execute the build command
    command = action_map[args.action]
    
    # Some commands can run directly without build system setup
    if args.action in ["format", "lint"]:
        success = run_direct_tool(args.action)
    else:
        success = run_build_tool(command, global_args=build_args)
    
    if success:
        print(f"✅ {args.action.title()} completed successfully!")
    else:
        print(f"❌ {args.action.title()} failed!")
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())