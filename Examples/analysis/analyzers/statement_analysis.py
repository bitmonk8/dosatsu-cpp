#!/usr/bin/env python3
"""
Analyze statement analysis including different statement types and patterns
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from framework import BaseAnalyzer

class StatementAnalysis(BaseAnalyzer):
    """Analyze statement analysis across all analyze files"""
    
    def run(self):
        """Analyze various statement types and their relationships"""
        
        # Analyze that Statement table exists and has entries
        self.framework.assert_query_has_results(
            "MATCH (s:Statement) RETURN s LIMIT 1",
            "Statement table should have entries"
        )
        
        # Analyze different statement kinds
        expected_statement_kinds = [
            "CompoundStmt", "DeclStmt", "ReturnStmt", "IfStmt", "ForStmt", 
            "WhileStmt", "DoStmt", "SwitchStmt", "CaseStmt", "DefaultStmt",
            "BreakStmt", "ContinueStmt", "ExprStmt", "NullStmt"
        ]
        
        found_statement_kinds = []
        total_statements = 0
        
        for stmt_kind in expected_statement_kinds:
            count = self.framework.query_count(f"""
                MATCH (s:Statement) 
                WHERE s.statement_kind = '{stmt_kind}'
                RETURN count(s) as count
            """)
            total_statements += count
            if count > 0:
                found_statement_kinds.append(stmt_kind)
                print(f"[OK] Found {count} {stmt_kind} statements")
            else:
                print(f"Warning: No {stmt_kind} statements found")
        
        print(f"Found {len(found_statement_kinds)} different statement kinds, {total_statements} total statements")
        
        # Analyze compound statements (blocks)
        compound_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.is_compound = true
            RETURN count(s) as count
        """)
        
        if compound_count > 0:
            print(f"[OK] Found {compound_count} compound statements")
            
            # Analyze nested compound statements
            nested_compound = self.framework.query_count("""
                MATCH (outer:Statement)-[:PARENT_OF*]->(inner:Statement)
                WHERE outer.is_compound = true AND inner.is_compound = true
                RETURN count(DISTINCT inner) as count
            """)
            
            if nested_compound > 0:
                print(f"  Nested compound statements: {nested_compound}")
        
        # Analyze statements with side effects
        side_effects_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.has_side_effects = true
            RETURN count(s) as count
        """)
        
        if side_effects_count > 0:
            print(f"[OK] Found {side_effects_count} statements with side effects")
        
        # Analyze control flow statements
        control_flow_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.control_flow_type IS NOT NULL AND s.control_flow_type <> ''
            RETURN count(s) as count
        """)
        
        if control_flow_count > 0:
            print(f"[OK] Found {control_flow_count} control flow statements")
            
            # Analyze control flow types
            control_types = self.framework.query_to_list("""
                MATCH (s:Statement)
                WHERE s.control_flow_type IS NOT NULL AND s.control_flow_type <> ''
                RETURN DISTINCT s.control_flow_type as type, count(*) as count
                ORDER BY count DESC
            """)
            
            if control_types:
                print("  Control flow types:")
                for cf_type in control_types:
                    print(f"    {cf_type['type']}: {cf_type['count']} statements")
        
        # Analyze conditional statements
        conditional_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.condition_text IS NOT NULL AND s.condition_text <> ''
            RETURN count(s) as count
        """)
        
        if conditional_count > 0:
            print(f"[OK] Found {conditional_count} conditional statements")
            
            # Show some condition examples
            conditions = self.framework.query_to_list("""
                MATCH (s:Statement)
                WHERE s.condition_text IS NOT NULL AND s.condition_text <> ''
                RETURN s.statement_kind as kind, s.condition_text as condition
                LIMIT 5
            """)
            
            print("  Example conditions:")
            for condition in conditions:
                print(f"    {condition['kind']}: {condition['condition']}")
        
        # Analyze constexpr statements
        constexpr_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.is_constexpr = true
            RETURN count(s) as count
        """)
        
        if constexpr_count > 0:
            print(f"[OK] Found {constexpr_count} constexpr statements")
        
        # Analyze expression statements
        expr_stmt_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'ExprStmt'
            RETURN count(s) as count
        """)
        
        if expr_stmt_count > 0:
            print(f"[OK] Found {expr_stmt_count} expression statements")
        
        # Analyze declaration statements
        decl_stmt_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'DeclStmt'
            RETURN count(s) as count
        """)
        
        if decl_stmt_count > 0:
            print(f"[OK] Found {decl_stmt_count} declaration statements")
        
        # Analyze loop statements
        loop_kinds = ["ForStmt", "WhileStmt", "DoStmt"]
        loop_count = 0
        
        for loop_kind in loop_kinds:
            count = self.framework.query_count(f"""
                MATCH (s:Statement)
                WHERE s.statement_kind = '{loop_kind}'
                RETURN count(s) as count
            """)
            loop_count += count
        
        if loop_count > 0:
            print(f"[OK] Found {loop_count} loop statements")
            
            # Analyze nested loops
            nested_loops = self.framework.query_count(f"""
                MATCH (outer:Statement)-[:PARENT_OF*]->(inner:Statement)
                WHERE outer.statement_kind IN {loop_kinds} 
                   AND inner.statement_kind IN {loop_kinds}
                RETURN count(DISTINCT inner) as count
            """)
            
            if nested_loops > 0:
                print(f"  Nested loops: {nested_loops}")
        
        # Analyze switch statements and cases
        switch_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'SwitchStmt'
            RETURN count(s) as count
        """)
        
        case_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'CaseStmt'
            RETURN count(s) as count
        """)
        
        default_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'DefaultStmt'
            RETURN count(s) as count
        """)
        
        if switch_count > 0:
            print(f"[OK] Switch statements: {switch_count} switches, {case_count} cases, {default_count} defaults")
        
        # Analyze jump statements
        jump_kinds = ["BreakStmt", "ContinueStmt", "ReturnStmt", "GotoStmt"]
        jump_counts = {}
        total_jumps = 0
        
        for jump_kind in jump_kinds:
            count = self.framework.query_count(f"""
                MATCH (s:Statement)
                WHERE s.statement_kind = '{jump_kind}'
                RETURN count(s) as count
            """)
            jump_counts[jump_kind] = count
            total_jumps += count
        
        if total_jumps > 0:
            print(f"[OK] Found {total_jumps} jump statements:")
            for jump_kind, count in jump_counts.items():
                if count > 0:
                    print(f"  {jump_kind}: {count}")
        
        # Analyze exception handling statements
        try_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'CXXTryStmt'
            RETURN count(s) as count
        """)
        
        catch_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'CXXCatchStmt'
            RETURN count(s) as count
        """)
        
        throw_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'CXXThrowExpr'
            RETURN count(s) as count
        """)
        
        if try_count > 0 or catch_count > 0 or throw_count > 0:
            print(f"[OK] Exception handling: {try_count} try, {catch_count} catch, {throw_count} throw")
        
        # Analyze label statements
        label_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'LabelStmt'
            RETURN count(s) as count
        """)
        
        if label_count > 0:
            print(f"[OK] Found {label_count} label statements")
        
        # Analyze null statements
        null_count = self.framework.query_count("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'NullStmt'
            RETURN count(s) as count
        """)
        
        if null_count > 0:
            print(f"[OK] Found {null_count} null statements")
        
        # Analyze statement nesting depth
        max_depth = self.framework.query_to_list("""
            MATCH path = (root:Statement)-[:PARENT_OF*]->(leaf:Statement)
            WHERE NOT (leaf)-[:PARENT_OF]->(:Statement)
            RETURN length(path) as depth
            ORDER BY depth DESC
            LIMIT 1
        """)
        
        if max_depth and max_depth[0]['depth'] is not None:
            print(f"[OK] Maximum statement nesting depth: {max_depth[0]['depth']}")
        
        # Analyze statements per function
        stmt_per_function = self.framework.query_to_list("""
            MATCH (a:ASTNode), (f:Declaration)-[:PARENT_OF*]->(s:Statement)
            WHERE a.node_id = f.node_id AND a.node_type IN ['FunctionDecl', 'CXXMethodDecl']
            WITH f, count(s) as stmt_count
            WHERE stmt_count > 5
            RETURN f.name as function_name, stmt_count
            ORDER BY stmt_count DESC
            LIMIT 5
        """)
        
        if stmt_per_function:
            print("[OK] Functions with most statements:")
            for func in stmt_per_function:
                print(f"  {func['function_name']}: {func['stmt_count']} statements")
        
        # Analyze for-loop components
        for_stmt_details = self.framework.query_to_list("""
            MATCH (s:Statement)
            WHERE s.statement_kind = 'ForStmt' AND s.condition_text IS NOT NULL
            RETURN s.condition_text as condition
            LIMIT 3
        """)
        
        if for_stmt_details:
            print("[OK] Example for-loop conditions:")
            for detail in for_stmt_details:
                print(f"  {detail['condition']}")
        
        # Analyze statements in different source files
        stmt_by_file = self.framework.query_to_list("""
            MATCH (s:Statement)-[:PARENT_OF*0..10]-(root:ASTNode)
            WHERE root.source_file IS NOT NULL
            WITH DISTINCT root.source_file as file, count(s) as stmt_count
            RETURN file, stmt_count
            ORDER BY stmt_count DESC
            LIMIT 5
        """)
        
        if stmt_by_file:
            print("[OK] Statements by source file:")
            for file_info in stmt_by_file:
                filename = file_info['file'].split('/')[-1] if '/' in file_info['file'] else file_info['file']
                print(f"  {filename}: {file_info['stmt_count']} statements")
        
        # Analyze complex statement patterns
        complex_if_count = self.framework.query_count("""
            MATCH (if_stmt:Statement)-[:PARENT_OF]->(nested:Statement)
            WHERE if_stmt.statement_kind = 'IfStmt' AND nested.statement_kind = 'IfStmt'
            RETURN count(DISTINCT if_stmt) as count
        """)
        
        if complex_if_count > 0:
            print(f"[OK] Found {complex_if_count} nested if statements")
        
        # Analyze early returns
        early_return_count = self.framework.query_count("""
            MATCH (a:ASTNode), (func:Declaration)-[:PARENT_OF*]->(ret:Statement)
            WHERE a.node_id = func.node_id AND a.node_type IN ['FunctionDecl', 'CXXMethodDecl'] 
               AND ret.statement_kind = 'ReturnStmt'
            WITH func, count(ret) as return_count
            WHERE return_count > 1
            RETURN count(func) as count
        """)
        
        if early_return_count > 0:
            print(f"[OK] Found {early_return_count} functions with multiple return statements")
        
        print("Statement analysis completed!")
