#!/usr/bin/env python3

import kuzu
import os
import sys

def main():
    # Check if database path was provided
    if len(sys.argv) < 2:
        print("Usage: python debug_database.py <database_path>")
        print("Example: python debug_database.py simple_test_db")
        return
    
    db_path = sys.argv[1]
    if not os.path.exists(db_path):
        print(f"Database file {db_path} does not exist!")
        return
    
    print(f"Database file size: {os.path.getsize(db_path)} bytes")
    
    try:
        # Connect to the generated database
        db = kuzu.Database(db_path)
        conn = kuzu.Connection(db)
        print("Successfully connected to database")
    except Exception as e:
        print(f"Failed to connect to database: {e}")
        return
    
    # Note: Kuzu doesn't support "SHOW TABLES" syntax, so we directly query known tables
    # Try to query ASTNode table
    try:
        result = conn.execute("MATCH (n:ASTNode) RETURN DISTINCT n.node_type as node_type ORDER BY node_type")
        
        print("\nNode types found in ASTNode table:")
        node_types = []
        while result.has_next():
            row = result.get_next()
            node_type = row[0]
            node_types.append(node_type)
            print(f"  - {node_type}")
        
        print(f"\nTotal node types: {len(node_types)}")
        
        # Show specific expected types
        expected_types = ["FunctionDecl", "CXXRecordDecl", "VarDecl", "CompoundStmt", "ReturnStmt"]
        print(f"\nChecking for expected types:")
        for expected in expected_types:
            if expected in node_types:
                print(f"  ✓ {expected} - FOUND")
            else:
                print(f"  ✗ {expected} - MISSING")
        
        # Count total nodes
        result = conn.execute("MATCH (n:ASTNode) RETURN COUNT(n) as total")
        if result.has_next():
            total_nodes = result.get_next()[0]
            print(f"\nTotal AST nodes: {total_nodes}")
            
    except Exception as e:
        print(f"ASTNode table not accessible: {e}")

if __name__ == "__main__":
    main()