#!/usr/bin/env python3
"""
Analyze preprocessor analysis including macros, includes, and conditional compilation
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from framework import BaseAnalyzer

class PreprocessorAnalysis(BaseAnalyzer):
    """Analyze preprocessor analysis from test_preprocessor.cpp"""
    
    def run(self):
        """Analyze macro definitions, includes, and preprocessor directives"""
        
        # Analyze macro definitions exist
        macro_count = self.framework.query_count(
            "MATCH (m:MacroDefinition) RETURN count(m) as count"
        )
        
        if macro_count > 0:
            print(f"✓ Found {macro_count} macro definitions")
            
            # Analyze different macro types
            function_like_count = self.framework.query_count("""
                MATCH (m:MacroDefinition)
                WHERE m.is_function_like = true
                RETURN count(m) as count
            """)
            
            object_like_count = self.framework.query_count("""
                MATCH (m:MacroDefinition)
                WHERE m.is_function_like = false
                RETURN count(m) as count
            """)
            
            print(f"  Function-like macros: {function_like_count}")
            print(f"  Object-like macros: {object_like_count}")
        else:
            print("Warning: No macro definitions found")
        
        # Analyze specific macros from test_preprocessor.cpp
        expected_macros = [
            "PI", "MAX_SIZE", "DEBUG_MODE", "SQUARE", "MAX", "MIN", 
            "SWAP", "DEBUG_PRINT", "LOG", "STRINGIFY"
        ]
        
        found_macros = []
        for macro_name in expected_macros:
            count = self.framework.query_count(f"""
                MATCH (m:MacroDefinition)
                WHERE m.macro_name = '{macro_name}'
                RETURN count(m) as count
            """)
            if count > 0:
                found_macros.append(macro_name)
                print(f"✓ Found macro {macro_name}")
            else:
                print(f"Warning: Macro {macro_name} not found")
        
        print(f"Found {len(found_macros)}/{len(expected_macros)} expected macros")
        
        # Analyze macro parameters
        macro_params = self.framework.query_to_list("""
            MATCH (m:MacroDefinition)
            WHERE m.parameter_count > 0
            RETURN m.macro_name as name, m.parameter_count as param_count, m.parameter_names as params
            ORDER BY m.parameter_count DESC
            LIMIT 5
        """)
        
        if macro_params:
            print("✓ Function-like macros with parameters:")
            for macro in macro_params:
                print(f"  {macro['name']}({macro['params']}) - {macro['param_count']} parameters")
        
        # Analyze variadic macros
        variadic_count = self.framework.query_count("""
            MATCH (m:MacroDefinition)
            WHERE m.parameter_names CONTAINS '...'
            RETURN count(m) as count
        """)
        
        if variadic_count > 0:
            print(f"✓ Found {variadic_count} variadic macros")
        
        # Analyze builtin macros
        builtin_count = self.framework.query_count("""
            MATCH (m:MacroDefinition)
            WHERE m.is_builtin = true
            RETURN count(m) as count
        """)
        
        if builtin_count > 0:
            print(f"✓ Found {builtin_count} builtin macros")
            
            # Show some examples
            builtins = self.framework.query_to_list("""
                MATCH (m:MacroDefinition)
                WHERE m.is_builtin = true
                RETURN m.macro_name as name
                LIMIT 5
            """)
            
            builtin_names = [b['name'] for b in builtins]
            print(f"  Examples: {', '.join(builtin_names)}")
        
        # Analyze conditional macros
        conditional_count = self.framework.query_count("""
            MATCH (m:MacroDefinition)
            WHERE m.is_conditional = true
            RETURN count(m) as count
        """)
        
        if conditional_count > 0:
            print(f"✓ Found {conditional_count} conditional compilation macros")
        
        # Analyze macro expansions
        expansion_count = self.framework.query_count(
            "MATCH (n:ASTNode)-[:MACRO_EXPANSION]->(m:MacroDefinition) RETURN count(*) as count"
        )
        
        if expansion_count > 0:
            print(f"✓ Found {expansion_count} macro expansion relationships")
            
            # Analyze expansion contexts
            expansion_contexts = self.framework.query_to_list("""
                MATCH (n:ASTNode)-[e:MACRO_EXPANSION]->(m:MacroDefinition)
                WHERE e.expansion_context IS NOT NULL
                RETURN DISTINCT e.expansion_context as context, count(*) as count
                ORDER BY count DESC
                LIMIT 5
            """)
            
            if expansion_contexts:
                print("  Expansion contexts:")
                for context in expansion_contexts:
                    print(f"    {context['context']}: {context['count']} expansions")
        else:
            print("Warning: No macro expansion relationships found")
        
        # Analyze include directives
        include_count = self.framework.query_count(
            "MATCH (i:IncludeDirective) RETURN count(i) as count"
        )
        
        if include_count > 0:
            print(f"✓ Found {include_count} include directives")
            
            # Analyze system vs user includes
            system_includes = self.framework.query_count("""
                MATCH (i:IncludeDirective)
                WHERE i.is_system_include = true
                RETURN count(i) as count
            """)
            
            user_includes = self.framework.query_count("""
                MATCH (i:IncludeDirective)
                WHERE i.is_system_include = false
                RETURN count(i) as count
            """)
            
            print(f"  System includes: {system_includes}")
            print(f"  User includes: {user_includes}")
            
            # Analyze angled vs quoted includes
            angled_includes = self.framework.query_count("""
                MATCH (i:IncludeDirective)
                WHERE i.is_angled = true
                RETURN count(i) as count
            """)
            
            print(f"  Angled includes (<>): {angled_includes}")
            print(f"  Quoted includes (\"\"): {include_count - angled_includes}")
            
            # Show some include paths
            include_paths = self.framework.query_to_list("""
                MATCH (i:IncludeDirective)
                RETURN DISTINCT i.include_path as path
                ORDER BY i.include_path
                LIMIT 10
            """)
            
            if include_paths:
                print("  Example include paths:")
                for path in include_paths[:5]:
                    print(f"    {path['path']}")
        else:
            print("Warning: No include directives found")
        
        # Analyze include relationships
        include_relations = self.framework.query_count(
            "MATCH (n:ASTNode)-[:INCLUDES]->(i:IncludeDirective) RETURN count(*) as count"
        )
        
        if include_relations > 0:
            print(f"✓ Found {include_relations} include relationships")
        
        # Analyze conditional directives
        conditional_directives = self.framework.query_count(
            "MATCH (c:ConditionalDirective) RETURN count(c) as count"
        )
        
        if conditional_directives > 0:
            print(f"✓ Found {conditional_directives} conditional directives")
            
            # Analyze directive types
            directive_types = self.framework.query_to_list("""
                MATCH (c:ConditionalDirective)
                WHERE c.directive_type IS NOT NULL
                RETURN DISTINCT c.directive_type as type, count(*) as count
                ORDER BY count DESC
            """)
            
            if directive_types:
                print("  Directive types:")
                for dtype in directive_types:
                    print(f"    {dtype['type']}: {dtype['count']} directives")
        
        # Analyze pragma directives
        pragma_count = self.framework.query_count(
            "MATCH (p:PragmaDirective) RETURN count(p) as count"
        )
        
        if pragma_count > 0:
            print(f"✓ Found {pragma_count} pragma directives")
            
            # Analyze pragma kinds
            pragma_kinds = self.framework.query_to_list("""
                MATCH (p:PragmaDirective)
                WHERE p.pragma_kind IS NOT NULL
                RETURN DISTINCT p.pragma_kind as kind, count(*) as count
                ORDER BY count DESC
                LIMIT 5
            """)
            
            if pragma_kinds:
                print("  Pragma kinds:")
                for kind in pragma_kinds:
                    print(f"    {kind['kind']}: {kind['count']} pragmas")
        
        # Analyze macro definitions relationships
        definition_relations = self.framework.query_count(
            "MATCH (n:ASTNode)-[:DEFINES]->(m:MacroDefinition) RETURN count(*) as count"
        )
        
        if definition_relations > 0:
            print(f"✓ Found {definition_relations} macro definition relationships")
        
        # Analyze preprocessor token analysis
        
        # Analyze stringification (#) usage
        stringify_usage = self.framework.query_count("""
            MATCH (m:MacroDefinition)
            WHERE m.replacement_text CONTAINS '#'
            RETURN count(m) as count
        """)
        
        if stringify_usage > 0:
            print(f"✓ Found {stringify_usage} macros using stringification (#)")
        
        # Analyze token pasting (##) usage
        paste_usage = self.framework.query_count("""
            MATCH (m:MacroDefinition)
            WHERE m.replacement_text CONTAINS '##'
            RETURN count(m) as count
        """)
        
        if paste_usage > 0:
            print(f"✓ Found {paste_usage} macros using token pasting (##)")
        
        # Analyze multi-line macros
        multiline_count = self.framework.query_count("""
            MATCH (m:MacroDefinition)
            WHERE m.replacement_text CONTAINS '\\\\'
            RETURN count(m) as count
        """)
        
        if multiline_count > 0:
            print(f"✓ Found {multiline_count} multi-line macros")
        
        # Analyze predefined macros
        predefined_macros = ["__FILE__", "__LINE__", "__FUNCTION__", "__DATE__", "__TIME__"]
        predefined_found = 0
        
        for macro in predefined_macros:
            count = self.framework.query_count(f"""
                MATCH (m:MacroDefinition)
                WHERE m.macro_name = '{macro}'
                RETURN count(m) as count
            """)
            if count > 0:
                predefined_found += 1
        
        if predefined_found > 0:
            print(f"✓ Found {predefined_found} predefined macros")
        
        # Analyze include depth
        max_depth = self.framework.query_to_list("""
            MATCH (i:IncludeDirective)
            WHERE i.include_depth IS NOT NULL
            RETURN max(i.include_depth) as max_depth
        """)
        
        if max_depth and max_depth[0]['max_depth'] is not None:
            print(f"✓ Maximum include depth: {max_depth[0]['max_depth']}")
        
        # Analyze macro complexity (parameter count)
        complex_macros = self.framework.query_to_list("""
            MATCH (m:MacroDefinition)
            WHERE m.parameter_count > 2
            RETURN m.macro_name as name, m.parameter_count as params
            ORDER BY m.parameter_count DESC
            LIMIT 3
        """)
        
        if complex_macros:
            print("✓ Most complex macros by parameter count:")
            for macro in complex_macros:
                print(f"  {macro['name']}: {macro['params']} parameters")
        
        print("Preprocessor analysis completed!")
