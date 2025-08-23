#!/usr/bin/env python3
"""
Test template analysis including specializations and instantiations
"""

from test_framework import BaseTest

class TestTemplatesTest(BaseTest):
    """Test template analysis from test_templates.cpp"""
    
    def run(self):
        """Test template declarations, specializations, and instantiations"""
        
        # Test template declarations exist
        self.framework.assert_query_has_results(
            "MATCH (a:ASTNode), (d:Declaration) WHERE a.node_type CONTAINS 'Template' AND a.node_id = d.node_id RETURN d LIMIT 1",
            "Should have template declarations"
        )
        
        # Test for specific template types
        template_types = [
            "ClassTemplateDecl",
            "FunctionTemplateDecl", 
            "ClassTemplateSpecializationDecl",
            "ClassTemplatePartialSpecializationDecl"
        ]
        
        for template_type in template_types:
            count = self.framework.query_count(f"""
                MATCH (a:ASTNode), (d:Declaration) 
                WHERE a.node_type = '{template_type}' AND a.node_id = d.node_id
                RETURN count(d) as count
            """)
            if count > 0:
                print(f"[OK] Found {count} {template_type} declarations")
            else:
                print(f"Warning: No {template_type} declarations found")
        
        # Test template parameters
        template_param_count = self.framework.query_count(
            "MATCH (n:TemplateParameter) RETURN count(n) as count"
        )
        
        if template_param_count > 0:
            print(f"[OK] Found {template_param_count} template parameters")
            
            # Test parameter kinds
            param_kinds = self.framework.query_to_list("""
                MATCH (p:TemplateParameter)
                RETURN DISTINCT p.parameter_kind as kind, count(*) as count
                ORDER BY count DESC
            """)
            
            for param in param_kinds:
                print(f"  {param['kind']}: {param['count']} parameters")
        else:
            print("Warning: No template parameters detected")
        
        # Test specific templates from test_templates.cpp
        expected_templates = ["FixedArray", "max", "multiply", "Container"]
        
        for template_name in expected_templates:
            count = self.framework.query_count(f"""
                MATCH (a:ASTNode), (d:Declaration) 
                WHERE a.node_type CONTAINS 'Template' AND a.node_id = d.node_id AND d.name = '{template_name}'
                RETURN count(d) as count
            """)
            if count > 0:
                print(f"[OK] Found template {template_name}")
            else:
                print(f"Warning: Template {template_name} not found")
        
        # Test template specialization relationships
        specialization_count = self.framework.query_count(
            "MATCH (spec:Declaration)-[:SPECIALIZES]->(template:Declaration) RETURN count(*) as count"
        )
        
        if specialization_count > 0:
            print(f"[OK] Found {specialization_count} template specializations")
            
            # Get examples of specializations
            specializations = self.framework.query_to_list("""
                MATCH (spec:Declaration)-[r:SPECIALIZES]->(template:Declaration)
                RETURN spec.name as spec_name, template.name as template_name, 
                       r.template_arguments as args, r.specialization_kind as kind
                LIMIT 5
            """)
            
            for spec in specializations:
                print(f"  {spec['spec_name']} specializes {spec['template_name']} with {spec['args']} ({spec['kind']})")
        else:
            print("Warning: No template specializations detected")
        
        # Test template instantiations
        instantiation_count = self.framework.query_count("""
            MATCH (inst:Declaration)-[r:TEMPLATE_RELATION]->(template:Declaration)
            WHERE r.relation_kind = 'instantiates'
            RETURN count(*) as count
        """)
        
        if instantiation_count > 0:
            print(f"[OK] Found {instantiation_count} template instantiations")
        else:
            print("Warning: No template instantiations detected")
        
        # Test variadic templates
        variadic_count = self.framework.query_count("""
            MATCH (p:TemplateParameter)
            WHERE p.is_parameter_pack = true
            RETURN count(*) as count
        """)
        
        if variadic_count > 0:
            print(f"[OK] Found {variadic_count} variadic template parameters")
        else:
            print("No variadic template parameters detected")
        
        # Test default template arguments
        default_args_count = self.framework.query_count("""
            MATCH (p:TemplateParameter)
            WHERE p.has_default_argument = true
            RETURN count(*) as count
        """)
        
        if default_args_count > 0:
            print(f"[OK] Found {default_args_count} template parameters with default arguments")
            
            # Show some examples
            defaults = self.framework.query_to_list("""
                MATCH (p:TemplateParameter)
                WHERE p.has_default_argument = true AND p.default_argument_text IS NOT NULL
                RETURN p.parameter_name as name, p.default_argument_text as default_value
                LIMIT 3
            """)
            
            for default in defaults:
                print(f"  {default['name']} = {default['default_value']}")
        
        # Test template metaprogramming constructs
        metaprog_count = self.framework.query_count(
            "MATCH (n:TemplateMetaprogramming) RETURN count(n) as count"
        )
        
        if metaprog_count > 0:
            print(f"[OK] Found {metaprog_count} template metaprogramming constructs")
        
        # Test SFINAE and enable_if patterns
        sfinae_count = self.framework.query_count("""
            MATCH (n:Declaration)
            WHERE n.qualified_name CONTAINS 'enable_if' OR n.name CONTAINS 'enable_if'
            RETURN count(n) as count
        """)
        
        if sfinae_count > 0:
            print(f"[OK] Found {sfinae_count} SFINAE/enable_if patterns")
        
        # Test nested template classes
        nested_template_count = self.framework.query_count("""
            MATCH (ao:ASTNode), (ai:ASTNode), (outer:Declaration)-[:PARENT_OF*]->(inner:Declaration)
            WHERE ao.node_id = outer.node_id AND ai.node_id = inner.node_id 
              AND ao.node_type CONTAINS 'Template' AND ai.node_type CONTAINS 'Template'
            RETURN count(DISTINCT inner) as count
        """)
        
        if nested_template_count > 0:
            print(f"[OK] Found {nested_template_count} nested template declarations")
        
        # Test template function overloads
        template_function_overloads = self.framework.query_to_list("""
            MATCH (a:ASTNode), (f:Declaration)
            WHERE a.node_id = f.node_id AND a.node_type = 'FunctionTemplateDecl'
            WITH f.name as func_name, count(*) as overload_count
            WHERE overload_count > 1
            RETURN func_name, overload_count
            ORDER BY overload_count DESC
            LIMIT 5
        """)
        
        if template_function_overloads:
            print("[OK] Template function overloads found:")
            for overload in template_function_overloads:
                print(f"  {overload['func_name']}: {overload['overload_count']} overloads")
        
        # Test auto and decltype usage in templates
        auto_decltype_count = self.framework.query_count("""
            MATCH (n:ASTNode)
            WHERE n.raw_text CONTAINS 'auto' OR n.raw_text CONTAINS 'decltype'
            RETURN count(n) as count
        """)
        
        if auto_decltype_count > 0:
            print(f"[OK] Found {auto_decltype_count} uses of auto/decltype")
        
        # Test constexpr templates
        constexpr_template_count = self.framework.query_count("""
            MATCH (a:ASTNode), (n:Declaration)
            WHERE a.node_id = n.node_id AND a.node_type CONTAINS 'Template' AND (n.raw_text CONTAINS 'constexpr' OR n.name CONTAINS 'constexpr')
            RETURN count(n) as count
        """)
        
        if constexpr_template_count > 0:
            print(f"[OK] Found {constexpr_template_count} constexpr templates")
        
        # Test alias templates
        alias_template_count = self.framework.query_count("""
            MATCH (a:ASTNode), (n:Declaration)
            WHERE a.node_id = n.node_id AND a.node_type = 'TypeAliasTemplateDecl'
            RETURN count(n) as count
        """)
        
        if alias_template_count > 0:
            print(f"[OK] Found {alias_template_count} alias templates")
        
        print("Template analysis completed!")
