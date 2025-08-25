#!/usr/bin/env python3
"""
Dosatsu Examples Runner

This script provides an easy way to run and process the C++ examples included with Dosatsu.
It can compile examples, run Dosatsu indexing on them, and verify the results.
"""

import sys
import os
import argparse
import subprocess
import json
import tempfile
from pathlib import Path

def get_project_root():
    """Get the project root directory."""
    return Path(__file__).parent.parent.absolute()

def get_examples_root():
    """Get the examples root directory."""
    return Path(__file__).parent.absolute()

def fix_compilation_database_paths(compile_db_path):
    """Fix compilation database paths to be absolute and correct for the current project location."""
    project_root = get_project_root().absolute()
    
    with open(compile_db_path, 'r') as f:
        compile_db = json.load(f)
    
    # Fix paths in the compilation database
    # Now all compilation databases use "directory": "." consistently
    for entry in compile_db:
        # Fix directory path - always convert "." to absolute project root
        entry['directory'] = str(project_root)
        
        # Fix file path to be absolute
        file_path = entry.get('file', '')
        if not Path(file_path).is_absolute():
            entry['file'] = str(project_root / file_path)
        
        # Fix command to use absolute paths
        command = entry.get('command', '')
        if 'Examples/' in command:
            # Replace relative paths in command with absolute paths
            # Use forward slashes for consistency with clang
            examples_abs_path = str(project_root / 'Examples').replace('\\', '/')
            command = command.replace('Examples/', examples_abs_path + '/')
            entry['command'] = command
    
    # Create a temporary file with the fixed compilation database
    temp_fd, temp_path = tempfile.mkstemp(suffix='.json', prefix='compile_commands_')
    try:
        with os.fdopen(temp_fd, 'w') as f:
            json.dump(compile_db, f, indent=2)
        return temp_path
    except:
        os.unlink(temp_path)
        raise

def list_available_examples():
    """List all available C++ examples."""
    examples_root = get_examples_root()
    
    print("=== Available C++ Examples ===\n")
    
    # Basic examples
    basic_dir = examples_root / "cpp" / "basic"
    if basic_dir.exists():
        print("Basic Examples (cpp/basic/):")
        for cpp_file in sorted(basic_dir.glob("*.cpp")):
            print(f"   - {cpp_file.stem}.cpp")
        print()
    
    # Comprehensive examples  
    comprehensive_dir = examples_root / "cpp" / "comprehensive"
    if comprehensive_dir.exists():
        print("Comprehensive Examples (cpp/comprehensive/):")
        for cpp_file in sorted(comprehensive_dir.glob("*.cpp")):
            print(f"   - {cpp_file.stem}.cpp")
        print()
    
    # Compilation configurations
    compilation_dir = examples_root / "cpp" / "compilation"
    if compilation_dir.exists():
        print("Compilation Configurations (cpp/compilation/):")
        for json_file in sorted(compilation_dir.glob("*.json")):
            print(f"   - {json_file.stem}.json")
        print()

def has_main_function(file_path):
    """Check if a C++ file has a main function."""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            # Look for main function patterns
            import re
            # Match various main function signatures
            main_patterns = [
                r'\bint\s+main\s*\(',
                r'\bauto\s+main\s*\(',
                r'\bvoid\s+main\s*\('
            ]
            for pattern in main_patterns:
                if re.search(pattern, content):
                    return True
            return False
    except Exception:
        return False

def compile_example(example_path, output_dir=None):
    """Compile a single C++ example."""
    project_root = get_project_root()
    
    if output_dir is None:
        output_dir = project_root / "artifacts" / "examples"
    
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    example_path = Path(example_path)
    if not example_path.is_absolute():
        example_path = get_examples_root() / "cpp" / example_path
    
    if not example_path.exists():
        print(f"[ERROR] Example not found: {example_path}")
        return False
    
    output_name = example_path.stem
    
    # Check if file has main function
    has_main = has_main_function(example_path)
    
    if has_main:
        # Compile as executable
        output_path = output_dir / f"{output_name}.exe"
        cmd = [
            "clang++",
            "-std=c++17",
            "-I.", 
            "-DEXAMPLE_MODE",
            str(example_path),
            "-o", str(output_path)
        ]
        compile_type = "executable"
    else:
        # Compile as object file for syntax checking
        output_path = output_dir / f"{output_name}.o"
        cmd = [
            "clang++",
            "-std=c++17",
            "-I.", 
            "-DEXAMPLE_MODE",
            "-c",  # Compile only, don't link
            str(example_path),
            "-o", str(output_path)
        ]
        compile_type = "object file"
    
    print(f"Compiling {example_path.name} as {compile_type}...")
    print(f"   Command: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(cmd, cwd=project_root, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"[SUCCESS] Compiled to: {output_path}")
            return True
        else:
            print(f"[ERROR] Compilation failed:")
            print(f"   Error: {result.stderr}")
            return False
    except FileNotFoundError:
        print("[ERROR] clang++ not found. Please ensure it's installed and in your PATH.")
        return False
    except Exception as e:
        print(f"[ERROR] Compilation error: {e}")
        return False

def run_dosatsu_indexing(compile_commands_path, output_db_path=None):
    """Run Dosatsu indexing on examples."""
    project_root = get_project_root()
    
    if output_db_path is None:
        import time
        timestamp = int(time.time())
        output_db_path = project_root / "artifacts" / "examples" / f"example_database_{timestamp}"
    
    dosatsu_path = project_root / "artifacts" / "debug" / "bin" / "dosatsu_cpp.exe"
    
    if not dosatsu_path.exists():
        print("[ERROR] Dosatsu not found. Please run 'python please.py build' first.")
        return False
    
    compile_commands_path = Path(compile_commands_path)
    if not compile_commands_path.is_absolute():
        compile_commands_path = get_examples_root() / "cpp" / "compilation" / compile_commands_path
    
    if not compile_commands_path.exists():
        print(f"[ERROR] Compilation database not found: {compile_commands_path}")
        return False
    
    # Fix the compilation database paths to be absolute and correct
    try:
        fixed_compile_db_path = fix_compilation_database_paths(compile_commands_path)
    except Exception as e:
        print(f"[ERROR] Failed to fix compilation database paths: {e}")
        return False
    
    # Run Dosatsu
    cmd = [
        str(dosatsu_path),
        fixed_compile_db_path,
        "--output-db", str(output_db_path)
    ]
    
    print(f"Running Dosatsu indexing...")
    print(f"   Input: {compile_commands_path.name}")
    print(f"   Output: {output_db_path}")
    
    try:
        result = subprocess.run(cmd, cwd=project_root, capture_output=True, text=True)
        if result.returncode == 0:
            print("[SUCCESS] Indexing completed successfully!")
            print(f"   Database created at: {output_db_path}")
            return True
        else:
            print(f"[ERROR] Indexing failed:")
            print(f"   Error: {result.stderr}")
            return False
    except Exception as e:
        print(f"[ERROR] Indexing error: {e}")
        return False
    finally:
        # Clean up the temporary file
        try:
            if 'fixed_compile_db_path' in locals():
                os.unlink(fixed_compile_db_path)
        except:
            pass

def run_verification():
    """Run the verification query suite."""
    examples_root = get_examples_root()
    verification_script = examples_root / "queries" / "run_queries.py"
    
    if not verification_script.exists():
        print("[ERROR] Verification query script not found.")
        return False
    
    print("Running verification query suite...")
    
    try:
        result = subprocess.run([sys.executable, str(verification_script)], 
                              cwd=get_project_root(), 
                              capture_output=False)
        return result.returncode == 0
    except Exception as e:
        print(f"[ERROR] Verification error: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(
        description="Dosatsu Examples Runner",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python run_examples.py --list                    # List all available examples
  python run_examples.py --compile basic/inheritance.cpp  # Compile single example
  python run_examples.py --index comprehensive_compile_commands.json  # Index examples
  python run_examples.py --verify                  # Run verification suite
  python run_examples.py --all                     # Run complete workflow
        """
    )
    
    parser.add_argument("--list", action="store_true",
                       help="List all available examples")
    parser.add_argument("--compile", metavar="EXAMPLE",
                       help="Compile a specific C++ example")
    parser.add_argument("--index", metavar="COMPILE_DB",
                       help="Run Dosatsu indexing on examples")
    parser.add_argument("--verify", action="store_true",
                       help="Run verification query suite")
    parser.add_argument("--all", action="store_true",
                       help="Run complete workflow: index + verify")
    parser.add_argument("--output-dir", metavar="DIR",
                       help="Output directory for compiled examples")
    parser.add_argument("--db-output", metavar="PATH",
                       help="Output path for index database")
    
    args = parser.parse_args()
    
    if not any([args.list, args.compile, args.index, args.verify, args.all]):
        parser.print_help()
        return 1
    
    print("Dosatsu Examples Runner")
    print("=" * 50)
    
    success = True
    
    if args.list:
        list_available_examples()
    
    if args.compile:
        success &= compile_example(args.compile, args.output_dir)
    
    if args.index:
        success &= run_dosatsu_indexing(args.index, args.db_output)
    
    if args.verify:
        success &= run_verification()
    
    if args.all:
        print("\nRunning complete examples workflow...\n")
        
        # Step 1: Index examples
        print("Step 1: Indexing examples...")
        success &= run_dosatsu_indexing("comprehensive_no_std_compile_commands.json")
        
        if success:
            print("\nStep 2: Running verification queries...")
            success &= run_verification()
    
    print("\n" + "=" * 50)
    if success:
        print("[SUCCESS] All operations completed successfully!")
    else:
        print("[ERROR] Some operations failed. Check the output above.")
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
