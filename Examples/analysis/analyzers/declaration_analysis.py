#!/usr/bin/env python3
"""
Analyze declaration analysis including functions, variables, and classes
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from framework import BaseAnalyzer

class DeclarationAnalysis(BaseAnalyzer):
    """Analyze declaration analysis across all analyze files"""
    
    def run(self):
        """Analyze various declaration types and their properties"""
        
        # Analyze that Declaration table exists and has entries
        self.framework.assert_query_has_results(
            "MATCH (d:Declaration) RETURN d LIMIT 1",
            "Declaration table should have entries"
        )
        
        # Analyze different declaration types
        expected_decl_types = [
            "FunctionDecl", "CXXMethodDecl", "CXXConstructorDecl", "CXXDestructorDecl",
            "VarDecl", "ParmVarDecl", "FieldDecl", "CXXRecordDecl", "NamespaceDecl",
            "TypedefDecl", "EnumDecl", "EnumConstantDecl"
        ]
        
        found_decl_types = []
        for decl_type in expected_decl_types:
            count = self.framework.query_count(f"""
                MATCH (a:ASTNode), (d:Declaration) 
                WHERE a.node_type = '{decl_type}' AND a.node_id = d.node_id
                RETURN count(d) as count
            """)
            if count > 0:
                found_decl_types.append(decl_type)
                print(f"[OK] Found {count} {decl_type} declarations")
            else:
                print(f"Warning: No {decl_type} declarations found")
        
        print(f"Found {len(found_decl_types)} different declaration types")
        
        # Analyze access specifiers
        access_specifiers = self.framework.query_to_list("""
            MATCH (d:Declaration)
            WHERE d.access_specifier IS NOT NULL AND d.access_specifier <> 'none'
            RETURN DISTINCT d.access_specifier as access, count(*) as count
            ORDER BY count DESC
        """)
        
        if access_specifiers:
            print("[OK] Access specifiers found:")
            for access in access_specifiers:
                print(f"  {access['access']}: {access['count']} declarations")
        
        # Analyze storage classes
        storage_classes = self.framework.query_to_list("""
            MATCH (d:Declaration)
            WHERE d.storage_class IS NOT NULL AND d.storage_class <> ''
            RETURN DISTINCT d.storage_class as storage, count(*) as count
            ORDER BY count DESC
        """)
        
        if storage_classes:
            print("[OK] Storage classes found:")
            for storage in storage_classes:
                print(f"  {storage['storage']}: {storage['count']} declarations")
        
        # Analyze definitions vs declarations
        definitions = self.framework.query_count("""
            MATCH (d:Declaration)
            WHERE d.is_definition = true
            RETURN count(d) as count
        """)
        
        declarations_only = self.framework.query_count("""
            MATCH (d:Declaration)
            WHERE d.is_definition = false
            RETURN count(d) as count
        """)
        
        print(f"[OK] Definitions: {definitions}")
        print(f"[OK] Declarations only: {declarations_only}")
        
        # Analyze function declarations
        function_count = self.framework.query_count("""
            MATCH (a:ASTNode), (f:Declaration)
            WHERE a.node_id = f.node_id AND a.node_type IN ['FunctionDecl', 'CXXMethodDecl']
            RETURN count(f) as count
        """)
        
        if function_count > 0:
            print(f"[OK] Found {function_count} function declarations")
            
            # Analyze specific functions from our analyze files
            expected_functions = [
                "testInheritance", "testTemplates", "testNamespaces", 
                "testControlFlow", "testExpressions", "testPreprocessor",
                "main", "makeSound", "calculateArea"
            ]
            
            for func_name in expected_functions:
                count = self.framework.query_count(f"""
                    MATCH (a:ASTNode), (f:Declaration)
                    WHERE a.node_id = f.node_id AND a.node_type IN ['FunctionDecl', 'CXXMethodDecl'] 
                       AND f.name = '{func_name}'
                    RETURN count(f) as count
                """)
                if count > 0:
                    print(f"  [OK] Found function {func_name}")
        
        # Analyze variable declarations
        variable_count = self.framework.query_count("""
            MATCH (a:ASTNode), (v:Declaration)
            WHERE a.node_id = v.node_id AND a.node_type IN ['VarDecl', 'ParmVarDecl', 'FieldDecl']
            RETURN count(v) as count
        """)
        
        if variable_count > 0:
            print(f"[OK] Found {variable_count} variable declarations")
            
            # Analyze global vs local variables
            global_vars = self.framework.query_count("""
                MATCH (a:ASTNode), (v:Declaration)
                WHERE a.node_id = v.node_id AND a.node_type = 'VarDecl' AND v.namespace_context IS NOT NULL
                RETURN count(v) as count
            """)
            
            if global_vars > 0:
                print(f"  Global variables: {global_vars}")
        
        # Analyze class declarations
        class_count = self.framework.query_count("""
            MATCH (a:ASTNode), (c:Declaration)
            WHERE a.node_id = c.node_id AND a.node_type = 'CXXRecordDecl'
            RETURN count(c) as count
        """)
        
        if class_count > 0:
            print(f"[OK] Found {class_count} class declarations")
            
            # Analyze specific classes from our analyze files
            expected_classes = [
                "Animal", "Mammal", "Bat", "Shape", "Rectangle",
                "ExpressionTestClass", "MacroTestClass", "FixedArray"
            ]
            
            found_classes = []
            for class_name in expected_classes:
                count = self.framework.query_count(f"""
                    MATCH (a:ASTNode), (c:Declaration)
                    WHERE a.node_id = c.node_id AND a.node_type = 'CXXRecordDecl' AND c.name = '{class_name}'
                    RETURN count(c) as count
                """)
                if count > 0:
                    found_classes.append(class_name)
                    print(f"  [OK] Found class {class_name}")
            
            print(f"  Found {len(found_classes)}/{len(expected_classes)} expected classes")
        
        # Analyze constructors and destructors
        constructor_count = self.framework.query_count("""
            MATCH (a:ASTNode), (c:Declaration)
            WHERE a.node_id = c.node_id AND a.node_type = 'CXXConstructorDecl'
            RETURN count(c) as count
        """)
        
        destructor_count = self.framework.query_count("""
            MATCH (a:ASTNode), (d:Declaration)
            WHERE a.node_id = d.node_id AND a.node_type = 'CXXDestructorDecl'
            RETURN count(d) as count
        """)
        
        if constructor_count > 0:
            print(f"[OK] Found {constructor_count} constructors")
        if destructor_count > 0:
            print(f"[OK] Found {destructor_count} destructors")
        
        # Analyze namespace declarations
        namespace_count = self.framework.query_count("""
            MATCH (a:ASTNode), (n:Declaration)
            WHERE a.node_id = n.node_id AND a.node_type = 'NamespaceDecl'
            RETURN count(n) as count
        """)
        
        if namespace_count > 0:
            print(f"[OK] Found {namespace_count} namespace declarations")
        
        # Analyze qualified names
        qualified_names = self.framework.query_count("""
            MATCH (d:Declaration)
            WHERE d.qualified_name IS NOT NULL AND d.qualified_name CONTAINS '::'
            RETURN count(d) as count
        """)
        
        if qualified_names > 0:
            print(f"[OK] Found {qualified_names} declarations with qualified names")
            
            # Show longest qualified names
            long_names = self.framework.query_to_list("""
                MATCH (d:Declaration)
                WHERE d.qualified_name IS NOT NULL AND d.qualified_name CONTAINS '::'
                RETURN d.qualified_name as name, size(d.qualified_name) as length
                ORDER BY length DESC
                LIMIT 3
            """)
            
            if long_names:
                print("  Longest qualified names:")
                for name in long_names:
                    print(f"    {name['name']} (length: {name['length']})")
        
        # Analyze template declarations
        template_decl_count = self.framework.query_count("""
            MATCH (a:ASTNode), (t:Declaration)
            WHERE a.node_id = t.node_id AND a.node_type CONTAINS 'Template'
            RETURN count(t) as count
        """)
        
        if template_decl_count > 0:
            print(f"[OK] Found {template_decl_count} template-related declarations")
        
        # Analyze enum declarations
        enum_count = self.framework.query_count("""
            MATCH (a:ASTNode), (e:Declaration)
            WHERE a.node_id = e.node_id AND a.node_type = 'EnumDecl'
            RETURN count(e) as count
        """)
        
        enum_constant_count = self.framework.query_count("""
            MATCH (a:ASTNode), (ec:Declaration)
            WHERE a.node_id = ec.node_id AND a.node_type = 'EnumConstantDecl'
            RETURN count(ec) as count
        """)
        
        if enum_count > 0:
            print(f"[OK] Found {enum_count} enum declarations")
        if enum_constant_count > 0:
            print(f"[OK] Found {enum_constant_count} enum constant declarations")
        
        # Analyze typedef declarations
        typedef_count = self.framework.query_count("""
            MATCH (a:ASTNode), (t:Declaration)
            WHERE a.node_id = t.node_id AND a.node_type IN ['TypedefDecl', 'TypeAliasDecl']
            RETURN count(t) as count
        """)
        
        if typedef_count > 0:
            print(f"[OK] Found {typedef_count} typedef/type alias declarations")
        
        # Analyze static members
        static_count = self.framework.query_count("""
            MATCH (s:Declaration)
            WHERE s.storage_class = 'static'
            RETURN count(s) as count
        """)
        
        if static_count > 0:
            print(f"[OK] Found {static_count} static declarations")
        
        # Analyze inline functions
        inline_count = self.framework.query_count("""
            MATCH (a:ASTNode), (i:Declaration)
            WHERE a.node_id = i.node_id AND a.node_type IN ['FunctionDecl', 'CXXMethodDecl'] 
               AND (i.storage_class CONTAINS 'inline' OR a.raw_text CONTAINS 'inline')
            RETURN count(i) as count
        """)
        
        if inline_count > 0:
            print(f"[OK] Found {inline_count} inline function declarations")
        
        # Analyze virtual functions
        virtual_count = self.framework.query_count("""
            MATCH (a:ASTNode), (v:Declaration)
            WHERE a.node_id = v.node_id AND a.node_type = 'CXXMethodDecl' AND a.raw_text CONTAINS 'virtual'
            RETURN count(v) as count
        """)
        
        if virtual_count > 0:
            print(f"[OK] Found {virtual_count} virtual function declarations")
        
        # Analyze pure virtual functions
        pure_virtual_count = self.framework.query_count("""
            MATCH (a:ASTNode), (pv:Declaration)
            WHERE a.node_id = pv.node_id AND a.node_type = 'CXXMethodDecl' AND a.raw_text CONTAINS '= 0'
            RETURN count(pv) as count
        """)
        
        if pure_virtual_count > 0:
            print(f"[OK] Found {pure_virtual_count} pure virtual function declarations")
        
        # Analyze friend declarations
        friend_count = self.framework.query_count("""
            MATCH (a:ASTNode), (f:Declaration)
            WHERE a.node_id = f.node_id AND a.node_type = 'FriendDecl'
            RETURN count(f) as count
        """)
        
        if friend_count > 0:
            print(f"[OK] Found {friend_count} friend declarations")
        
        # Analyze using declarations
        using_count = self.framework.query_count("""
            MATCH (a:ASTNode), (u:Declaration)
            WHERE a.node_id = u.node_id AND a.node_type = 'UsingDecl'
            RETURN count(u) as count
        """)
        
        if using_count > 0:
            print(f"[OK] Found {using_count} using declarations")
        
        # Analyze most common declaration names
        common_names = self.framework.query_to_list("""
            MATCH (d:Declaration)
            WHERE d.name IS NOT NULL AND d.name <> ''
            WITH d.name as name, count(*) as usage_count
            WHERE usage_count > 1
            RETURN name, usage_count
            ORDER BY usage_count DESC
            LIMIT 5
        """)
        
        if common_names:
            print("[OK] Most common declaration names:")
            for name in common_names:
                print(f"  {name['name']}: {name['usage_count']} declarations")
        
        # Analyze namespace contexts
        namespace_contexts = self.framework.query_to_list("""
            MATCH (d:Declaration)
            WHERE d.namespace_context IS NOT NULL AND d.namespace_context <> ''
            RETURN DISTINCT d.namespace_context as context, count(*) as count
            ORDER BY count DESC
            LIMIT 5
        """)
        
        if namespace_contexts:
            print("[OK] Most common namespace contexts:")
            for context in namespace_contexts:
                print(f"  {context['context']}: {context['count']} declarations")
        
        print("Declaration analysis completed!")
