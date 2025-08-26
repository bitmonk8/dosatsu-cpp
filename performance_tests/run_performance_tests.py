#!/usr/bin/env python3
"""
Performance testing script for Dosatsu indexing performance comparison.

This script compares indexing performance between:
1. Standard library examples (std_library_performance_test.cpp)
2. Library-free examples (from Examples/cpp/)

Usage:
    python run_performance_tests.py [--examples-dir PATH] [--dosatsu-path PATH]
"""

import os
import sys
import time
import subprocess
import argparse
from pathlib import Path
import tempfile

def get_project_root():
    """Get the project root directory."""
    current_dir = Path(__file__).parent
    return current_dir.parent

def build_performance_test():
    """Build the performance test executable."""
    project_root = get_project_root()
    perf_test_dir = project_root / "performance_tests"
    build_dir = project_root / "artifacts" / "performance_tests"
    
    # Create build directory
    build_dir.mkdir(parents=True, exist_ok=True)
    
    # Configure CMake
    configure_cmd = [
        "cmake",
        "-S", str(perf_test_dir),
        "-B", str(build_dir),
        "-DCMAKE_BUILD_TYPE=Release"
    ]
    
    print(f"Configuring performance test: {' '.join(configure_cmd)}")
    subprocess.run(configure_cmd, check=True, cwd=project_root)
    
    # Build
    build_cmd = ["cmake", "--build", str(build_dir), "--config", "Release"]
    print(f"Building performance test: {' '.join(build_cmd)}")
    subprocess.run(build_cmd, check=True, cwd=project_root)
    
    return build_dir

def create_compile_commands(source_file, output_file):
    """Create a compile_commands.json for a single source file."""
    project_root = get_project_root()
    
    compile_commands = [
        {
            "directory": str(project_root),
            "command": f"clang++ -std=c++17 -I. {source_file} -o temp_executable",
            "file": str(source_file)
        }
    ]
    
    import json
    with open(output_file, 'w') as f:
        json.dump(compile_commands, f, indent=2)
    
    return output_file

def time_dosatsu_indexing(compile_commands_path, description):
    """Time Dosatsu indexing of a compile_commands.json file."""
    project_root = get_project_root()
    dosatsu_path = project_root / "artifacts" / "debug" / "bin" / "dosatsu_cpp.exe"
    
    if not dosatsu_path.exists():
        print(f"Error: Dosatsu executable not found at {dosatsu_path}")
        print("Please run 'python please.py build' first")
        return None
    
    # Create temporary database file
    with tempfile.NamedTemporaryFile(suffix=".db", delete=False) as temp_db:
        temp_db_path = temp_db.name
    
    try:
        # Run Dosatsu indexing
        cmd = [
            str(dosatsu_path),
            "--output-db", temp_db_path,
            str(compile_commands_path)
        ]
        
        print(f"\nTiming {description}...")
        print(f"Command: {' '.join(cmd)}")
        
        start_time = time.time()
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=project_root)
        end_time = time.time()
        
        elapsed_time = end_time - start_time
        
        if result.returncode == 0:
            print(f"SUCCESS: {description} completed successfully")
            print(f"  Time: {elapsed_time:.3f} seconds")
            return elapsed_time
        else:
            print(f"FAILED: {description} failed")
            print(f"  stdout: {result.stdout}")
            print(f"  stderr: {result.stderr}")
            return None
            
    finally:
        # Clean up temporary database
        try:
            os.unlink(temp_db_path)
        except OSError:
            pass

def run_performance_comparison():
    """Run performance comparison between std library and library-free examples."""
    project_root = get_project_root()
    
    print("=== Dosatsu Performance Test ===")
    print("Comparing indexing performance: Standard Library vs Library-Free Examples")
    print()
    
    # Build performance test
    try:
        build_dir = build_performance_test()
        print("SUCCESS: Performance test built successfully")
    except subprocess.CalledProcessError as e:
        print(f"FAILED: Failed to build performance test: {e}")
        return 1
    
    # Test 1: Standard library performance test
    perf_test_cpp = project_root / "performance_tests" / "std_library_performance_test.cpp"
    with tempfile.NamedTemporaryFile(mode='w', suffix='_std_compile_commands.json', delete=False) as temp_cc:
        std_cc_path = temp_cc.name
        create_compile_commands(perf_test_cpp, std_cc_path)
    
    std_time = time_dosatsu_indexing(std_cc_path, "Standard Library Example (with <vector>)")
    
    # Test 2: Library-free example
    examples_dir = project_root / "Examples" / "cpp"
    library_free_example = examples_dir / "simple_no_includes" / "simple_no_includes.cpp"
    
    if not library_free_example.exists():
        # Try other library-free examples
        library_free_example = examples_dir / "clean_example" / "clean_example.cpp"
    
    if library_free_example.exists():
        with tempfile.NamedTemporaryFile(mode='w', suffix='_lib_free_compile_commands.json', delete=False) as temp_cc:
            lib_free_cc_path = temp_cc.name
            create_compile_commands(library_free_example, lib_free_cc_path)
        
        lib_free_time = time_dosatsu_indexing(lib_free_cc_path, "Library-Free Example")
    else:
        print("FAILED: Could not find library-free example to test")
        lib_free_time = None
    
    # Cleanup
    try:
        os.unlink(std_cc_path)
        if 'lib_free_cc_path' in locals():
            os.unlink(lib_free_cc_path)
    except OSError:
        pass
    
    # Results
    print("\n=== Performance Comparison Results ===")
    if std_time is not None:
        print(f"Standard Library Example:  {std_time:.3f} seconds")
    else:
        print("Standard Library Example:  FAILED")
    
    if lib_free_time is not None:
        print(f"Library-Free Example:      {lib_free_time:.3f} seconds")
    else:
        print("Library-Free Example:      FAILED or MISSING")
    
    if std_time is not None and lib_free_time is not None:
        if std_time > lib_free_time:
            speedup = std_time / lib_free_time
            print(f"\nSUCCESS: Library-free example is {speedup:.1f}x faster!")
        else:
            slowdown = lib_free_time / std_time
            print(f"\nWARNING: Standard library example is {slowdown:.1f}x faster (unexpected)")
    
    return 0

def main():
    """Main function."""
    parser = argparse.ArgumentParser(description="Run Dosatsu performance tests")
    parser.add_argument("--examples-dir", type=Path, 
                       help="Path to Examples directory (default: auto-detect)")
    parser.add_argument("--dosatsu-path", type=Path,
                       help="Path to Dosatsu executable (default: auto-detect)")
    
    args = parser.parse_args()
    
    try:
        return run_performance_comparison()
    except KeyboardInterrupt:
        print("\n\nPerformance test interrupted by user")
        return 1
    except Exception as e:
        print(f"\nError running performance test: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())
