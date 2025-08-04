#!/usr/bin/env python3
"""
Lint script for CppGraphIndex using clang-tidy.
Provides equivalent functionality to xmake run lint.
"""

import os
import sys
import json
import tempfile
import subprocess
import argparse
from pathlib import Path

def find_source_files(source_dirs, file_patterns):
    """Find all source files in specified directories matching patterns."""
    files = []
    for directory in source_dirs:
        if os.path.isdir(directory):
            for pattern in file_patterns:
                # Convert glob pattern to pathlib Pattern
                path_obj = Path(directory)
                found_files = list(path_obj.glob(pattern))
                files.extend([str(f) for f in found_files])
    return files

def sanitize_compile_commands(compile_db_path):
    """
    Sanitize compile_commands.json by removing unwanted flags and adding necessary ones.
    Matches the xmake lint behavior.
    """
    if not os.path.exists(compile_db_path):
        print(f"Error: compile_commands.json not found at {compile_db_path}")
        return False
    
    try:
        with open(compile_db_path, 'r') as f:
            content = f.read()
        
        # Remove unwanted flags (matching xmake behavior)
        content = content.replace(' -Wno-unused-command-line-argument', '')
        content = content.replace(' -Werror', '')
        
        # Add warning suppression flags for LLVM compatibility
        # Parse JSON to modify properly (handle both "command" and "arguments" formats)
        compile_commands = json.loads(content)
        
        for entry in compile_commands:
            # Handle both formats: arguments array or command string
            if 'arguments' in entry:
                # Arguments array format
                args = entry['arguments']
                # Check if flags aren't already present
                has_deprecated_capture = any('-Wno-deprecated-this-capture' in arg for arg in args)
                has_deprecated_anon_enum = any('-Wno-deprecated-anon-enum-enum-conversion' in arg for arg in args)
                has_language_extensions = any('-Wno-language-extension-token' in arg for arg in args)
                has_ms_extensions = any('-fms-extensions' in arg for arg in args)
                
                if not has_deprecated_capture:
                    args.append('-Wno-deprecated-this-capture')
                if not has_deprecated_anon_enum:
                    args.append('-Wno-deprecated-anon-enum-enum-conversion')
                if not has_language_extensions:
                    args.append('-Wno-language-extension-token')
                if not has_ms_extensions:
                    args.append('-fms-extensions')
                    
            elif 'command' in entry:
                # Command string format
                cmd = entry['command']
                # Remove -Werror first
                cmd = cmd.replace(' -Werror', '')
                
                # Add flags if not present
                if '-Wno-deprecated-this-capture' not in cmd:
                    cmd += ' -Wno-deprecated-this-capture'
                if '-Wno-deprecated-anon-enum-enum-conversion' not in cmd:
                    cmd += ' -Wno-deprecated-anon-enum-enum-conversion'
                if '-Wno-language-extension-token' not in cmd:
                    cmd += ' -Wno-language-extension-token'
                if '-fms-extensions' not in cmd:
                    cmd += ' -fms-extensions'
                    
                entry['command'] = cmd
        
        # Write back the modified content
        with open(compile_db_path, 'w') as f:
            json.dump(compile_commands, f, indent=2)
        
        print("Temporarily sanitized compile_commands.json for linting.")
        return True
        
    except Exception as e:
        print(f"Error sanitizing compile_commands.json: {e}")
        return False

def run_clang_tidy_fix(compile_db_path, files):
    """Run clang-tidy with --fix to automatically fix issues (silent)."""
    fix_args = [
        'clang-tidy',
        '-p', os.path.dirname(compile_db_path),
        '--fix',
        '--fix-errors',
        '--quiet'
    ] + files
    
    try:
        # Run silently (matching xmake behavior)
        result = subprocess.run(fix_args, 
                              stdout=subprocess.DEVNULL, 
                              stderr=subprocess.DEVNULL,
                              check=False)
        return result.returncode == 0
    except FileNotFoundError:
        print("Error: clang-tidy not found. Make sure it's installed and in PATH.")
        return False

def run_clang_tidy_check(compile_db_path, files):
    """Run clang-tidy to check and display remaining issues."""
    check_args = [
        'clang-tidy',
        '-p', os.path.dirname(compile_db_path)
    ] + files
    
    try:
        print(' '.join(check_args))  # Display command being run
        
        result = subprocess.run(check_args, 
                              capture_output=True, 
                              text=True,
                              check=False)
        
        return result.returncode, result.stdout + result.stderr
        
    except FileNotFoundError:
        print("Error: clang-tidy not found. Make sure it's installed and in PATH.")
        return 1, ""

def filter_clang_tidy_output(output):
    """Filter clang-tidy output to remove noise (matching xmake behavior)."""
    filtered_lines = []
    
    for line in output.split('\n'):
        # Skip suppression messages and header filter suggestions
        if 'Suppressed' in line and 'warnings' in line:
            continue
        if 'Use -header-filter=' in line and 'to display errors from all non-system headers' in line:
            continue
        filtered_lines.append(line)
    
    return '\n'.join(filtered_lines)

def main():
    parser = argparse.ArgumentParser(description='Lint C++ source files using clang-tidy')
    parser.add_argument('file', nargs='?', help='Specific file to lint (optional)')
    parser.add_argument('--build-dir', default='builddir', help='Build directory containing compile_commands.json')
    
    args = parser.parse_args()
    
    # Determine files to lint
    if args.file:
        if os.path.isfile(args.file):
            files = [args.file]
            print(f"Running clang-tidy on {args.file}...")
        else:
            print(f"Error: File not found at: {args.file}")
            return 1
    else:
        print("Running clang-tidy on all project files...")
        source_dirs = ["MakeIndex"]
        file_patterns = ["*.cpp"]
        files = find_source_files(source_dirs, file_patterns)
        
        if not files:
            # Try to find the source directory from build directory
            potential_source_dir = os.path.join("..", "MakeIndex")
            if os.path.isdir(potential_source_dir):
                files = find_source_files([potential_source_dir], file_patterns)
                
            if not files:
                print("No source files found to lint")
                return 0
    
    # Path to compile_commands.json
    compile_db_path = os.path.join(args.build_dir, 'compile_commands.json')
    
    # Check if compile_commands.json exists
    if not os.path.exists(compile_db_path):
        print(f"Error: compile_commands.json not found at {compile_db_path}")
        print("Make sure to run 'meson setup builddir' first.")
        return 1
    
    # Sanitize compile_commands.json
    if not sanitize_compile_commands(compile_db_path):
        return 1
    
    # Run clang-tidy with --fix first (silent)
    run_clang_tidy_fix(compile_db_path, files)
    
    # Run clang-tidy to check and display issues
    return_code, output = run_clang_tidy_check(compile_db_path, files)
    
    # Filter and display output
    if output:
        filtered_output = filter_clang_tidy_output(output)
        if filtered_output.strip():
            print(filtered_output)
    
    # Print completion message
    if return_code == 0:
        print("\nCode linting completed successfully!")
    else:
        print("\nCode linting completed with errors.")
    
    return return_code

if __name__ == '__main__':
    sys.exit(main())