#!/usr/bin/env python3
"""
Query operations for Dosatsu verification
"""

from typing import List, Dict, Any
import kuzu


def execute_query(conn: kuzu.Connection, cypher_query: str) -> kuzu.QueryResult:
    """Execute Cypher query"""
    if not conn:
        raise RuntimeError("Database not connected. Call connect_to_database() first.")
    
    return conn.execute(cypher_query)


def query_to_list(conn: kuzu.Connection, cypher_query: str) -> List[Dict[str, Any]]:
    """Execute query and return as list of dictionaries"""
    result = execute_query(conn, cypher_query)
    results = []
    try:
        columns = result.get_column_names()
        for row in result:
            row_dict = {}
            for i, value in enumerate(row):
                if i < len(columns):
                    row_dict[columns[i]] = value
            results.append(row_dict)
    except Exception as e:
        print(f"Error converting query results: {e}")
    return results


def count_query_results(conn: kuzu.Connection, cypher_query: str) -> int:
    """Count results from query"""
    try:
        results = query_to_list(conn, cypher_query)
        
        # If this is a count query that returns a single value, extract it
        if len(results) == 1 and len(results[0]) == 1:
            count_value = list(results[0].values())[0]
            # Handle different types of return values
            if isinstance(count_value, (int, float)):
                return int(count_value)
            elif isinstance(count_value, str) and count_value.isdigit():
                return int(count_value)
            elif count_value is None:
                return 0
            else:
                # If it's not a simple count value (e.g., a dict/object), treat as one result
                return 1
        
        # For non-count queries, return the number of result rows
        return len(results)
    except Exception as e:
        print(f"Query error: {e}")
        return 0


def get_table_info(conn: kuzu.Connection) -> Dict[str, int]:
    """Get information about database tables"""
    tables = {}
    
    # Get node table counts
    node_tables = [
        "ASTNode", "Declaration", "Type", "Statement", "Expression",
        "TemplateParameter", "UsingDeclaration", "MacroDefinition",
        "IncludeDirective", "Comment", "ConstantExpression", "CFGBlock"
    ]
    
    for table in node_tables:
        try:
            # Use a simple count query
            result_list = query_to_list(conn, f"MATCH (n:{table}) RETURN count(n) as count")
            if result_list and 'count' in result_list[0]:
                tables[table] = result_list[0]['count']
            else:
                tables[table] = 0
        except Exception as e:
            tables[table] = f"Error: {e}"
    
    return tables


def print_database_summary(conn: kuzu.Connection):
    """Print a summary of the database contents"""
    print("\n=== Database Summary ===")
    table_info = get_table_info(conn)
    
    for table, count in table_info.items():
        print(f"{table}: {count}")
    
    print("\n=== Sample ASTNode entries ===")
    try:
        sample_nodes = query_to_list(conn, "MATCH (n:ASTNode) RETURN n.node_type as node_type, n.source_file as source_file, n.start_line as start_line LIMIT 10")
        for node in sample_nodes:
            print(f"  {node['node_type']} in {node['source_file']}:{node['start_line']}")
    except Exception as e:
        print(f"Error getting sample nodes: {e}")
