#!/usr/bin/env python3
"""
Test namespace analysis and using declarations
"""

from test_framework import BaseTest

class TestNamespacesTest(BaseTest):
    """Test namespace analysis from test_namespaces.cpp"""
    
    def run(self):
        """Test namespace declarations and using declarations"""
        
        # Test namespace declarations exist
        self.framework.assert_query_has_results(
            "MATCH (n:Declaration) WHERE n.node_type = 'NamespaceDecl' RETURN n LIMIT 1",
            "Should have namespace declarations"
        )
        
        # Test for specific namespaces from test_namespaces.cpp
        expected_namespaces = [
            "Mathematics", "Geometry", "Advanced", "Statistics", 
            "Graphics", "CustomTypes", "TemplateDemo", "Colors"
        ]
        
        found_namespaces = []
        for namespace in expected_namespaces:
            count = self.framework.query_count(f"""
                MATCH (n:Declaration) 
                WHERE n.node_type = 'NamespaceDecl' AND n.name = '{namespace}'
                RETURN count(n) as count
            """)
            if count > 0:
                found_namespaces.append(namespace)
                print(f"✓ Found namespace {namespace}")
            else:
                print(f"Warning: Namespace {namespace} not found")
        
        print(f"Found {len(found_namespaces)} expected namespaces")
        
        # Test nested namespaces
        nested_namespace_count = self.framework.query_count("""
            MATCH (parent:Declaration)-[:PARENT_OF]->(child:Declaration)
            WHERE parent.node_type = 'NamespaceDecl' AND child.node_type = 'NamespaceDecl'
            RETURN count(*) as count
        """)
        
        if nested_namespace_count > 0:
            print(f"✓ Found {nested_namespace_count} nested namespace relationships")
            
            # Get examples of nested namespaces
            nested_examples = self.framework.query_to_list("""
                MATCH (parent:Declaration)-[:PARENT_OF]->(child:Declaration)
                WHERE parent.node_type = 'NamespaceDecl' AND child.node_type = 'NamespaceDecl'
                RETURN parent.name as parent_ns, child.name as child_ns
                LIMIT 5
            """)
            
            for example in nested_examples:
                print(f"  {example['parent_ns']}::{example['child_ns']}")
        else:
            print("Warning: No nested namespaces detected")
        
        # Test using declarations
        using_decl_count = self.framework.query_count(
            "MATCH (n:UsingDeclaration) RETURN count(n) as count"
        )
        
        if using_decl_count > 0:
            print(f"✓ Found {using_decl_count} using declarations")
            
            # Test different kinds of using declarations
            using_kinds = self.framework.query_to_list("""
                MATCH (u:UsingDeclaration)
                RETURN DISTINCT u.using_kind as kind, count(*) as count
                ORDER BY count DESC
            """)
            
            for kind in using_kinds:
                print(f"  {kind['kind']}: {kind['count']} declarations")
        else:
            print("Warning: No using declarations detected")
        
        # Test namespace context in declarations
        namespace_context_count = self.framework.query_count("""
            MATCH (d:Declaration)
            WHERE d.namespace_context IS NOT NULL AND d.namespace_context <> ''
            RETURN count(d) as count
        """)
        
        if namespace_context_count > 0:
            print(f"✓ Found {namespace_context_count} declarations with namespace context")
            
            # Show some examples
            context_examples = self.framework.query_to_list("""
                MATCH (d:Declaration)
                WHERE d.namespace_context IS NOT NULL AND d.namespace_context <> ''
                RETURN DISTINCT d.namespace_context as context, count(*) as count
                ORDER BY count DESC
                LIMIT 5
            """)
            
            for example in context_examples:
                print(f"  Context '{example['context']}': {example['count']} declarations")
        
        # Test qualified names
        qualified_name_count = self.framework.query_count("""
            MATCH (d:Declaration)
            WHERE d.qualified_name CONTAINS '::'
            RETURN count(d) as count
        """)
        
        if qualified_name_count > 0:
            print(f"✓ Found {qualified_name_count} declarations with qualified names")
            
            # Show examples of qualified names
            qualified_examples = self.framework.query_to_list("""
                MATCH (d:Declaration)
                WHERE d.qualified_name CONTAINS '::'
                RETURN d.qualified_name as qualified_name, d.name as simple_name, d.node_type as type
                ORDER BY length(d.qualified_name) DESC
                LIMIT 5
            """)
            
            for example in qualified_examples:
                print(f"  {example['qualified_name']} ({example['type']})")
        
        # Test anonymous namespaces
        anonymous_ns_count = self.framework.query_count("""
            MATCH (n:Declaration)
            WHERE n.node_type = 'NamespaceDecl' AND (n.name = '' OR n.name IS NULL)
            RETURN count(n) as count
        """)
        
        if anonymous_ns_count > 0:
            print(f"✓ Found {anonymous_ns_count} anonymous namespaces")
        else:
            print("No anonymous namespaces detected")
        
        # Test inline namespaces
        inline_ns_count = self.framework.query_count("""
            MATCH (n:Declaration)
            WHERE n.node_type = 'NamespaceDecl' AND n.raw_text CONTAINS 'inline'
            RETURN count(n) as count
        """)
        
        if inline_ns_count > 0:
            print(f"✓ Found {inline_ns_count} inline namespaces")
        else:
            print("No inline namespaces detected")
        
        # Test scope relationships
        scope_count = self.framework.query_count(
            "MATCH (n:ASTNode)-[:IN_SCOPE]->(scope:Declaration) RETURN count(*) as count"
        )
        
        if scope_count > 0:
            print(f"✓ Found {scope_count} scope relationships")
            
            # Test different scope kinds
            scope_kinds = self.framework.query_to_list("""
                MATCH (n:ASTNode)-[r:IN_SCOPE]->(scope:Declaration)
                RETURN DISTINCT r.scope_kind as kind, count(*) as count
                ORDER BY count DESC
                LIMIT 5
            """)
            
            for kind in scope_kinds:
                print(f"  {kind['kind']} scope: {kind['count']} relationships")
        else:
            print("Warning: No scope relationships detected")
        
        # Test namespace aliases
        namespace_alias_count = self.framework.query_count("""
            MATCH (u:UsingDeclaration)
            WHERE u.using_kind = 'namespace_alias'
            RETURN count(u) as count
        """)
        
        if namespace_alias_count > 0:
            print(f"✓ Found {namespace_alias_count} namespace aliases")
        
        # Test using directives vs using declarations
        using_directive_count = self.framework.query_count("""
            MATCH (u:UsingDeclaration)
            WHERE u.using_kind = 'using_directive'
            RETURN count(u) as count
        """)
        
        using_declaration_count = self.framework.query_count("""
            MATCH (u:UsingDeclaration)
            WHERE u.using_kind = 'using_decl'
            RETURN count(u) as count
        """)
        
        print(f"Using directives: {using_directive_count}")
        print(f"Using declarations: {using_declaration_count}")
        
        # Test functions in namespaces
        namespace_functions = self.framework.query_count("""
            MATCH (f:Declaration)
            WHERE f.node_type = 'FunctionDecl' AND f.namespace_context IS NOT NULL AND f.namespace_context <> ''
            RETURN count(f) as count
        """)
        
        if namespace_functions > 0:
            print(f"✓ Found {namespace_functions} functions in namespaces")
        
        # Test classes in namespaces
        namespace_classes = self.framework.query_count("""
            MATCH (c:Declaration)
            WHERE c.node_type = 'CXXRecordDecl' AND c.namespace_context IS NOT NULL AND c.namespace_context <> ''
            RETURN count(c) as count
        """)
        
        if namespace_classes > 0:
            print(f"✓ Found {namespace_classes} classes in namespaces")
        
        # Test ADL (Argument Dependent Lookup) candidates
        # Look for functions that might be found via ADL
        adl_candidates = self.framework.query_count("""
            MATCH (func:Declaration)-[:PARENT_OF*]-(class:Declaration)
            WHERE func.node_type = 'FunctionDecl' AND class.node_type = 'CXXRecordDecl'
               AND func.namespace_context = class.namespace_context
            RETURN count(DISTINCT func) as count
        """)
        
        if adl_candidates > 0:
            print(f"✓ Found {adl_candidates} potential ADL function candidates")
        
        print("Namespace analysis completed!")
