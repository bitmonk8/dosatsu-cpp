#!/usr/bin/env python3
"""
Verify inheritance relationships from C++ code
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import kuzu
from query_operations import count_query_results, query_to_list
from assertion_helpers import assert_query_has_results
from verification_result import VerificationResult


def verify_inheritance_relationships(conn: kuzu.Connection) -> VerificationResult:
    """Verify inheritance relationships and virtual function overrides"""
    warnings = []
    errors = []
    details = {}
    
    try:
        # Check for class declarations
        if not has_class_declarations(conn):
            errors.append("No C++ class declarations found")
            return VerificationResult("InheritanceVerifier", False, warnings, errors, {})
        
        # Check specific classes
        missing_classes = check_expected_classes(conn, ["Base", "Derived", "TemplateClass", "SimpleString"])
        warnings.extend(f"Class {cls} not found" for cls in missing_classes)
        
        # Verify inheritance relationships exist
        inheritance_results = verify_inheritance_structure(conn)
        details.update(inheritance_results)
        
        # Check virtual function overrides
        override_results = verify_virtual_function_overrides(conn)
        details.update(override_results)
        
        # Check access specifiers
        access_results = verify_access_specifiers(conn)
        details.update(access_results)
        
        # Check inheritance hierarchies
        hierarchy_results = verify_inheritance_hierarchies(conn)
        details.update(hierarchy_results)
        
        # Check destructors
        destructor_results = verify_destructors(conn)
        details.update(destructor_results)
        
        return VerificationResult(
            verifier_name="InheritanceVerifier",
            passed=len(errors) == 0,
            warnings=warnings,
            errors=errors,
            details=details
        )
        
    except Exception as e:
        errors.append(f"Exception during inheritance verification: {str(e)}")
        return VerificationResult("InheritanceVerifier", False, warnings, errors, details)


def has_class_declarations(conn: kuzu.Connection) -> bool:
    """Check if class declarations exist"""
    try:
        assert_query_has_results(
            conn,
            "MATCH (a:ASTNode), (d:Declaration) WHERE a.node_type = 'CXXRecordDecl' AND a.node_id = d.node_id RETURN d LIMIT 1",
            "Should have C++ class declarations"
        )
        return True
    except AssertionError:
        return False


def check_expected_classes(conn: kuzu.Connection, expected: list[str]) -> list[str]:
    """Return list of missing expected classes"""
    missing_classes = []
    
    for class_name in expected:
        count = count_query_results(conn, 
            f"MATCH (a:ASTNode), (d:Declaration) WHERE a.node_type = 'CXXRecordDecl' AND a.node_id = d.node_id AND d.name = '{class_name}' RETURN d"
        )
        if count == 0:
            missing_classes.append(class_name)
        else:
            print(f"[OK] Found class {class_name}")
    
    return missing_classes


def verify_inheritance_structure(conn: kuzu.Connection) -> dict:
    """Verify inheritance relationships, return detailed results"""
    results = {}
    
    # Check that INHERITS_FROM relationships exist
    try:
        assert_query_has_results(
            conn,
            "MATCH (derived:Declaration)-[:INHERITS_FROM]->(base:Declaration) RETURN derived, base LIMIT 1",
            "Should have inheritance relationships"
        )
        results["has_inheritance_relationships"] = True
    except AssertionError:
        results["has_inheritance_relationships"] = False
        return results
    
    # Check specific inheritance relationships
    inheritance_tests = [
        ("Mammal", "Animal"),
        ("Bat", "Mammal"),
        ("WaterBird", "Animal"),
    ]
    
    found_relationships = []
    for derived, base in inheritance_tests:
        count = count_query_results(conn, f"""
            MATCH (derived:Declaration)-[:INHERITS_FROM]->(base:Declaration)
            WHERE derived.name = '{derived}' AND base.name = '{base}'
            RETURN count(*) as count
        """)
        if count > 0:
            print(f"[OK] Found inheritance: {derived} -> {base}")
            found_relationships.append((derived, base))
        else:
            print(f"Warning: Inheritance {derived} -> {base} not found")
    
    results["found_inheritance_relationships"] = found_relationships
    
    # Check multiple inheritance (Bat inherits from both Mammal and Flyable)
    bat_inheritance = count_query_results(conn, """
        MATCH (bat:Declaration)-[:INHERITS_FROM]->(base:Declaration)
        WHERE bat.name = 'Bat'
        RETURN count(*) as count
    """)
    
    results["bat_multiple_inheritance_count"] = bat_inheritance
    if bat_inheritance >= 2:
        print(f"[OK] Bat has multiple inheritance ({bat_inheritance} base classes)")
    else:
        print(f"Warning: Bat multiple inheritance not fully detected ({bat_inheritance} bases)")
    
    return results


def verify_virtual_function_overrides(conn: kuzu.Connection) -> dict:
    """Verify virtual function overrides"""
    results = {}
    
    # Check for method declarations
    try:
        assert_query_has_results(
            conn,
            "MATCH (a:ASTNode), (d:Declaration) WHERE a.node_type = 'CXXMethodDecl' AND a.node_id = d.node_id RETURN d LIMIT 1",
            "Should have method declarations"
        )
        results["has_method_declarations"] = True
    except AssertionError:
        results["has_method_declarations"] = False
        return results
    
    # Check OVERRIDES relationships
    override_count = count_query_results(conn,
        "MATCH (derived:Declaration)-[:OVERRIDES]->(base:Declaration) RETURN count(*) as count"
    )
    
    results["override_count"] = override_count
    
    if override_count > 0:
        print(f"[OK] Found {override_count} virtual function overrides")
        
        # Get some examples
        overrides = query_to_list(conn, """
            MATCH (derived:Declaration)-[:OVERRIDES]->(base:Declaration)
            RETURN derived.name as derived_method, base.name as base_method
            LIMIT 5
        """)
        
        results["override_examples"] = overrides
        for override in overrides:
            print(f"  Override: {override['derived_method']} overrides {override['base_method']}")
    else:
        print("Warning: No virtual function overrides detected")
        results["override_examples"] = []
    
    return results


def verify_access_specifiers(conn: kuzu.Connection) -> dict:
    """Verify access specifiers in inheritance"""
    results = {}
    access_tests = ["public", "private", "protected"]
    
    for access in access_tests:
        count = count_query_results(conn, f"""
            MATCH (r:Declaration)-[:INHERITS_FROM {{inheritance_type: '{access}'}}]->(base:Declaration)
            RETURN count(*) as count
        """)
        results[f"{access}_inheritance_count"] = count
        if count > 0:
            print(f"[OK] Found {count} {access} inheritance relationships")
    
    # Check virtual inheritance
    virtual_inheritance = count_query_results(conn, """
        MATCH (derived:Declaration)-[r:INHERITS_FROM]->(base:Declaration)
        WHERE r.is_virtual = true
        RETURN count(*) as count
    """)
    
    results["virtual_inheritance_count"] = virtual_inheritance
    if virtual_inheritance > 0:
        print(f"[OK] Found {virtual_inheritance} virtual inheritance relationships")
    else:
        print("No virtual inheritance detected (expected for test_inheritance.cpp)")
    
    return results


def verify_inheritance_hierarchies(conn: kuzu.Connection) -> dict:
    """Verify inheritance hierarchies (multi-level)"""
    results = {}
    
    hierarchy_depth = query_to_list(conn, """
        MATCH path = (derived:Declaration)-[:INHERITS_FROM*1..3]->(ancestor:Declaration),
              (ad:ASTNode), (aa:ASTNode)
        WHERE ad.node_id = derived.node_id AND aa.node_id = ancestor.node_id
          AND ad.node_type = 'CXXRecordDecl' AND aa.node_type = 'CXXRecordDecl'
        RETURN derived.name as derived, ancestor.name as ancestor, length(path) as depth
        ORDER BY depth DESC, derived.name
        LIMIT 10
    """)
    
    results["inheritance_hierarchies"] = hierarchy_depth
    if hierarchy_depth:
        print("[OK] Inheritance hierarchies found:")
        for hier in hierarchy_depth[:5]:
            print(f"  {hier['derived']} -> {hier['ancestor']} (depth: {hier['depth']})")
    
    return results


def verify_destructors(conn: kuzu.Connection) -> dict:
    """Verify destructors"""
    results = {}
    
    destructor_count = count_query_results(conn, """
        MATCH (destructor:Declaration)
        WHERE destructor.node_type = 'CXXDestructorDecl'
        RETURN count(*) as count
    """)
    
    results["destructor_count"] = destructor_count
    if destructor_count > 0:
        print(f"[OK] Found {destructor_count} destructors")
    
    # Check pure virtual functions (abstract classes)
    pure_virtual_count = count_query_results(conn, """
        MATCH (method:Declaration)
        WHERE method.node_type = 'CXXMethodDecl' AND method.name CONTAINS 'pure'
        RETURN count(*) as count
    """)
    
    results["pure_virtual_count"] = pure_virtual_count
    print(f"Found {pure_virtual_count} potential pure virtual functions")
    
    return results
