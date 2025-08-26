#!/usr/bin/env python3
"""
Database operations for Dosatsu verification queries
"""

import os
import subprocess
import tempfile
import shutil
import json
from pathlib import Path
from typing import Tuple
import kuzu


def create_temp_database(project_root: Path) -> str:
    """Create temporary database and return path"""
    temp_db_path = tempfile.mkdtemp(prefix="kuzu_test_")
    db_path = os.path.join(temp_db_path, "test_db")
    return db_path


def fix_compilation_database_paths(compile_db_path: Path, project_root: Path) -> str:
    """Fix compilation database paths to be absolute and correct for the current project location."""
    
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


def run_dosatsu(dosatsu_path: Path, compile_commands: Path, db_path: str) -> bool:
    """Run Dosatsu on compilation database"""
    if not dosatsu_path.exists():
        raise FileNotFoundError(f"Dosatsu not found at {dosatsu_path}. Run 'please build' first.")
    
    if not compile_commands.exists():
        raise FileNotFoundError(f"Compilation database not found at {compile_commands}")
    
    print(f"Running Dosatsu on {compile_commands}...")
    print(f"Output database: {db_path}")
    
    # Get project root (ensure it's absolute)
    project_root = dosatsu_path.parent.parent.parent.parent.absolute()
    
    # Fix the compilation database paths to be absolute and correct
    try:
        fixed_compile_db_path = fix_compilation_database_paths(compile_commands, project_root)
    except Exception as e:
        raise RuntimeError(f"Failed to fix compilation database paths: {e}")
    
    # Execute Dosatsu
    cmd = [
        str(dosatsu_path),
        fixed_compile_db_path,
        "--output-db", db_path
    ]
    
    try:
        result = subprocess.run(cmd, 
                              capture_output=True, 
                              text=True, 
                              cwd=str(project_root),
                              timeout=300)  # 5 minute timeout
        
        if result.returncode != 0:
            print(f"Dosatsu stdout: {result.stdout}")
            print(f"Dosatsu stderr: {result.stderr}")
            raise RuntimeError(f"Dosatsu failed with return code {result.returncode}")
        
        print("Dosatsu completed successfully")
        print(f"stdout: {result.stdout}")
        return True
        
    except subprocess.TimeoutExpired:
        raise RuntimeError("Dosatsu timed out after 5 minutes")
    finally:
        # Clean up the temporary file
        try:
            if 'fixed_compile_db_path' in locals():
                os.unlink(fixed_compile_db_path)
        except:
            pass


def connect_to_database(db_path: str) -> Tuple[kuzu.Database, kuzu.Connection]:
    """Connect to Kuzu database and return connection objects"""
    db = kuzu.Database(db_path)
    conn = kuzu.Connection(db)
    return db, conn


def cleanup_database(temp_path: str, db: kuzu.Database, conn: kuzu.Connection):
    """Clean up database resources"""
    if conn:
        conn.close()
    if db:
        db.close()
    
    # Extract temp directory from db path
    if temp_path and os.path.exists(temp_path):
        # If temp_path is the db path, get its parent directory
        if os.path.basename(temp_path) == "test_db":
            temp_dir = os.path.dirname(temp_path)
        else:
            temp_dir = temp_path
            
        if os.path.exists(temp_dir):
            shutil.rmtree(temp_dir)


def get_project_paths(project_root: Path = None) -> Tuple[Path, Path, Path]:
    """Get standard project paths (project_root, dosatsu_path, example_data_path)"""
    if project_root is None:
        # Auto-detect project root - go up from Examples/queries to project root
        current_dir = Path(__file__).parent.absolute()
        project_root = current_dir.parent.parent
    else:
        project_root = Path(project_root)
    
    dosatsu_path = project_root / "artifacts" / "debug" / "bin" / "dosatsu_cpp.exe"
    example_data_path = project_root / "Examples" / "cpp"
    
    return project_root, dosatsu_path, example_data_path


def setup_example_database(project_root: Path = None) -> Tuple[str, kuzu.Database, kuzu.Connection]:
    """Create a temporary database and run Dosatsu on the example files"""
    project_root, dosatsu_path, example_data_path = get_project_paths(project_root)
    
    # Create temporary database
    db_path = create_temp_database(project_root)
    
    # Use comprehensive no-std compilation database from CMake-generated artifacts
    compile_commands_path = project_root / "artifacts" / "examples" / "nostd_cmake_compile_commands.json"
    
    # Run Dosatsu
    run_dosatsu(dosatsu_path, compile_commands_path, db_path)
    
    # Connect to database
    db, conn = connect_to_database(db_path)
    
    return db_path, db, conn
