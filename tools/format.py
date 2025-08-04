#!/usr/bin/env python3
"""
Code formatting script for CppGraphIndex project.
Uses clang-format to format all .h and .cpp files in the MakeIndex directory.
Matches the behavior of the xmake format target.
"""

import os
import sys
import subprocess
import glob
from pathlib import Path

def find_source_files():
    """Find all .h and .cpp files in the MakeIndex directory."""
    source_dirs = ["MakeIndex"]
    file_patterns = ["*.h", "*.cpp"]
    files = []
    
    for source_dir in source_dirs:
        if os.path.isdir(source_dir):
            for pattern in file_patterns:
                pattern_path = os.path.join(source_dir, pattern)
                found_files = glob.glob(pattern_path)
                files.extend(found_files)
    
    return files

def format_files(files):
    """Format the given files using clang-format."""
    if not files:
        print("No source files found to format")
        return True
    
    print(f"Formatting {len(files)} source files...")
    
    # Build clang-format command with all files
    cmd = ["clang-format", "-i"] + files
    
    try:
        # Execute the formatting command
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        print("Code formatting completed successfully!")
        print("Formatted files in directories: MakeIndex")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running clang-format: {e}")
        print("Make sure clang-format is installed and available in PATH")
        if e.stderr:
            print(f"clang-format stderr: {e.stderr}")
        return False
    except FileNotFoundError:
        print("Error: clang-format not found in PATH")
        print("Make sure clang-format is installed and available in PATH")
        return False

def main():
    """Main function to orchestrate the formatting process."""
    # Change to the script's directory's parent (project root)
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    os.chdir(project_root)
    
    print("CppGraphIndex Code Formatter")
    print("============================")
    
    # Find source files
    source_files = find_source_files()
    
    if not source_files:
        print("No source files found to format")
        return 0
    
    # Format the files
    success = format_files(source_files)
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())