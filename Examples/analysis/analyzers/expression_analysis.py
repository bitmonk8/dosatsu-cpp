#!/usr/bin/env python3
"""
Analyze expression analysis including operators, literals, and casts
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from framework import BaseAnalyzer

class ExpressionAnalysis(BaseAnalyzer):
    """Analyze expression analysis from test_expressions.cpp"""
    
    def run(self):
        """Analyze expression declarations and operator analysis"""
        
        # Analyze expression declarations exist
        self.framework.assert_query_has_results(
            "MATCH (n:Expression) RETURN n LIMIT 1",
            "Should have expression declarations"
        )
        
        # Analyze different expression kinds
        expected_expression_kinds = [
            "BinaryOperator", "UnaryOperator", "CallExpr", "MemberExpr",
            "DeclRefExpr", "IntegerLiteral", "FloatingLiteral", "StringLiteral",
            "CXXMemberCallExpr", "ImplicitCastExpr", "CStyleCastExpr"
        ]
        
        found_expression_kinds = []
        for expr_kind in expected_expression_kinds:
            count = self.framework.query_count(f"""
                MATCH (e:Expression) 
                WHERE e.expression_kind = '{expr_kind}'
                RETURN count(e) as count
            """)
            if count > 0:
                found_expression_kinds.append(expr_kind)
                print(f"[OK] Found {count} {expr_kind} expressions")
            else:
                print(f"Warning: No {expr_kind} expressions found")
        
        print(f"Found {len(found_expression_kinds)} different expression kinds")
        
        # Analyze value categories
        value_categories = self.framework.query_to_list("""
            MATCH (e:Expression)
            WHERE e.value_category IS NOT NULL AND e.value_category <> ''
            RETURN DISTINCT e.value_category as category, count(*) as count
            ORDER BY count DESC
        """)
        
        if value_categories:
            print("[OK] Value categories found:")
            for category in value_categories:
                print(f"  {category['category']}: {category['count']} expressions")
        
        # Analyze literal values
        literal_count = self.framework.query_count("""
            MATCH (e:Expression)
            WHERE e.literal_value IS NOT NULL AND e.literal_value <> ''
            RETURN count(e) as count
        """)
        
        if literal_count > 0:
            print(f"[OK] Found {literal_count} expressions with literal values")
            
            # Show some examples of literals
            literals = self.framework.query_to_list("""
                MATCH (e:Expression)
                WHERE e.literal_value IS NOT NULL AND e.literal_value <> ''
                RETURN e.expression_kind as kind, e.literal_value as value
                LIMIT 10
            """)
            
            print("  Example literals:")
            for literal in literals[:5]:
                print(f"    {literal['kind']}: {literal['value']}")
        
        # Analyze operator kinds
        operator_kinds = self.framework.query_to_list("""
            MATCH (e:Expression)
            WHERE e.operator_kind IS NOT NULL AND e.operator_kind <> ''
            RETURN DISTINCT e.operator_kind as operator, count(*) as count
            ORDER BY count DESC
        """)
        
        if operator_kinds:
            print("[OK] Operator kinds found:")
            for op in operator_kinds[:10]:  # Show top 10
                print(f"  {op['operator']}: {op['count']} uses")
        
        # Analyze arithmetic operators
        arithmetic_ops = ['+', '-', '*', '/', '%', '++', '--']
        arithmetic_count = 0
        
        for op in arithmetic_ops:
            count = self.framework.query_count(f"""
                MATCH (e:Expression)
                WHERE e.operator_kind = '{op}'
                RETURN count(e) as count
            """)
            arithmetic_count += count
        
        if arithmetic_count > 0:
            print(f"[OK] Found {arithmetic_count} arithmetic operator expressions")
        
        # Analyze comparison operators
        comparison_ops = ['==', '!=', '<', '>', '<=', '>=']
        comparison_count = 0
        
        for op in comparison_ops:
            count = self.framework.query_count(f"""
                MATCH (e:Expression)
                WHERE e.operator_kind = '{op}'
                RETURN count(e) as count
            """)
            comparison_count += count
        
        if comparison_count > 0:
            print(f"[OK] Found {comparison_count} comparison operator expressions")
        
        # Analyze logical operators
        logical_ops = ['&&', '||', '!']
        logical_count = 0
        
        for op in logical_ops:
            count = self.framework.query_count(f"""
                MATCH (e:Expression)
                WHERE e.operator_kind = '{op}'
                RETURN count(e) as count
            """)
            logical_count += count
        
        if logical_count > 0:
            print(f"[OK] Found {logical_count} logical operator expressions")
        
        # Analyze assignment operators
        assignment_ops = ['=', '+=', '-=', '*=', '/=', '%=']
        assignment_count = 0
        
        for op in assignment_ops:
            count = self.framework.query_count(f"""
                MATCH (e:Expression)
                WHERE e.operator_kind = '{op}'
                RETURN count(e) as count
            """)
            assignment_count += count
        
        if assignment_count > 0:
            print(f"[OK] Found {assignment_count} assignment operator expressions")
        
        # Analyze member access expressions
        member_access_count = self.framework.query_count("""
            MATCH (e:Expression)
            WHERE e.expression_kind = 'MemberExpr'
            RETURN count(e) as count
        """)
        
        if member_access_count > 0:
            print(f"[OK] Found {member_access_count} member access expressions")
        
        # Analyze function call expressions
        call_expr_count = self.framework.query_count("""
            MATCH (e:Expression)
            WHERE e.expression_kind IN ['CallExpr', 'CXXMemberCallExpr', 'CXXOperatorCallExpr']
            RETURN count(e) as count
        """)
        
        if call_expr_count > 0:
            print(f"[OK] Found {call_expr_count} function call expressions")
        
        # Analyze cast expressions
        cast_count = self.framework.query_count("""
            MATCH (e:Expression)
            WHERE e.expression_kind CONTAINS 'Cast'
            RETURN count(e) as count
        """)
        
        if cast_count > 0:
            print(f"[OK] Found {cast_count} cast expressions")
            
            # Analyze implicit cast kinds
            implicit_casts = self.framework.query_to_list("""
                MATCH (e:Expression)
                WHERE e.implicit_cast_kind IS NOT NULL AND e.implicit_cast_kind <> ''
                RETURN DISTINCT e.implicit_cast_kind as cast_kind, count(*) as count
                ORDER BY count DESC
                LIMIT 5
            """)
            
            if implicit_casts:
                print("  Implicit cast kinds:")
                for cast in implicit_casts:
                    print(f"    {cast['cast_kind']}: {cast['count']} casts")
        
        # Analyze constexpr expressions
        constexpr_count = self.framework.query_count("""
            MATCH (e:Expression)
            WHERE e.is_constexpr = true
            RETURN count(e) as count
        """)
        
        if constexpr_count > 0:
            print(f"[OK] Found {constexpr_count} constexpr expressions")
        
        # Analyze evaluated expressions
        evaluated_count = self.framework.query_count("""
            MATCH (e:Expression)
            WHERE e.evaluation_result IS NOT NULL AND e.evaluation_result <> ''
            RETURN count(e) as count
        """)
        
        if evaluated_count > 0:
            print(f"[OK] Found {evaluated_count} expressions with evaluation results")
            
            # Show some examples
            evaluations = self.framework.query_to_list("""
                MATCH (e:Expression)
                WHERE e.evaluation_result IS NOT NULL AND e.evaluation_result <> ''
                   AND e.literal_value IS NOT NULL
                RETURN e.literal_value as input, e.evaluation_result as result
                LIMIT 5
            """)
            
            if evaluations:
                print("  Example evaluations:")
                for eval_ex in evaluations:
                    print(f"    {eval_ex['input']} -> {eval_ex['result']}")
        
        # Analyze ConstantExpression nodes
        constant_expr_count = self.framework.query_count(
            "MATCH (c:ConstantExpression) RETURN count(c) as count"
        )
        
        if constant_expr_count > 0:
            print(f"[OK] Found {constant_expr_count} constant expression nodes")
            
            # Analyze constant values
            constant_values = self.framework.query_to_list("""
                MATCH (c:ConstantExpression)
                WHERE c.constant_value IS NOT NULL AND c.constant_value <> ''
                RETURN c.constant_value as value, c.constant_type as type
                LIMIT 5
            """)
            
            if constant_values:
                print("  Example constant values:")
                for const in constant_values:
                    print(f"    {const['value']} ({const['type']})")
        
        # Analyze compile-time evaluation
        compile_time_count = self.framework.query_count("""
            MATCH (c:ConstantExpression)
            WHERE c.is_compile_time_constant = true
            RETURN count(c) as count
        """)
        
        if compile_time_count > 0:
            print(f"[OK] Found {compile_time_count} compile-time constant expressions")
        
        # Analyze HAS_CONSTANT_VALUE relationships
        const_value_relations = self.framework.query_count(
            "MATCH (e:Expression)-[:HAS_CONSTANT_VALUE]->(c:ConstantExpression) RETURN count(*) as count"
        )
        
        if const_value_relations > 0:
            print(f"[OK] Found {const_value_relations} expression-to-constant relationships")
        
        # Analyze different literal types
        literal_types = [
            ("IntegerLiteral", "integer"),
            ("FloatingLiteral", "floating-point"),
            ("StringLiteral", "string"), 
            ("CharacterLiteral", "character"),
            ("CXXBoolLiteralExpr", "boolean")
        ]
        
        for literal_type, description in literal_types:
            count = self.framework.query_count(f"""
                MATCH (e:Expression)
                WHERE e.expression_kind = '{literal_type}'
                RETURN count(e) as count
            """)
            if count > 0:
                print(f"[OK] Found {count} {description} literals")
        
        # Analyze lambda expressions
        lambda_count = self.framework.query_count("""
            MATCH (e:Expression)
            WHERE e.expression_kind = 'LambdaExpr'
            RETURN count(e) as count
        """)
        
        if lambda_count > 0:
            print(f"[OK] Found {lambda_count} lambda expressions")
        
        # Analyze conditional expressions (ternary operator)
        conditional_count = self.framework.query_count("""
            MATCH (e:Expression)
            WHERE e.expression_kind = 'ConditionalOperator'
            RETURN count(e) as count
        """)
        
        if conditional_count > 0:
            print(f"[OK] Found {conditional_count} conditional (ternary) expressions")
        
        # Analyze array subscript expressions
        subscript_count = self.framework.query_count("""
            MATCH (e:Expression)
            WHERE e.expression_kind = 'ArraySubscriptExpr'
            RETURN count(e) as count
        """)
        
        if subscript_count > 0:
            print(f"[OK] Found {subscript_count} array subscript expressions")
        
        # Analyze reference expressions
        ref_count = self.framework.query_count("""
            MATCH (e:Expression)
            WHERE e.expression_kind = 'DeclRefExpr'
            RETURN count(e) as count
        """)
        
        if ref_count > 0:
            print(f"[OK] Found {ref_count} declaration reference expressions")
        
        # Analyze size and type information
        size_info_count = self.framework.query_count("""
            MATCH (e:Expression)-[:HAS_TYPE]->(t:Type)
            WHERE t.size_bytes IS NOT NULL AND t.size_bytes > 0
            RETURN count(e) as count
        """)
        
        if size_info_count > 0:
            print(f"[OK] Found {size_info_count} expressions with type size information")
        
        print("Expression analysis completed!")
