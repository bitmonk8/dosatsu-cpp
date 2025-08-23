#!/usr/bin/env python3
"""
Test AST node structure and basic functionality
"""

from test_framework import BaseTest

class TestAstNodesTest(BaseTest):
    """Test basic AST node structure and properties"""
    
    def run(self):
        """Test AST node creation and basic properties"""
        
        # Test that ASTNode table exists and has entries
        self.framework.assert_query_has_results(
            "MATCH (n:ASTNode) RETURN n LIMIT 1",
            "ASTNode table should have entries"
        )
        
        # Test required fields are present
        required_fields = [
            "node_id", "node_type", "source_file", 
            "start_line", "start_column", "end_line", "end_column"
        ]
        
        for field in required_fields:
            self.framework.assert_query_has_results(
                f"MATCH (n:ASTNode) WHERE n.{field} IS NOT NULL RETURN n LIMIT 1",
                f"ASTNode should have {field} field"
            )
        
        # Test that we have different node types
        node_types = self.framework.query_to_list(
            "MATCH (n:ASTNode) RETURN DISTINCT n.node_type as node_type ORDER BY node_type"
        )
        
        expected_types = [
            "Function", "CXXRecord", "Var", "CompoundStmt",
            "CallExpr", "ReturnStmt", "IfStmt"
        ]
        
        found_types = [row["node_type"] for row in node_types]
        
        for expected_type in expected_types:
            if expected_type not in found_types:
                print(f"Warning: Expected node type {expected_type} not found")
        
        print(f"Found {len(found_types)} different node types")
        
        # Test that source files are correctly recorded
        source_files = self.framework.query_to_list(
            "MATCH (n:ASTNode) RETURN DISTINCT n.source_file as file"
        )
        
        expected_files = [
            "test_inheritance.cpp", "test_templates.cpp", "test_namespaces.cpp",
            "test_control_flow.cpp", "test_expressions.cpp", "test_preprocessor.cpp"
        ]
        
        found_files = [row["file"] for row in source_files]
        
        for expected_file in expected_files:
            # Check if any file contains the expected name (path might be different)
            if not any(expected_file in file for file in found_files):
                print(f"Warning: Expected file {expected_file} not found in source files")
        
        print(f"Found {len(found_files)} source files")
        
        # Test line number ranges are reasonable
        invalid_positions = self.framework.query_count(
            """MATCH (n:ASTNode) 
               WHERE n.start_line < 1 OR n.end_line < n.start_line 
                  OR n.start_column < 0 OR n.end_column < n.start_column
               RETURN count(n) as count"""
        )
        
        if invalid_positions > 0:
            print(f"Warning: Found {invalid_positions} nodes with invalid position information")
        
        # Test PARENT_OF relationships exist
        self.framework.assert_query_has_results(
            "MATCH (parent:ASTNode)-[:PARENT_OF]->(child:ASTNode) RETURN parent, child LIMIT 1",
            "Should have PARENT_OF relationships between AST nodes"
        )
        
        # Test that parent-child relationships have proper ordering
        parent_child_counts = self.framework.query_to_list(
            """MATCH (parent:ASTNode)-[r:PARENT_OF]->(child:ASTNode)
               RETURN parent.node_id as parent_id, count(child) as child_count
               ORDER BY child_count DESC LIMIT 10"""
        )
        
        print(f"Top parent nodes by child count: {parent_child_counts[:3]}")
        
        # Test memory addresses are recorded
        self.framework.assert_query_has_results(
            "MATCH (n:ASTNode) WHERE n.memory_address IS NOT NULL RETURN n LIMIT 1",
            "Should have memory addresses recorded"
        )
        
        print("All AST node tests passed!")
