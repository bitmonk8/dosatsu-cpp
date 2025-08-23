#!/usr/bin/env python3
"""
Analyze type analysis and type relationships
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from framework import BaseAnalyzer

class TypeAnalysis(BaseAnalyzer):
    """Analyze type analysis across all analyze files"""
    
    def run(self):
        """Analyze type declarations and relationships"""
        
        # Analyze that Type table exists and has entries
        self.framework.assert_query_has_results(
            "MATCH (t:Type) RETURN t LIMIT 1",
            "Type table should have entries"
        )
        
        # Analyze builtin types
        builtin_count = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.is_builtin = true
            RETURN count(t) as count
        """)
        
        if builtin_count > 0:
            print(f"[OK] Found {builtin_count} builtin types")
            
            # Show some builtin types
            builtins = self.framework.query_to_list("""
                MATCH (t:Type)
                WHERE t.is_builtin = true
                RETURN DISTINCT t.type_name as name
                ORDER BY name
                LIMIT 10
            """)
            
            builtin_names = [b['name'] for b in builtins]
            print(f"  Examples: {', '.join(builtin_names[:5])}")
        
        # Analyze user-defined types
        user_defined_count = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.is_builtin = false
            RETURN count(t) as count
        """)
        
        if user_defined_count > 0:
            print(f"[OK] Found {user_defined_count} user-defined types")
        
        # Analyze const and volatile qualifiers
        const_count = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.is_const = true
            RETURN count(t) as count
        """)
        
        volatile_count = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.is_volatile = true
            RETURN count(t) as count
        """)
        
        if const_count > 0:
            print(f"[OK] Found {const_count} const-qualified types")
        if volatile_count > 0:
            print(f"[OK] Found {volatile_count} volatile-qualified types")
        
        # Analyze type sizes
        sized_types = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.size_bytes IS NOT NULL AND t.size_bytes > 0
            RETURN count(t) as count
        """)
        
        if sized_types > 0:
            print(f"[OK] Found {sized_types} types with size information")
            
            # Show size distribution
            size_stats = self.framework.query_to_list("""
                MATCH (t:Type)
                WHERE t.size_bytes IS NOT NULL AND t.size_bytes > 0
                RETURN t.size_bytes as size, count(*) as count
                ORDER BY size
                LIMIT 10
            """)
            
            if size_stats:
                print("  Size distribution:")
                for stat in size_stats:
                    print(f"    {stat['size']} bytes: {stat['count']} types")
        
        # Analyze canonical types
        canonical_count = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.canonical_type IS NOT NULL AND t.canonical_type <> t.type_name
            RETURN count(t) as count
        """)
        
        if canonical_count > 0:
            print(f"[OK] Found {canonical_count} types with different canonical forms")
            
            # Show some examples
            canonical_examples = self.framework.query_to_list("""
                MATCH (t:Type)
                WHERE t.canonical_type IS NOT NULL AND t.canonical_type <> t.type_name
                RETURN t.type_name as type_name, t.canonical_type as canonical
                LIMIT 5
            """)
            
            for example in canonical_examples:
                print(f"    {example['type_name']} -> {example['canonical']}")
        
        # Analyze HAS_TYPE relationships
        type_relations = self.framework.query_count(
            "MATCH (d:Declaration)-[:HAS_TYPE]->(t:Type) RETURN count(*) as count"
        )
        
        if type_relations > 0:
            print(f"[OK] Found {type_relations} declaration-to-type relationships")
            
            # Analyze different type roles
            type_roles = self.framework.query_to_list("""
                MATCH (d:Declaration)-[r:HAS_TYPE]->(t:Type)
                WHERE r.type_role IS NOT NULL
                RETURN DISTINCT r.type_role as role, count(*) as count
                ORDER BY count DESC
                LIMIT 5
            """)
            
            if type_roles:
                print("  Type roles:")
                for role in type_roles:
                    print(f"    {role['role']}: {role['count']} relationships")
        
        # Analyze pointer types
        pointer_types = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.type_name CONTAINS '*'
            RETURN count(t) as count
        """)
        
        if pointer_types > 0:
            print(f"[OK] Found {pointer_types} pointer types")
        
        # Analyze reference types
        reference_types = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.type_name CONTAINS '&'
            RETURN count(t) as count
        """)
        
        if reference_types > 0:
            print(f"[OK] Found {reference_types} reference types")
        
        # Analyze template instantiated types
        template_types = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.type_name CONTAINS '<' AND t.type_name CONTAINS '>'
            RETURN count(t) as count
        """)
        
        if template_types > 0:
            print(f"[OK] Found {template_types} template instantiated types")
            
            # Show some examples
            template_examples = self.framework.query_to_list("""
                MATCH (t:Type)
                WHERE t.type_name CONTAINS '<' AND t.type_name CONTAINS '>'
                RETURN DISTINCT t.type_name as name
                ORDER BY size(name) DESC
                LIMIT 5
            """)
            
            for example in template_examples:
                print(f"    {example['name']}")
        
        # Analyze array types
        array_types = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.type_name CONTAINS '[' AND t.type_name CONTAINS ']'
            RETURN count(t) as count
        """)
        
        if array_types > 0:
            print(f"[OK] Found {array_types} array types")
        
        # Analyze function types
        function_types = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.type_name CONTAINS '(' AND t.type_name CONTAINS ')'
            RETURN count(t) as count
        """)
        
        if function_types > 0:
            print(f"[OK] Found {function_types} function types")
        
        # Analyze class/struct types from our analyze files
        expected_types = [
            "Animal", "Mammal", "Bat", "Shape", "Rectangle", 
            "ExpressionTestClass", "MacroTestClass", "FixedArray"
        ]
        
        for type_name in expected_types:
            count = self.framework.query_count(f"""
                MATCH (t:Type)
                WHERE t.type_name = '{type_name}' OR t.type_name CONTAINS '{type_name}'
                RETURN count(t) as count
            """)
            if count > 0:
                print(f"[OK] Found type {type_name}")
        
        # Analyze namespace-qualified types
        namespaced_types = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.type_name CONTAINS '::'
            RETURN count(t) as count
        """)
        
        if namespaced_types > 0:
            print(f"[OK] Found {namespaced_types} namespace-qualified types")
        
        # Analyze standard library types
        std_types = ["std::string", "std::vector", "std::unique_ptr", "std::function"]
        std_found = 0
        
        for std_type in std_types:
            count = self.framework.query_count(f"""
                MATCH (t:Type)
                WHERE t.type_name CONTAINS '{std_type}'
                RETURN count(t) as count
            """)
            if count > 0:
                std_found += 1
        
        if std_found > 0:
            print(f"[OK] Found {std_found} standard library types")
        
        # Analyze auto and decltype types
        auto_count = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.type_name CONTAINS 'auto'
            RETURN count(t) as count
        """)
        
        decltype_count = self.framework.query_count("""
            MATCH (t:Type)
            WHERE t.type_name CONTAINS 'decltype'
            RETURN count(t) as count
        """)
        
        if auto_count > 0:
            print(f"[OK] Found {auto_count} auto types")
        if decltype_count > 0:
            print(f"[OK] Found {decltype_count} decltype types")
        
        # Analyze most complex types
        complex_types = self.framework.query_to_list("""
            MATCH (t:Type)
            WHERE t.type_name IS NOT NULL
            RETURN t.type_name as name, size(t.type_name) as complexity
            ORDER BY complexity DESC
            LIMIT 5
        """)
        
        if complex_types:
            print("[OK] Most complex types:")
            for ctype in complex_types:
                print(f"    {ctype['name']} (length: {ctype['complexity']})")
        
        # Analyze type usage frequency
        most_used_types = self.framework.query_to_list("""
            MATCH (d:Declaration)-[:HAS_TYPE]->(t:Type)
            WITH t, count(d) as usage_count
            WHERE usage_count > 1
            RETURN t.type_name as type_name, usage_count
            ORDER BY usage_count DESC
            LIMIT 5
        """)
        
        if most_used_types:
            print("[OK] Most frequently used types:")
            for used_type in most_used_types:
                print(f"    {used_type['type_name']}: {used_type['usage_count']} uses")
        
        print("Type analysis completed!")
