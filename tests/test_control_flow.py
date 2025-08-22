#!/usr/bin/env python3
"""
Test control flow analysis and CFG block relationships
"""

from test_framework import BaseTest

class TestControlFlowTest(BaseTest):
    """Test control flow analysis from test_control_flow.cpp"""
    
    def run(self):
        """Test control flow statements and CFG blocks"""
        
        # Test statement declarations exist
        self.framework.assert_query_has_results(
            "MATCH (n:Statement) RETURN n LIMIT 1",
            "Should have statement declarations"
        )
        
        # Test different statement kinds
        expected_statement_kinds = [
            "IfStmt", "ForStmt", "WhileStmt", "DoStmt", "SwitchStmt", 
            "ReturnStmt", "BreakStmt", "ContinueStmt", "CompoundStmt"
        ]
        
        found_statement_kinds = []
        for stmt_kind in expected_statement_kinds:
            count = self.framework.query_count(f"""
                MATCH (s:Statement) 
                WHERE s.statement_kind = '{stmt_kind}'
                RETURN count(s) as count
            """)
            if count > 0:
                found_statement_kinds.append(stmt_kind)
                print(f"✓ Found {count} {stmt_kind} statements")
            else:
                print(f"Warning: No {stmt_kind} statements found")
        
        print(f"Found {len(found_statement_kinds)} different statement kinds")
        
        # Test control flow types
        control_flow_types = self.framework.query_to_list("""
            MATCH (s:Statement)
            WHERE s.control_flow_type IS NOT NULL
            RETURN DISTINCT s.control_flow_type as type, count(*) as count
            ORDER BY count DESC
        """)
        
        if control_flow_types:
            print("✓ Control flow types found:")
            for cf_type in control_flow_types:
                print(f"  {cf_type['type']}: {cf_type['count']} statements")
        
        # Test conditional statements with conditions
        conditional_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.condition_text IS NOT NULL AND s.condition_text <> ''
            RETURN count(s) as count
        """)
        
        if conditional_count > 0:
            print(f"✓ Found {conditional_count} conditional statements with condition text")
            
            # Show some examples
            conditions = self.framework.query_to_list("""
                MATCH (s:Statement)
                WHERE s.condition_text IS NOT NULL AND s.condition_text <> ''
                RETURN s.statement_kind as kind, s.condition_text as condition
                LIMIT 5
            """)
            
            for condition in conditions:
                print(f"  {condition['kind']}: {condition['condition']}")
        
        # Test compound statements
        compound_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.is_compound = true
            RETURN count(s) as count
        """)
        
        if compound_count > 0:
            print(f"✓ Found {compound_count} compound statements")
        
        # Test constexpr statements
        constexpr_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.is_constexpr = true
            RETURN count(s) as count
        """)
        
        if constexpr_count > 0:
            print(f"✓ Found {constexpr_count} constexpr statements")
        
        # Test CFG blocks
        cfg_block_count = self.framework.query_count(
            "MATCH (b:CFGBlock) RETURN count(b) as count"
        )
        
        if cfg_block_count > 0:
            print(f"✓ Found {cfg_block_count} CFG blocks")
            
            # Test entry and exit blocks
            entry_blocks = self.framework.query_count("""
                MATCH (b:CFGBlock)
                WHERE b.is_entry_block = true
                RETURN count(b) as count
            """)
            
            exit_blocks = self.framework.query_count("""
                MATCH (b:CFGBlock)
                WHERE b.is_exit_block = true
                RETURN count(b) as count
            """)
            
            print(f"  Entry blocks: {entry_blocks}")
            print(f"  Exit blocks: {exit_blocks}")
            
            # Test terminator kinds
            terminators = self.framework.query_to_list("""
                MATCH (b:CFGBlock)
                WHERE b.terminator_kind IS NOT NULL AND b.terminator_kind <> ''
                RETURN DISTINCT b.terminator_kind as kind, count(*) as count
                ORDER BY count DESC
            """)
            
            if terminators:
                print("  Terminator kinds:")
                for term in terminators:
                    print(f"    {term['kind']}: {term['count']} blocks")
        else:
            print("Warning: No CFG blocks found")
        
        # Test CFG edges
        cfg_edge_count = self.framework.query_count(
            "MATCH (from:CFGBlock)-[:CFG_EDGE]->(to:CFGBlock) RETURN count(*) as count"
        )
        
        if cfg_edge_count > 0:
            print(f"✓ Found {cfg_edge_count} CFG edges")
            
            # Test edge types
            edge_types = self.framework.query_to_list("""
                MATCH (from:CFGBlock)-[e:CFG_EDGE]->(to:CFGBlock)
                WHERE e.edge_type IS NOT NULL
                RETURN DISTINCT e.edge_type as type, count(*) as count
                ORDER BY count DESC
            """)
            
            if edge_types:
                print("  Edge types:")
                for edge_type in edge_types:
                    print(f"    {edge_type['type']}: {edge_type['count']} edges")
        else:
            print("Warning: No CFG edges found")
        
        # Test function-CFG relationships
        function_cfg_count = self.framework.query_count(
            "MATCH (f:Declaration)-[:CONTAINS_CFG]->(b:CFGBlock) RETURN count(*) as count"
        )
        
        if function_cfg_count > 0:
            print(f"✓ Found {function_cfg_count} function-to-CFG relationships")
            
            # Test functions with multiple blocks
            complex_functions = self.framework.query_to_list("""
                MATCH (f:Declaration)-[:CONTAINS_CFG]->(b:CFGBlock)
                WHERE f.node_type = 'FunctionDecl'
                WITH f, count(b) as block_count
                WHERE block_count > 3
                RETURN f.name as function_name, block_count
                ORDER BY block_count DESC
                LIMIT 5
            """)
            
            if complex_functions:
                print("  Functions with complex control flow:")
                for func in complex_functions:
                    print(f"    {func['function_name']}: {func['block_count']} blocks")
        else:
            print("Warning: No function-CFG relationships found")
        
        # Test CFG-statement relationships
        cfg_stmt_count = self.framework.query_count(
            "MATCH (b:CFGBlock)-[:CFG_CONTAINS_STMT]->(s:Statement) RETURN count(*) as count"
        )
        
        if cfg_stmt_count > 0:
            print(f"✓ Found {cfg_stmt_count} CFG block-to-statement relationships")
        else:
            print("Warning: No CFG-statement relationships found")
        
        # Test specific control flow patterns from test_control_flow.cpp
        
        # Test nested loops
        nested_loops = self.framework.query_count("""
            MATCH (outer:Statement)-[:PARENT_OF*]->(inner:Statement)
            WHERE outer.statement_kind IN ['ForStmt', 'WhileStmt', 'DoStmt']
              AND inner.statement_kind IN ['ForStmt', 'WhileStmt', 'DoStmt']
            RETURN count(DISTINCT inner) as count
        """)
        
        if nested_loops > 0:
            print(f"✓ Found {nested_loops} nested loop patterns")
        
        # Test exception handling
        try_stmt_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'CXXTryStmt'
            RETURN count(s) as count
        """)
        
        catch_stmt_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'CXXCatchStmt'
            RETURN count(s) as count
        """)
        
        if try_stmt_count > 0 or catch_stmt_count > 0:
            print(f"✓ Exception handling: {try_stmt_count} try blocks, {catch_stmt_count} catch blocks")
        
        # Test switch cases
        case_stmt_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'CaseStmt'
            RETURN count(s) as count
        """)
        
        default_stmt_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'DefaultStmt'
            RETURN count(s) as count
        """)
        
        if case_stmt_count > 0 or default_stmt_count > 0:
            print(f"✓ Switch statements: {case_stmt_count} case labels, {default_stmt_count} default labels")
        
        # Test goto statements and labels
        goto_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'GotoStmt'
            RETURN count(s) as count
        """)
        
        label_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'LabelStmt'
            RETURN count(s) as count
        """)
        
        if goto_count > 0 or label_count > 0:
            print(f"✓ Goto/Label: {goto_count} goto statements, {label_count} labels")
        
        # Test statements with side effects
        side_effects_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.has_side_effects = true
            RETURN count(s) as count
        """)
        
        if side_effects_count > 0:
            print(f"✓ Found {side_effects_count} statements with side effects")
        
        # Test unreachable blocks
        unreachable_count = self.framework.query_count("""
            MATCH (b:CFGBlock)
            WHERE b.reachable = false
            RETURN count(b) as count
        """)
        
        if unreachable_count > 0:
            print(f"Warning: Found {unreachable_count} unreachable CFG blocks")
        else:
            print("✓ No unreachable blocks detected")
        
        # Test control flow paths
        simple_paths = self.framework.query_count("""
            MATCH path = (entry:CFGBlock {is_entry_block: true})-[:CFG_EDGE*1..3]->(exit:CFGBlock {is_exit_block: true})
            WHERE entry.function_id = exit.function_id
            RETURN count(path) as count
        """)
        
        if simple_paths > 0:
            print(f"✓ Found {simple_paths} simple control flow paths")
        
        print("Control flow analysis completed!")
