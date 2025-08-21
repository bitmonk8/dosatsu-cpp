# Kuzu Database Improvement Suggestions for MakeIndex

## Executive Summary

After conducting a comprehensive functionality review of MakeIndex, I have identified significant opportunities to enhance the generated Kuzu database with more complete AST information. While the current implementation provides a solid foundation with basic AST nodes, declarations, types, and relationships, there are substantial gaps that prevent users from being able to answer all queries about a C++ codebase purely from the database.

## Current Capabilities Analysis

### Strengths of Current Implementation

1. **Basic AST Structure**: Successfully captures fundamental AST node hierarchy with parent-child relationships
2. **Declaration Tracking**: Records named declarations with qualified names, access specifiers, and storage classes
3. **Type System**: Creates type nodes with basic information (builtin, const/volatile qualifiers)
4. **Template Support**: Identifies template declarations and specializations
5. **Scope Tracking**: Maintains namespace and function scope relationships
6. **Reference Tracking**: Links declarations to their usages (calls, references)
7. **Performance Optimization**: Uses batched operations for efficient database writes

### Database Schema Overview

The current schema includes:
- **ASTNode**: Base table for all AST nodes with type, location, and memory address
- **Declaration**: Named declarations with qualified names and context
- **Type**: Type information with qualifiers and categorization
- **Statement/Expression**: Basic statement and expression tracking
- **Relationships**: PARENT_OF, HAS_TYPE, REFERENCES, IN_SCOPE, TEMPLATE_RELATION

## Critical Missing Information

**IMPLEMENTED**: ✅ Function Bodies and Statement Detail enhancement has been completed and is now available in the current codebase. This included enhanced Statement and Expression table schemas with detailed information about control flow, operators, literals, and local variable declarations.


### 1. Inheritance and Class Hierarchy **[HIGH PRIORITY]**

**Current State**: Base classes are mentioned in text dump but not captured in database relationships

**Missing Information**:
- Inheritance relationships (public/private/protected/virtual)
- Virtual function override chains
- Interface implementation tracking
- Multiple inheritance diamond problems
- Virtual base class relationships

**Impact**: Cannot answer queries about:
- Class hierarchies and inheritance graphs
- Polymorphism analysis
- Virtual function dispatch
- Interface compliance

**Suggested Schema**:
```sql
CREATE REL TABLE INHERITS_FROM(
    FROM Declaration TO Declaration,
    inheritance_type STRING,    -- "public", "private", "protected"
    is_virtual BOOLEAN,
    base_access_path STRING
)

CREATE REL TABLE OVERRIDES(
    FROM Declaration TO Declaration,
    override_type STRING,       -- "override", "final", "pure_virtual"
    is_covariant_return BOOLEAN
)
```

### 2. Template System Enhancement **[MEDIUM PRIORITY]**

**Current State**: Basic template declarations are captured but specialization details are limited

**Missing Information**:
- Template parameter constraints (C++20 concepts)
- Template argument deduction details
- Partial specialization relationships
- Template instantiation contexts
- SFINAE failure tracking
- Parameter pack information

**Suggested Enhancements**:
```sql
CREATE NODE TABLE TemplateParameter(
    node_id INT64 PRIMARY KEY,
    parameter_kind STRING,      -- "type", "non_type", "template"
    parameter_name STRING,
    has_default_argument BOOLEAN,
    default_argument_text STRING,
    is_parameter_pack BOOLEAN
)

CREATE REL TABLE SPECIALIZES(
    FROM Declaration TO Declaration,
    specialization_kind STRING, -- "explicit", "partial", "implicit"
    template_arguments STRING,
    instantiation_context STRING
)
```

### 3. Preprocessor and Macro Information **[MEDIUM PRIORITY]**

**Current State**: No preprocessor information is captured

**Missing Information**:
- Macro definitions and their expansions
- Include file relationships and order
- Conditional compilation (#if/#ifdef)
- Pragma directives
- Header guard analysis

**Impact**: Cannot analyze:
- Build dependency graphs
- Macro usage patterns
- Include optimization opportunities
- Platform-specific code paths

### 4. Comments and Documentation **[LOW PRIORITY]**

**Current State**: Comments are not captured

**Missing Information**:
- Function and class documentation comments
- TODO/FIXME/HACK markers
- Doxygen-style documentation
- License headers

**Impact**: Cannot generate:
- API documentation
- Code quality reports
- Developer annotations analysis

### 5. Namespace and Using Declarations **[MEDIUM PRIORITY]**

**Current State**: Basic namespace context is captured but using declarations are incomplete

**Missing Information**:
- Using directives scope and impact
- Namespace aliases resolution
- Argument-dependent lookup (ADL) relationships
- Using declarations for specific symbols

**Suggested Enhancement**:
```sql
CREATE NODE TABLE UsingDeclaration(
    node_id INT64 PRIMARY KEY,
    using_kind STRING,          -- "directive", "declaration", "alias"
    target_name STRING,
    introduces_name STRING,
    scope_impact STRING
)
```

### 6. Memory Management and Resource Analysis **[MEDIUM PRIORITY]**

**Missing Information**:
- new/delete expression tracking
- RAII pattern identification
- Smart pointer usage
- Memory allocation/deallocation pairs
- Resource acquisition and release patterns

### 7. Constant Expression and Compile-Time Evaluation **[LOW PRIORITY]**

**Missing Information**:
- constexpr function evaluation results
- Template metaprogramming evaluation
- Compile-time constant values
- Static assertion information


## Specific Implementation Recommendations

### 1. Implement Inheritance Tracking

```cpp
void KuzuDump::VisitCXXRecordDecl(const CXXRecordDecl* D) {
    int64_t nodeId = createASTNode(D);
    
    // Process base classes
    for (const auto& base : D->bases()) {
        if (const auto* baseDecl = base.getType()->getAsCXXRecordDecl()) {
            int64_t baseNodeId = createASTNode(baseDecl);
            createInheritanceRelation(nodeId, baseNodeId, 
                                    base.getAccessSpecifier(),
                                    base.isVirtual());
        }
    }
}
```
