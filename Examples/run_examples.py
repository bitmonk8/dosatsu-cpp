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
import shutil
import time
from pathlib import Path
from datetime import datetime

def get_project_root():
    """Get the project root directory."""
    return Path(__file__).parent.parent.absolute()

def get_examples_root():
    """Get the examples root directory."""
    return Path(__file__).parent.absolute()

def get_compilation_database_path(compile_db_name):
    """Get compilation database path, generating CMake version for all databases."""
    project_root = get_project_root()
    
    # Map ALL compilation databases to their CMake-generated equivalents
    cmake_database_mapping = {
        # Standard library databases (auto-generate CMake versions)
        'comprehensive_compile_commands.json': 'comprehensive',
        'comprehensive_advanced_compile_commands.json': 'comprehensive',
        
        # Legacy database names - redirect to existing CMake categories
        'simple_compile_commands.json': 'basic',          # Use basic category for simple examples
        'single_file_compile_commands.json': 'basic',     # Use basic category for single files
        'multi_file_compile_commands.json': 'basic',      # Use basic category for multi-file
        'schema_coverage_compile_commands.json': 'comprehensive',  # Use comprehensive for schema
        'two_file_compile_commands.json': 'basic',        # Use basic category for two-file examples
        
        # No-std databases - now using CMake generation for unified approach!
        'comprehensive_no_std_compile_commands.json': 'nostd',
        'advanced_no_std_compile_commands.json': 'nostd',
        'two_file_no_std_compile_commands.json': 'nostd',
    }
    
    if compile_db_name in cmake_database_mapping:
        category = cmake_database_mapping[compile_db_name]
        if category is not None:
            # Generate CMake database
            cmake_db_path = ensure_cmake_compilation_database(category)
            return cmake_db_path
    
    # For databases not in mapping, this is likely an error - CMake should handle all cases now
    # Log a warning but still try to use artifacts location for consistency
    print(f"Warning: Database '{compile_db_name}' not found in CMake mapping, trying artifacts location")
    return project_root / "artifacts" / "examples" / compile_db_name

def ensure_cmake_compilation_database(example_category):
    """Ensure CMake compilation database exists, creating if necessary."""
    project_root = get_project_root()
    cmake_db_path = project_root / "artifacts" / "examples" / f"{example_category}_cmake_compile_commands.json"
    
    # Check if database exists and is recent
    if cmake_db_path.exists() and is_cmake_db_current(example_category):
        return cmake_db_path
    
    # Generate new CMake compilation database
    print(f"Generating CMake compilation database for {example_category} examples...")
    return generate_cmake_compilation_database(example_category)

def is_cmake_db_current(example_category):
    """Check if CMake compilation database is current (not stale)."""
    project_root = get_project_root()
    cmake_db_path = project_root / "artifacts" / "examples" / f"{example_category}_cmake_compile_commands.json"
    
    if not cmake_db_path.exists():
        return False
    
    # Check if source files are newer than the database
    # For simple and schema categories, check multiple source directories
    db_mtime = cmake_db_path.stat().st_mtime
    
    if example_category == "simple":
        cpp_dir = project_root / "Examples" / "cpp" / "basic"
        if cpp_dir.exists():
            for file_name in ["simple_no_includes.cpp", "simple2.cpp"]:
                cpp_file = cpp_dir / file_name
                if cpp_file.exists() and cpp_file.stat().st_mtime > db_mtime:
                    return False
    elif example_category == "schema":
        cpp_dir = project_root / "Examples" / "cpp" / "comprehensive"
        if cpp_dir.exists():
            for file_name in ["schema_coverage_complete.cpp"]:
                cpp_file = cpp_dir / file_name
                if cpp_file.exists() and cpp_file.stat().st_mtime > db_mtime:
                    return False
    elif example_category == "nostd":
        # Check no-std files from both basic and comprehensive directories
        basic_dir = project_root / "Examples" / "cpp" / "basic"
        comprehensive_dir = project_root / "Examples" / "cpp" / "comprehensive"
        
        # Check basic directory files
        if basic_dir.exists():
            for file_name in ["simple_no_includes.cpp", "inheritance_no_std.cpp"]:
                cpp_file = basic_dir / file_name
                if cpp_file.exists() and cpp_file.stat().st_mtime > db_mtime:
                    return False
        
        # Check comprehensive directory files
        if comprehensive_dir.exists():
            for file_name in ["no_std_example.cpp", "advanced_features_no_std.cpp"]:
                cpp_file = comprehensive_dir / file_name
                if cpp_file.exists() and cpp_file.stat().st_mtime > db_mtime:
                    return False
    elif example_category in ["basic", "comprehensive"]:
        # For basic and comprehensive categories, check the entire directory
        cpp_dir = project_root / "Examples" / "cpp" / example_category
        if cpp_dir.exists():
            for cpp_file in cpp_dir.glob("*.cpp"):
                if cpp_file.stat().st_mtime > db_mtime:
                    return False
    
    # Also check if CMake project files are newer (since we copy files)
    cmake_project_dir = project_root / "Examples" / "cmake_projects" / f"{example_category}_examples"
    cmake_file = cmake_project_dir / "CMakeLists.txt"
    if cmake_file.exists() and cmake_file.stat().st_mtime > db_mtime:
        return False
    
    return True

def create_cmake_project(example_category):
    """Ensure CMake project directory exists - now using consolidated structure."""
    project_root = get_project_root()
    cmake_project_dir = project_root / "Examples" / "cpp" / example_category
    
    # Verify the directory exists (should already exist from consolidation)
    if not cmake_project_dir.exists():
        raise RuntimeError(f"Example category directory not found: {cmake_project_dir}")
    
    # Verify CMakeLists.txt exists
    cmake_file = cmake_project_dir / "CMakeLists.txt"
    if not cmake_file.exists():
        raise RuntimeError(f"CMakeLists.txt not found in: {cmake_project_dir}")
    
    print(f"   Using consolidated CMake project: {cmake_project_dir}")

def generate_cmake_compilation_database(example_category):
    """Generate compilation database using CMake for given example category."""
    project_root = get_project_root()
    cmake_project_dir = project_root / "Examples" / "cpp" / example_category
    build_dir = project_root / "artifacts" / "examples" / example_category
    build_dir.mkdir(parents=True, exist_ok=True)
    
    # Ensure CMake project exists
    create_cmake_project(example_category)
    
    # Configure CMake project using Ninja generator
    configure_cmd = [
        "cmake",
        "-S", str(cmake_project_dir),
        "-B", str(build_dir),
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    ]
    
    print(f"   Configuring CMake project: {' '.join(configure_cmd)}")
    result = subprocess.run(configure_cmd, check=True, cwd=project_root, capture_output=True, text=True)
    
    # Copy generated compile_commands.json
    src_db = build_dir / "compile_commands.json"
    dst_db = project_root / "artifacts" / "examples" / f"{example_category}_cmake_compile_commands.json"
    dst_db.parent.mkdir(parents=True, exist_ok=True)  # Ensure directory exists
    
    if not src_db.exists():
        raise RuntimeError(f"CMake did not generate compile_commands.json at {src_db}")
    
    shutil.copy2(src_db, dst_db)
    print(f"   Generated: {dst_db.name}")
    
    return dst_db

def generate_cmake_content(example_category):
    """Generate CMakeLists.txt content for example category."""
    if example_category == "basic":
        example_files = [
            "inheritance.cpp",
            "templates.cpp", 
            "namespaces.cpp",
            "control_flow.cpp",
            "expressions.cpp",
            "preprocessor.cpp"
        ]
        advanced_definitions = ""
    elif example_category == "comprehensive":
        example_files = [
            "advanced_features.cpp",
            "complete_example.cpp",
            "modern_cpp_features.cpp",
            "control_flow_complex.cpp",
            "standard_example.cpp",
            "clean_example.cpp"
        ]
        advanced_definitions = """
# Add advanced definitions for specific files
if(TARGET advanced_features)
    target_compile_definitions(advanced_features PRIVATE 
        DEBUG 
        ENABLE_OPTIMIZATION=1 
        ADVANCED_MODE 
        FEATURE_A 
        FEATURE_B 
        FEATURE_A_VERSION=2 
        FEATURE_B_VERSION=3 
        ENABLE_LOGGING 
        VERBOSE_LOGGING
    )
endif()

if(TARGET preprocessor_advanced)
    target_compile_definitions(preprocessor_advanced PRIVATE 
        DEBUG 
        ENABLE_OPTIMIZATION=1 
        ADVANCED_MODE
    )
endif()"""
    elif example_category == "simple":
        example_files = [
            "simple_no_includes.cpp",
            "simple2.cpp"
        ]
        advanced_definitions = ""
    elif example_category == "schema":
        example_files = [
            "schema_coverage_complete.cpp"
        ]
        advanced_definitions = ""
    elif example_category == "nostd":
        example_files = [
            "no_std_example.cpp",
            "advanced_features_no_std.cpp",
            "simple_no_includes.cpp",
            "inheritance_no_std.cpp"
        ]
        advanced_definitions = """
# Add advanced definitions for specific files
if(TARGET advanced_features_no_std)
    target_compile_definitions(advanced_features_no_std PRIVATE 
        DEBUG 
        ENABLE_OPTIMIZATION=1 
        ADVANCED_MODE
    )
endif()"""
    else:
        # Fallback - no files defined for this category
        example_files = []
        advanced_definitions = ""
    
    files_list = '\n    '.join(example_files)
    
    return f"""cmake_minimum_required(VERSION 3.24)
project(Dosatsu{example_category.title()}Examples 
    VERSION 1.0.0
    DESCRIPTION "Dosatsu {example_category.title()} C++ Examples"
    LANGUAGES CXX
)

# Match main project configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Windows MSVC settings (aligned with main CMakeLists.txt)
if(WIN32 AND MSVC)
    add_compile_options(/W4 /EHsc)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
endif()

# Example files
set(EXAMPLE_FILES
    {files_list}
)

# Create targets for each source file
foreach(SOURCE ${{EXAMPLE_FILES}})
    get_filename_component(TARGET_NAME ${{SOURCE}} NAME_WE)
    add_executable(${{TARGET_NAME}} "src/${{SOURCE}}")
    set_target_properties(${{TARGET_NAME}} PROPERTIES
        EXCLUDE_FROM_ALL TRUE
    )
    # Add example mode definition
    target_compile_definitions(${{TARGET_NAME}} PRIVATE EXAMPLE_MODE)
endforeach(){advanced_definitions}

# Special target to build all examples
add_custom_target(build_all_{example_category}_examples
    DEPENDS ${{EXAMPLE_FILES}}
)
"""

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

def create_log_file_path():
    """Create a timestamped log file path for dosatsu output."""
    project_root = get_project_root()
    log_dir = project_root / "artifacts" / "examples" / "logs"
    log_dir.mkdir(parents=True, exist_ok=True)
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_file = log_dir / f"dosatsu_output_{timestamp}.log"
    return log_file

def handle_dosatsu_output(stdout_output, stderr_output, log_file):
    """Handle dosatsu output - show on console if short, otherwise redirect to file."""
    # Combine stdout and stderr for analysis
    full_output = ""
    if stdout_output:
        full_output += "=== STDOUT ===\n" + stdout_output + "\n"
    if stderr_output:
        full_output += "=== STDERR ===\n" + stderr_output + "\n"
    
    # Count lines in output
    output_lines = full_output.count('\n') if full_output else 0
    
    # Write full output to log file always
    try:
        with open(log_file, 'w', encoding='utf-8') as f:
            f.write(f"Dosatsu execution log - {datetime.now().isoformat()}\n")
            f.write("=" * 60 + "\n\n")
            f.write(full_output)
        log_written = True
    except Exception as e:
        print(f"[WARNING] Failed to write log file {log_file}: {e}")
        log_written = False
    
    # Determine how to present output
    if output_lines <= 20 and output_lines > 0:
        # Short output - show on console
        if stdout_output:
            print("   dosatsu-cpp output:")
            for line in stdout_output.strip().split('\n'):
                print(f"     {line}")
        if stderr_output:
            print("   dosatsu-cpp errors:")
            for line in stderr_output.strip().split('\n'):
                print(f"     {line}")
        if log_written:
            print(f"   Full output also saved to: {log_file}")
    else:
        # Long output or no output - just refer to log file
        if output_lines > 20:
            print(f"   dosatsu-cpp output ({output_lines} lines) saved to: {log_file}")
        elif log_written:
            print(f"   dosatsu-cpp execution log saved to: {log_file}")
    
    return log_written

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
    
    # Use the new path resolution function that handles CMake generation
    if isinstance(compile_commands_path, str):
        compile_commands_path = get_compilation_database_path(compile_commands_path)
    else:
        compile_commands_path = Path(compile_commands_path)
    
    if not compile_commands_path.exists():
        print(f"[ERROR] Compilation database not found: {compile_commands_path}")
        return False
    
    # Fix the compilation database paths to be absolute and correct
    try:
        fixed_compile_db_path = fix_compilation_database_paths(compile_commands_path)
    except Exception as e:
        print(f"[ERROR] Failed to fix compilation database paths: {e}")
        return False
    
    # Create log file for output
    log_file = create_log_file_path()
    
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
        
        # Handle output redirection
        handle_dosatsu_output(result.stdout, result.stderr, log_file)
        
        if result.returncode == 0:
            print("[SUCCESS] Indexing completed successfully!")
            print(f"   Database created at: {output_db_path}")
            return True
        else:
            print(f"[ERROR] Indexing failed (return code: {result.returncode})")
            if not result.stderr and not result.stdout:
                print(f"   No output captured. Check log file: {log_file}")
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
  python run_examples.py --index comprehensive_compile_commands.json  # Index examples (auto-generates CMake DB)
  python run_examples.py --verify                  # Run verification suite
  python run_examples.py --all                     # Run complete workflow
  python run_examples.py --generate-cmake comprehensive   # Generate CMake DB for comprehensive examples
  python run_examples.py --force-regenerate-cmake  # Force regenerate all CMake databases
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
    parser.add_argument("--force-regenerate-cmake", action="store_true",
                       help="Force regenerate all CMake compilation databases")
    parser.add_argument("--generate-cmake", metavar="CATEGORY",
                       help="Generate CMake compilation database for specific category (basic, comprehensive)")
    
    args = parser.parse_args()
    
    if not any([args.list, args.compile, args.index, args.verify, args.all, args.force_regenerate_cmake, args.generate_cmake]):
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
    
    if args.force_regenerate_cmake:
        print("Force regenerating all CMake compilation databases...")
        for category in ['basic', 'comprehensive', 'nostd']:
            # Remove existing cmake database to force regeneration
            cmake_db_path = get_project_root() / "artifacts" / "examples" / f"{category}_cmake_compile_commands.json"
            if cmake_db_path.exists():
                cmake_db_path.unlink()
                print(f"   Removed existing database: {cmake_db_path.name}")
            
            # Generate new database
            try:
                new_db_path = generate_cmake_compilation_database(category)
                print(f"   Successfully generated: {new_db_path.name}")
            except Exception as e:
                print(f"   [ERROR] Failed to generate {category} database: {e}")
                success = False
    
    if args.generate_cmake:
        category = args.generate_cmake
        print(f"Generating CMake compilation database for {category} examples...")
        try:
            new_db_path = generate_cmake_compilation_database(category)
            print(f"   Successfully generated: {new_db_path}")
        except Exception as e:
            print(f"   [ERROR] Failed to generate {category} database: {e}")
            success = False
    
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
