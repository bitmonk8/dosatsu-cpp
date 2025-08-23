# CppGraphIndex Analysis Tools

This directory contains Python tools for analyzing and verifying the output of CppGraphIndex when processing C++ examples. These tools ensure that the indexing system correctly captures and represents C++ language constructs.

## Overview

The analysis suite provides:

1. **Automated Verification** - Ensures CppGraphIndex correctly analyzes C++ code
2. **Database Inspection** - Tools to examine the generated graph database
3. **Regression Testing** - Detects when changes break existing functionality
4. **Feature Validation** - Verifies new language features are properly supported

## Directory Structure

```
analysis/
├── README.md                    # This file
├── run_analysis.py             # Main analysis runner
├── framework.py                # Core analysis framework
├── __init__.py                 # Python package initialization
└── analyzers/                  # Individual analysis modules
    ├── __init__.py
    ├── ast_analysis.py         # AST node structure verification
    ├── inheritance_analysis.py # Inheritance relationship analysis
    ├── template_analysis.py    # Template analysis verification
    ├── namespace_analysis.py   # Namespace scope analysis
    ├── control_flow_analysis.py # Control flow verification
    ├── expression_analysis.py  # Expression analysis verification
    ├── preprocessor_analysis.py # Preprocessor directive analysis
    ├── declaration_analysis.py # Declaration analysis verification
    ├── statement_analysis.py   # Statement analysis verification
    └── type_analysis.py        # Type system verification
```

## Quick Start

### Running All Analyzers

```bash
# From the analysis directory
python run_analysis.py

# From project root
python Examples/analysis/run_analysis.py
```

### Running Individual Analyzers

```python
from framework import AnalysisFramework
from analyzers.inheritance_analysis import InheritanceAnalysis

# Setup framework
framework = AnalysisFramework()
framework.setup_example_database()

# Run specific analyzer
analyzer = InheritanceAnalysis(framework)
success = analyzer.execute()

# Cleanup
framework.cleanup()
```

## Core Components

### `framework.py` - Analysis Framework

The `AnalysisFramework` class provides:

- **Database Management**: Creates temporary databases from C++ examples
- **Query Interface**: Simplified Cypher query execution
- **Assertion Methods**: Verification utilities for common patterns
- **Cleanup**: Automatic resource management

Key methods:
```python
framework = AnalysisFramework()
framework.setup_example_database()  # Creates DB from examples
framework.assert_query_has_results(query, description)
framework.assert_query_count(query, expected_count, description)
framework.query_to_list(query)  # Returns results as list
framework.cleanup()  # Cleans up temporary resources
```

### `BaseAnalyzer` - Analyzer Base Class

All analyzers inherit from `BaseAnalyzer`:

```python
class MyAnalyzer(BaseAnalyzer):
    def run(self):
        """Implement analysis logic here"""
        self.framework.assert_query_has_results(
            "MATCH (n:MyNode) RETURN n",
            "Should find MyNode instances"
        )
        return True  # Success
```

### `run_analysis.py` - Main Runner

Orchestrates the complete analysis process:

1. Sets up the analysis framework
2. Creates database from C++ examples
3. Runs all available analyzers
4. Reports results and statistics
5. Cleans up resources

## Analysis Modules

### AST Analysis (`ast_analysis.py`)
- Verifies basic AST node structure
- Checks required fields and relationships
- Validates source location information

### Inheritance Analysis (`inheritance_analysis.py`)
- Verifies class inheritance relationships
- Checks virtual function overrides
- Validates access specifiers and visibility

### Template Analysis (`template_analysis.py`)
- Verifies template declarations and instantiations
- Checks template specializations
- Validates template parameter relationships

### Namespace Analysis (`namespace_analysis.py`)
- Verifies namespace declarations and usage
- Checks scope resolution and qualified names
- Validates using declarations and directives

### Control Flow Analysis (`control_flow_analysis.py`)
- Verifies control flow graph construction
- Checks loop and conditional structures
- Validates exception handling constructs

### Expression Analysis (`expression_analysis.py`)
- Verifies expression parsing and representation
- Checks operator precedence and associativity
- Validates type conversions and casts

### Preprocessor Analysis (`preprocessor_analysis.py`)
- Verifies macro definitions and expansions
- Checks conditional compilation directives
- Validates include processing

### Declaration Analysis (`declaration_analysis.py`)
- Verifies variable and function declarations
- Checks declaration contexts and scopes
- Validates forward declarations

### Statement Analysis (`statement_analysis.py`)
- Verifies statement parsing and classification
- Checks compound statements and blocks
- Validates statement relationships

### Type Analysis (`type_analysis.py`)
- Verifies type system representation
- Checks type qualifiers and modifiers
- Validates type relationships and conversions

## Database Schema Verification

The analyzers verify that CppGraphIndex correctly implements the database schema defined in `DB_SCHEMA.md`. Key aspects:

### Node Types
- **ASTNode**: Base node for all AST elements
- **Declaration**: Function, variable, class declarations
- **Statement**: Control flow and expression statements
- **Expression**: All expression types and operators
- **Type**: Type information and relationships

### Relationships
- **PARENT_OF**: AST hierarchy relationships
- **DECLARES**: Declaration relationships
- **USES**: Usage and reference relationships
- **INHERITS_FROM**: Class inheritance
- **SPECIALIZES**: Template specializations

### Properties
- **Source Locations**: File, line, column information
- **Names**: Qualified and unqualified names
- **Types**: Type information and qualifiers
- **Attributes**: Language-specific attributes

## Writing New Analyzers

### Basic Structure

```python
from ..framework import BaseAnalyzer

class MyFeatureAnalysis(BaseAnalyzer):
    """Analyze specific C++ feature"""
    
    def run(self):
        """Implement analysis logic"""
        
        # Check that feature nodes exist
        self.framework.assert_query_has_results(
            "MATCH (n:MyFeature) RETURN n LIMIT 1",
            "Should find MyFeature nodes"
        )
        
        # Verify specific properties
        results = self.framework.query_to_list(
            "MATCH (n:MyFeature) RETURN n.property"
        )
        
        for result in results:
            assert result['n.property'] is not None
        
        return True
```

### Best Practices

1. **Descriptive Assertions**: Use clear descriptions for all assertions
2. **Comprehensive Coverage**: Test both positive and negative cases
3. **Error Handling**: Handle missing data gracefully
4. **Performance**: Use efficient queries for large datasets
5. **Documentation**: Document what each analyzer verifies

### Integration

1. Add the analyzer to `analyzers/` directory
2. Import in `framework.py` analyzer list
3. Add comprehensive docstrings
4. Test with various C++ examples
5. Update this README

## Troubleshooting

### Common Issues

**Database Creation Fails**
- Ensure CppGraphIndex is built (`python please.py build`)
- Check that compilation commands are valid
- Verify C++ examples compile correctly

**Query Failures**
- Check Cypher query syntax
- Verify node and relationship names match schema
- Use `framework.print_database_summary()` for debugging

**Missing Results**
- Verify C++ examples contain expected constructs
- Check that compilation includes all necessary files
- Review CppGraphIndex analysis logs

### Debugging Tools

```python
# Print database statistics
framework.print_database_summary()

# Execute raw queries
results = framework.conn.execute("MATCH (n) RETURN labels(n), count(*)")

# Get sample nodes
framework.get_sample_nodes("ASTNode", limit=5)
```

## Performance Considerations

- **Database Size**: Comprehensive examples create large databases
- **Query Complexity**: Complex queries may be slow on large datasets
- **Memory Usage**: Framework creates temporary databases in memory
- **Cleanup**: Always call `framework.cleanup()` to free resources

## Integration with CI/CD

These analysis tools are integrated with the main project's test suite:

1. **Automated Testing**: Run on every commit and pull request
2. **Regression Detection**: Catch breaking changes early
3. **Performance Monitoring**: Track analysis speed and accuracy
4. **Quality Gates**: Prevent merging code that breaks analysis
