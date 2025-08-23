#!/usr/bin/env python3
"""
Test inheritance relationships and virtual function analysis
"""

from test_framework import BaseTest

class TestInheritanceTest(BaseTest):
    """Test inheritance analysis from test_inheritance.cpp"""
    
    def run(self):
        """Test inheritance relationships and virtual function overrides"""
        
        # Test that we have class declarations  
        self.framework.assert_query_has_results(
            "MATCH (a:ASTNode), (d:Declaration) WHERE a.node_type = 'CXXRecord' AND a.node_id = d.node_id RETURN d LIMIT 1",
            "Should have C++ class declarations"
        )
        
        # Test for specific classes from comprehensive_test_no_std.cpp
        expected_classes = [
            "Base", "Derived", "TemplateClass", "SimpleString"
        ]
        
        for class_name in expected_classes:
            results = self.framework.query_count(
                f"MATCH (a:ASTNode), (d:Declaration) WHERE a.node_type = 'CXXRecord' AND a.node_id = d.node_id AND d.name = '{class_name}' RETURN d"
            )
            if results == 0:
                print(f"Warning: Class {class_name} not found")
            else:
                print(f"[OK] Found class {class_name}")
        
        # Test INHERITS_FROM relationships exist
        self.framework.assert_query_has_results(
            "MATCH (derived:Declaration)-[:INHERITS_FROM]->(base:Declaration) RETURN derived, base LIMIT 1",
            "Should have inheritance relationships"
        )
        
        # Test specific inheritance relationships
        inheritance_tests = [
            ("Mammal", "Animal"),
            ("Bat", "Mammal"),
            ("WaterBird", "Animal"),
        ]
        
        for derived, base in inheritance_tests:
            count = self.framework.query_count(f"""
                MATCH (derived:Declaration)-[:INHERITS_FROM]->(base:Declaration)
                WHERE derived.name = '{derived}' AND base.name = '{base}'
                RETURN count(*) as count
            """)
            if count > 0:
                print(f"[OK] Found inheritance: {derived} -> {base}")
            else:
                print(f"Warning: Inheritance {derived} -> {base} not found")
        
        # Test multiple inheritance (Bat inherits from both Mammal and Flyable)
        bat_inheritance = self.framework.query_count("""
            MATCH (bat:Declaration)-[:INHERITS_FROM]->(base:Declaration)
            WHERE bat.name = 'Bat'
            RETURN count(*) as count
        """)
        
        if bat_inheritance >= 2:
            print(f"[OK] Bat has multiple inheritance ({bat_inheritance} base classes)")
        else:
            print(f"Warning: Bat multiple inheritance not fully detected ({bat_inheritance} bases)")
        
        # Test virtual functions
        self.framework.assert_query_has_results(
            "MATCH (a:ASTNode), (d:Declaration) WHERE a.node_type = 'CXXMethodDecl' AND a.node_id = d.node_id RETURN d LIMIT 1",
            "Should have method declarations"
        )
        
        # Test OVERRIDES relationships
        override_count = self.framework.query_count(
            "MATCH (derived:Declaration)-[:OVERRIDES]->(base:Declaration) RETURN count(*) as count"
        )
        
        if override_count > 0:
            print(f"[OK] Found {override_count} virtual function overrides")
            
            # Get some examples
            overrides = self.framework.query_to_list("""
                MATCH (derived:Declaration)-[:OVERRIDES]->(base:Declaration)
                RETURN derived.name as derived_method, base.name as base_method
                LIMIT 5
            """)
            
            for override in overrides:
                print(f"  Override: {override['derived_method']} overrides {override['base_method']}")
        else:
            print("Warning: No virtual function overrides detected")
        
        # Test access specifiers
        access_tests = ["public", "private", "protected"]
        for access in access_tests:
            count = self.framework.query_count(f"""
                MATCH (r:Declaration)-[:INHERITS_FROM {{inheritance_type: '{access}'}}]->(base:Declaration)
                RETURN count(*) as count
            """)
            if count > 0:
                print(f"[OK] Found {count} {access} inheritance relationships")
        
        # Test virtual inheritance
        virtual_inheritance = self.framework.query_count("""
            MATCH (derived:Declaration)-[r:INHERITS_FROM]->(base:Declaration)
            WHERE r.is_virtual = true
            RETURN count(*) as count
        """)
        
        if virtual_inheritance > 0:
            print(f"[OK] Found {virtual_inheritance} virtual inheritance relationships")
        else:
            print("No virtual inheritance detected (expected for test_inheritance.cpp)")
        
        # Test inheritance hierarchies (multi-level)
        hierarchy_depth = self.framework.query_to_list("""
            MATCH path = (derived:Declaration)-[:INHERITS_FROM*1..3]->(ancestor:Declaration)
            WHERE derived.node_type = 'CXXRecordDecl' AND ancestor.node_type = 'CXXRecordDecl'
            RETURN derived.name as derived, ancestor.name as ancestor, length(path) as depth
            ORDER BY depth DESC, derived
            LIMIT 10
        """)
        
        if hierarchy_depth:
            print("[OK] Inheritance hierarchies found:")
            for hier in hierarchy_depth[:5]:
                print(f"  {hier['derived']} -> {hier['ancestor']} (depth: {hier['depth']})")
        
        # Test pure virtual functions (abstract classes)
        pure_virtual_count = self.framework.query_count("""
            MATCH (method:Declaration)
            WHERE method.node_type = 'CXXMethodDecl' AND method.name CONTAINS 'pure'
            RETURN count(*) as count
        """)
        
        print(f"Found {pure_virtual_count} potential pure virtual functions")
        
        # Test destructors
        destructor_count = self.framework.query_count("""
            MATCH (destructor:Declaration)
            WHERE destructor.node_type = 'CXXDestructorDecl'
            RETURN count(*) as count
        """)
        
        if destructor_count > 0:
            print(f"[OK] Found {destructor_count} destructors")
        
        print("Inheritance analysis completed!")
