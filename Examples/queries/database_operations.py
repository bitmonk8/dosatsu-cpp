#!/usr/bin/env python3
"""
Database operations for CppGraphIndex verification queries
"""

import os
import subprocess
import tempfile
import shutil
from pathlib import Path
from typing import Tuple
import kuzu


def create_temp_database(project_root: Path) -> str:
    """Create temporary database and return path"""
    temp_db_path = tempfile.mkdtemp(prefix="kuzu_test_")
    db_path = os.path.join(temp_db_path, "test_db")
    return db_path


def run_makeindex(makeindex_path: Path, compile_commands: Path, db_path: str) -> bool:
    """Run MakeIndex on compilation database"""
    if not makeindex_path.exists():
        raise FileNotFoundError(f"MakeIndex not found at {makeindex_path}. Run 'please build' first.")
    
    if not compile_commands.exists():
        raise FileNotFoundError(f"Compilation database not found at {compile_commands}")
    
    print(f"Running MakeIndex on {compile_commands}...")
    print(f"Output database: {db_path}")
    
    # Execute MakeIndex
    cmd = [
        str(makeindex_path),
        str(compile_commands),
        "--output-db", db_path
    ]
    
    try:
        result = subprocess.run(cmd, 
                              capture_output=True, 
                              text=True, 
                              cwd=str(makeindex_path.parent.parent.parent),  # project root
                              timeout=300)  # 5 minute timeout
        
        if result.returncode != 0:
            print(f"MakeIndex stdout: {result.stdout}")
            print(f"MakeIndex stderr: {result.stderr}")
            raise RuntimeError(f"MakeIndex failed with return code {result.returncode}")
        
        print("MakeIndex completed successfully")
        print(f"stdout: {result.stdout}")
        return True
        
    except subprocess.TimeoutExpired:
        raise RuntimeError("MakeIndex timed out after 5 minutes")


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
    """Get standard project paths (project_root, makeindex_path, example_data_path)"""
    if project_root is None:
        # Auto-detect project root - go up from Examples/queries to project root
        current_dir = Path(__file__).parent.absolute()
        project_root = current_dir.parent.parent
    else:
        project_root = Path(project_root)
    
    makeindex_path = project_root / "artifacts" / "debug" / "bin" / "MakeIndex.exe"
    example_data_path = project_root / "Examples" / "cpp"
    
    return project_root, makeindex_path, example_data_path


def setup_example_database(project_root: Path = None) -> Tuple[str, kuzu.Database, kuzu.Connection]:
    """Create a temporary database and run MakeIndex on the example files"""
    project_root, makeindex_path, example_data_path = get_project_paths(project_root)
    
    # Create temporary database
    db_path = create_temp_database(project_root)
    
    # Use comprehensive no-std compilation database
    compile_commands_path = example_data_path / "compilation" / "comprehensive_no_std_compile_commands.json"
    
    # Run MakeIndex
    run_makeindex(makeindex_path, compile_commands_path, db_path)
    
    # Connect to database
    db, conn = connect_to_database(db_path)
    
    return db_path, db, conn
