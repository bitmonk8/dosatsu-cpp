#!/usr/bin/env python3
"""
Analyze namespace analysis and using declarations
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from framework import BaseAnalyzer

class NamespaceAnalysis(BaseAnalyzer):
    """Analyze namespace analysis from test_namespaces.cpp"""
    
    def run(self):
        """Analyze namespace declarations and using declarations"""
        
        # Analyze namespace declarations exist
        self.framework.assert_query_has_results(
            "MATCH (a:ASTNode), (d:Declaration) WHERE a.node_type = 'NamespaceDecl' AND a.node_id = d.node_id RETURN d LIMIT 1",
            "Should have namespace declarations"
        )
        
        # Analyze for specific namespaces from comprehensive_test_no_std.cpp
        expected_namespaces = [
            "TestNamespace"
        ]
        
        found_namespaces = []
        for namespace in expected_namespaces:
            count = self.framework.query_count(f"""
                MATCH (a:ASTNode), (d:Declaration) 
                WHERE a.node_type = 'NamespaceDecl' AND a.node_id = d.node_id AND d.name = '{namespace}'
                RETURN count(d) as count
            """)
            if count > 0:
                found_namespaces.append(namespace)
                print(f"[OK] Found namespace {namespace}")
            else:
                print(f"Warning: Namespace {namespace} not found")
        
        print(f"Found {len(found_namespaces)} expected namespaces")
        
        # Analyze nested namespaces
        nested_namespace_count = self.framework.query_count("""
            MATCH (ap:ASTNode), (pd:Declaration), (ac:ASTNode), (cd:Declaration), 
                  (pd)-[:PARENT_OF]->(cd)
            WHERE ap.node_type = 'NamespaceDecl' AND ap.node_id = pd.node_id 
              AND ac.node_type = 'NamespaceDecl' AND ac.node_id = cd.node_id
            RETURN count(*) as count
        """)
        
        if nested_namespace_count > 0:
            print(f"[OK] Found {nested_namespace_count} nested namespace relationships")
            
            # Get examples of nested namespaces
            nested_examples = self.framework.query_to_list("""
                MATCH (ap:ASTNode), (pd:Declaration), (ac:ASTNode), (cd:Declaration),
                      (pd)-[:PARENT_OF]->(cd)
                WHERE ap.node_type = 'NamespaceDecl' AND ap.node_id = pd.node_id 
                  AND ac.node_type = 'NamespaceDecl' AND ac.node_id = cd.node_id
                RETURN pd.name as parent_ns, cd.name as child_ns
                LIMIT 5
            """)
            
            for example in nested_examples:
                print(f"  {example['parent_ns']}::{example['child_ns']}")
        else:
            print("Warning: No nested namespaces detected")
        
        # Analyze using declarations
        using_decl_count = self.framework.query_count(
            "MATCH (n:UsingDeclaration) RETURN count(n) as count"
        )
        
        if using_decl_count > 0:
            print(f"[OK] Found {using_decl_count} using declarations")
            
            # Analyze different kinds of using declarations
            using_kinds = self.framework.query_to_list("""
                MATCH (u:UsingDeclaration)
                RETURN DISTINCT u.using_kind as kind, count(*) as count
                ORDER BY count DESC
            """)
            
            for kind in using_kinds:
                print(f"  {kind['kind']}: {kind['count']} declarations")
        else:
            print("Warning: No using declarations detected")
        
        # Analyze namespace context in declarations
        namespace_context_count = self.framework.query_count("""
            MATCH (d:Declaration)
            WHERE d.namespace_context IS NOT NULL AND d.namespace_context <> ''
            RETURN count(d) as count
        """)
        
        if namespace_context_count > 0:
            print(f"[OK] Found {namespace_context_count} declarations with namespace context")
            
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
        
        # Analyze qualified names
        qualified_name_count = self.framework.query_count("""
            MATCH (d:Declaration)
            WHERE d.qualified_name CONTAINS '::'
            RETURN count(d) as count
        """)
        
        if qualified_name_count > 0:
            print(f"[OK] Found {qualified_name_count} declarations with qualified names")
            
            # Show examples of qualified names
            qualified_examples = self.framework.query_to_list("""
                MATCH (a:ASTNode), (d:Declaration)
                WHERE d.qualified_name CONTAINS '::' AND a.node_id = d.node_id
                RETURN d.qualified_name as qualified_name, d.name as simple_name, a.node_type as type
                ORDER BY size(d.qualified_name) DESC
                LIMIT 5
            """)
            
            for example in qualified_examples:
                print(f"  {example['qualified_name']} ({example['type']})")
        
        # Analyze anonymous namespaces
        anonymous_ns_count = self.framework.query_count("""
            MATCH (a:ASTNode), (d:Declaration)
            WHERE a.node_type = 'NamespaceDecl' AND a.node_id = d.node_id AND (d.name = '' OR d.name IS NULL)
            RETURN count(d) as count
        """)
        
        if anonymous_ns_count > 0:
            print(f"[OK] Found {anonymous_ns_count} anonymous namespaces")
        else:
            print("No anonymous namespaces detected")
        
        # Analyze inline namespaces
        inline_ns_count = self.framework.query_count("""
            MATCH (a:ASTNode), (d:Declaration)
            WHERE a.node_type = 'NamespaceDecl' AND a.node_id = d.node_id AND a.raw_text CONTAINS 'inline'
            RETURN count(d) as count
        """)
        
        if inline_ns_count > 0:
            print(f"[OK] Found {inline_ns_count} inline namespaces")
        else:
            print("No inline namespaces detected")
        
        # Analyze scope relationships
        scope_count = self.framework.query_count(
            "MATCH (n:ASTNode)-[:IN_SCOPE]->(scope:Declaration) RETURN count(*) as count"
        )
        
        if scope_count > 0:
            print(f"[OK] Found {scope_count} scope relationships")
            
            # Analyze different scope kinds
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
        
        # Analyze namespace aliases
        namespace_alias_count = self.framework.query_count("""
            MATCH (u:UsingDeclaration)
            WHERE u.using_kind = 'namespace_alias'
            RETURN count(u) as count
        """)
        
        if namespace_alias_count > 0:
            print(f"[OK] Found {namespace_alias_count} namespace aliases")
        
        # Analyze using directives vs using declarations
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
        
        # Analyze functions in namespaces
        namespace_functions = self.framework.query_count("""
            MATCH (a:ASTNode), (f:Declaration)
            WHERE a.node_type = 'FunctionDecl' AND a.node_id = f.node_id AND f.namespace_context IS NOT NULL AND f.namespace_context <> ''
            RETURN count(f) as count
        """)
        
        if namespace_functions > 0:
            print(f"[OK] Found {namespace_functions} functions in namespaces")
        
        # Analyze classes in namespaces
        namespace_classes = self.framework.query_count("""
            MATCH (a:ASTNode), (c:Declaration)
            WHERE a.node_type = 'CXXRecordDecl' AND a.node_id = c.node_id AND c.namespace_context IS NOT NULL AND c.namespace_context <> ''
            RETURN count(c) as count
        """)
        
        if namespace_classes > 0:
            print(f"[OK] Found {namespace_classes} classes in namespaces")
        
        # Analyze ADL (Argument Dependent Lookup) candidates
        # Look for functions that might be found via ADL
        adl_candidates = self.framework.query_count("""
            MATCH (af:ASTNode), (func:Declaration), (ac:ASTNode), (class:Declaration),
                  (func)-[:PARENT_OF*]-(class)
            WHERE af.node_type = 'FunctionDecl' AND af.node_id = func.node_id
              AND ac.node_type = 'CXXRecordDecl' AND ac.node_id = class.node_id
              AND func.namespace_context = class.namespace_context
            RETURN count(DISTINCT func) as count
        """)
        
        if adl_candidates > 0:
            print(f"[OK] Found {adl_candidates} potential ADL function candidates")
        
        print("Namespace analysis completed!")
