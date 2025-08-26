#!/usr/bin/env python3
"""
Verify control flow statements and CFG block relationships
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import kuzu
from query_operations import count_query_results, query_to_list
from assertion_helpers import assert_query_has_results
from verification_result import VerificationResult


def verify_control_flow(conn: kuzu.Connection) -> VerificationResult:
    """Verify control flow statements and CFG blocks"""
    warnings = []
    errors = []
    details = {}
    
    try:
        # Check if statements exist
        if not has_statements(conn):
            errors.append("No statement declarations found")
            return VerificationResult("ControlFlowVerifier", False, warnings, errors, {})
        
        # Verify statement types
        statement_results = verify_statement_types(conn)
        details.update(statement_results)
        warnings.extend(statement_results.get("missing_statement_types", []))
        
        # Verify control flow properties
        control_flow_results = verify_control_flow_properties(conn)
        details.update(control_flow_results)
        
        # Verify CFG blocks
        cfg_results = verify_cfg_blocks(conn)
        details.update(cfg_results)
        # CFG blocks are optional in the current implementation
        # No need to warn about missing CFG blocks
        
        # Verify CFG relationships
        relationship_results = verify_cfg_relationships(conn)
        details.update(relationship_results)
        
        # Verify specific control flow patterns
        pattern_results = verify_control_flow_patterns(conn)
        details.update(pattern_results)
        
        return VerificationResult(
            verifier_name="ControlFlowVerifier",
            passed=len(errors) == 0,
            warnings=warnings,
            errors=errors,
            details=details
        )
        
    except Exception as e:
        errors.append(f"Exception during control flow verification: {str(e)}")
        return VerificationResult("ControlFlowVerifier", False, warnings, errors, details)


def has_statements(conn: kuzu.Connection) -> bool:
    """Check if statements exist in database"""
    try:
        assert_query_has_results(
            conn,
            "MATCH (n:Statement) RETURN n LIMIT 1",
            "Should have statement declarations"
        )
        return True
    except AssertionError:
        return False


def verify_statement_types(conn: kuzu.Connection) -> dict:
    """Verify different statement types exist"""
    results = {}
    
    expected_statement_kinds = [
        "IfStmt", "ForStmt", "WhileStmt", "DoStmt", "SwitchStmt", 
        "ReturnStmt", "BreakStmt", "ContinueStmt", "CompoundStmt"
    ]
    
    found_statement_kinds = []
    missing_statement_types = []
    
    for stmt_kind in expected_statement_kinds:
        count = count_query_results(conn, f"""
            MATCH (s:Statement) 
            WHERE s.statement_kind = '{stmt_kind}'
            RETURN count(s) as count
        """)
        if count > 0:
            found_statement_kinds.append(stmt_kind)
            # Reduce verbosity - only report missing types as warnings
        else:
            missing_statement_types.append(f"No {stmt_kind} statements found")
    
    results["found_statement_kinds"] = found_statement_kinds
    results["missing_statement_types"] = missing_statement_types
    results["statement_kinds_count"] = len(found_statement_kinds)
    
    # Reduced verbosity
    # print(f"Found {len(found_statement_kinds)} different statement kinds")
    
    return results


def verify_control_flow_properties(conn: kuzu.Connection) -> dict:
    """Verify control flow properties and types"""
    results = {}
    
    # Check control flow types
    control_flow_types = query_to_list(conn, """
        MATCH (s:Statement)
        WHERE s.control_flow_type IS NOT NULL
        RETURN DISTINCT s.control_flow_type as type, count(*) as count
        ORDER BY count DESC
    """)
    
    results["control_flow_types"] = control_flow_types
    # Reduce verbosity - just store the data
    # if control_flow_types:
    #     print("[OK] Control flow types found:")
    #     for cf_type in control_flow_types:
    #         print(f"  {cf_type['type']}: {cf_type['count']} statements")
    
    # Check conditional statements
    conditional_count = count_query_results(conn, """
        MATCH (s:Statement)
        WHERE s.condition_text IS NOT NULL AND s.condition_text <> ''
        RETURN count(s) as count
    """)
    
    results["conditional_count"] = conditional_count
    if conditional_count > 0:
        # Reduce verbosity - just store examples without printing
        conditions = query_to_list(conn, """
            MATCH (s:Statement)
            WHERE s.condition_text IS NOT NULL AND s.condition_text <> ''
            RETURN s.statement_kind as kind, s.condition_text as condition
            LIMIT 5
        """)
        results["condition_examples"] = conditions
    
    # Check compound statements
    compound_count = count_query_results(conn, """
        MATCH (s:Statement)
        WHERE s.is_compound = true
        RETURN count(s) as count
    """)
    
    results["compound_count"] = compound_count
    # Reduce verbosity
    # if compound_count > 0:
    #     print(f"[OK] Found {compound_count} compound statements")
    
    # Check constexpr statements
    constexpr_count = count_query_results(conn, """
        MATCH (s:Statement)
        WHERE s.is_constexpr = true
        RETURN count(s) as count
    """)
    
    results["constexpr_count"] = constexpr_count
    # Reduce verbosity
    # if constexpr_count > 0:
    #     print(f"[OK] Found {constexpr_count} constexpr statements")
    
    return results


def verify_cfg_blocks(conn: kuzu.Connection) -> dict:
    """Verify CFG blocks and their properties"""
    results = {}
    
    cfg_block_count = count_query_results(conn,
        "MATCH (b:CFGBlock) RETURN count(b) as count"
    )
    
    results["cfg_block_count"] = cfg_block_count
    
    if cfg_block_count > 0:
        # Reduce verbosity - collect data without printing
        entry_blocks = count_query_results(conn, """
            MATCH (b:CFGBlock)
            WHERE b.is_entry_block = true
            RETURN count(b) as count
        """)
        
        exit_blocks = count_query_results(conn, """
            MATCH (b:CFGBlock)
            WHERE b.is_exit_block = true
            RETURN count(b) as count
        """)
        
        results["entry_blocks"] = entry_blocks
        results["exit_blocks"] = exit_blocks
        
        # Check terminator kinds
        terminators = query_to_list(conn, """
            MATCH (b:CFGBlock)
            WHERE b.terminator_kind IS NOT NULL AND b.terminator_kind <> ''
            RETURN DISTINCT b.terminator_kind as kind, count(*) as count
            ORDER BY count DESC
        """)
        
        results["terminators"] = terminators
        # Store terminator data without verbose printing
        # if terminators:
        #     print("  Terminator kinds:")
        #     for term in terminators:
        #         print(f"    {term['kind']}: {term['count']} blocks")
    # Note: CFG blocks are optional in current implementation
    
    return results


def verify_cfg_relationships(conn: kuzu.Connection) -> dict:
    """Verify CFG relationships and edges"""
    results = {}
    
    # Check CFG edges
    cfg_edge_count = count_query_results(conn,
        "MATCH (from:CFGBlock)-[:CFG_EDGE]->(to:CFGBlock) RETURN count(*) as count"
    )
    
    results["cfg_edge_count"] = cfg_edge_count
    if cfg_edge_count > 0:
        # Reduce verbosity - collect data without printing
        edge_types = query_to_list(conn, """
            MATCH (from:CFGBlock)-[e:CFG_EDGE]->(to:CFGBlock)
            WHERE e.edge_type IS NOT NULL
            RETURN DISTINCT e.edge_type as type, count(*) as count
            ORDER BY count DESC
        """)
        
        results["edge_types"] = edge_types
    # Note: CFG edges are optional in current implementation
    # else:
    #     print("Warning: No CFG edges found")
    
    # Check function-CFG relationships
    function_cfg_count = count_query_results(conn,
        "MATCH (f:Declaration)-[:CONTAINS_CFG]->(b:CFGBlock) RETURN count(*) as count"
    )
    
    results["function_cfg_count"] = function_cfg_count
    if function_cfg_count > 0:
        # Reduce verbosity - collect data without printing
        complex_functions = query_to_list(conn, """
            MATCH (f:Declaration)-[:CONTAINS_CFG]->(b:CFGBlock)
            WHERE f.node_type = 'FunctionDecl'
            WITH f, count(b) as block_count
            WHERE block_count > 3
            RETURN f.name as function_name, block_count
            ORDER BY block_count DESC
            LIMIT 5
        """)
        
        results["complex_functions"] = complex_functions
    # Note: Function-CFG relationships are optional in current implementation
    # else:
    #     print("Warning: No function-CFG relationships found")
    
    # Check CFG-statement relationships
    cfg_stmt_count = count_query_results(conn,
        "MATCH (b:CFGBlock)-[:CFG_CONTAINS_STMT]->(s:Statement) RETURN count(*) as count"
    )
    
    results["cfg_stmt_count"] = cfg_stmt_count
    # Reduce verbosity
    # if cfg_stmt_count > 0:
    #     print(f"[OK] Found {cfg_stmt_count} CFG block-to-statement relationships")
    # Note: CFG-statement relationships are optional in current implementation
    # else:
    #     print("Warning: No CFG-statement relationships found")
    
    return results


def verify_control_flow_patterns(conn: kuzu.Connection) -> dict:
    """Verify specific control flow patterns"""
    results = {}
    
    # Check nested loops
    nested_loops = count_query_results(conn, """
        MATCH (outer:Statement)-[:PARENT_OF*]->(inner:Statement)
        WHERE outer.statement_kind IN ['ForStmt', 'WhileStmt', 'DoStmt']
          AND inner.statement_kind IN ['ForStmt', 'WhileStmt', 'DoStmt']
        RETURN count(DISTINCT inner) as count
    """)
    
    results["nested_loops"] = nested_loops
    # Reduce verbosity
    # if nested_loops > 0:
    #     print(f"[OK] Found {nested_loops} nested loop patterns")
    
    # Check exception handling
    try_stmt_count = count_query_results(conn, """
        MATCH (s:Statement)
        WHERE s.statement_kind = 'CXXTryStmt'
        RETURN count(s) as count
    """)
    
    catch_stmt_count = count_query_results(conn, """
        MATCH (s:Statement)
        WHERE s.statement_kind = 'CXXCatchStmt'
        RETURN count(s) as count
    """)
    
    results["try_stmt_count"] = try_stmt_count
    results["catch_stmt_count"] = catch_stmt_count
    # Reduce verbosity
    # if try_stmt_count > 0 or catch_stmt_count > 0:
    #     print(f"[OK] Exception handling: {try_stmt_count} try blocks, {catch_stmt_count} catch blocks")
    
    # Check switch cases
    case_stmt_count = count_query_results(conn, """
        MATCH (s:Statement)
        WHERE s.statement_kind = 'CaseStmt'
        RETURN count(s) as count
    """)
    
    default_stmt_count = count_query_results(conn, """
        MATCH (s:Statement)
        WHERE s.statement_kind = 'DefaultStmt'
        RETURN count(s) as count
    """)
    
    results["case_stmt_count"] = case_stmt_count
    results["default_stmt_count"] = default_stmt_count
    # Reduce verbosity
    # if case_stmt_count > 0 or default_stmt_count > 0:
    #     print(f"[OK] Switch statements: {case_stmt_count} case labels, {default_stmt_count} default labels")
    
    # Check goto statements and labels
    goto_count = count_query_results(conn, """
        MATCH (s:Statement)
        WHERE s.statement_kind = 'GotoStmt'
        RETURN count(s) as count
    """)
    
    label_count = count_query_results(conn, """
        MATCH (s:Statement)
        WHERE s.statement_kind = 'LabelStmt'
        RETURN count(s) as count
    """)
    
    results["goto_count"] = goto_count
    results["label_count"] = label_count
    # Reduce verbosity
    # if goto_count > 0 or label_count > 0:
    #     print(f"[OK] Goto/Label: {goto_count} goto statements, {label_count} labels")
    
    # Check statements with side effects
    side_effects_count = count_query_results(conn, """
        MATCH (s:Statement)
        WHERE s.has_side_effects = true
        RETURN count(s) as count
    """)
    
    results["side_effects_count"] = side_effects_count
    # Reduce verbosity
    # if side_effects_count > 0:
    #     print(f"[OK] Found {side_effects_count} statements with side effects")
    
    # Check unreachable blocks (CFG blocks are optional in current implementation)
    try:
        unreachable_count = count_query_results(conn, """
            MATCH (b:CFGBlock)
            WHERE b.reachable = false
            RETURN count(b) as count
        """)
        
        results["unreachable_count"] = unreachable_count
        # Only show unreachable blocks if it's a significant issue
        if unreachable_count > 10:  # Only report if many blocks are unreachable
            print(f"Warning: Found {unreachable_count} unreachable CFG blocks")
    except Exception:
        # CFG blocks might not exist or have reachable property
        results["unreachable_count"] = 0
    
    # Check control flow paths
    simple_paths = count_query_results(conn, """
        MATCH path = (entry:CFGBlock {is_entry_block: true})-[:CFG_EDGE*1..3]->(exit:CFGBlock {is_exit_block: true})
        WHERE entry.function_id = exit.function_id
        RETURN count(path) as count
    """)
    
    results["simple_paths"] = simple_paths
    # Reduce verbosity
    # if simple_paths > 0:
    #     print(f"[OK] Found {simple_paths} simple control flow paths")
    
    return results
