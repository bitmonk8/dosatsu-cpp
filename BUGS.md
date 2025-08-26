# Known Issues in Dosatsu

This document describes known technical issues in the Dosatsu project that require investigation.

---

## Issue #1: Kuzu Query Generation Bug - Unescaped Strings in Declaration Names

**Problem**: The `DeclarationAnalyzer::createDeclarationNode()` function generates invalid Kuzu database queries when processing declarations with complex names containing special characters (backslashes, parentheses, quotes).

**Error Message**:
```
Batched query failed: Parser exception: Invalid input <CREATE (d:Declaration {node_id: 795, name: '>: expected rule oC_SingleQuery (line: 1, offset: 43)
"CREATE (d:Declaration {node_id: 795, name: '(unnamed struct at C:\UnitySrc\CppGraphIndex\Examples\cpp\modern_cpp_features\modern_cpp_features.cpp:490:5)', qualified_name: 'demonstrate_structured_bindings()::(anonymous struct)::(unnamed struct at C:\\UnitySrc\\CppGraphIndex\\Examples\\cpp\\modern_cpp_features\\modern_cpp_features.cpp:490:5)', access_specifier: 'public', storage_class: 'none', is_definition: true, namespace_context: ''})"
                                            ^
```

**Root Cause**:
1. **Location**: `source/DeclarationAnalyzer.cpp` lines 50-53 in `createDeclarationNode()`
2. **Issue**: The `name` field uses `decl->getNameAsString()` directly without escaping, while `qualifiedName` and `namespaceContext` are properly escaped through helper functions.
3. **Inconsistency**: Some fields are escaped (`qualified_name` via `extractQualifiedName()`) while others are not (`name`, `access_specifier`, `storage_class`).

**Affected Code**:
```cpp
// Line 42: Raw name extraction (NOT escaped)
std::string name = decl->getNameAsString();

// Line 43: Properly escaped qualified name
std::string qualifiedName = extractQualifiedName(decl);

// Lines 50-53: Query generation with mixed escaped/unescaped fields
std::string query = "CREATE (d:Declaration {node_id: " + std::to_string(nodeId) + ", name: '" + name +
                    "', qualified_name: '" + qualifiedName + "', access_specifier: '" + accessSpec +
                    "', storage_class: '" + storageClass + "', is_definition: " + (isDef ? "true" : "false") +
                    ", namespace_context: '" + namespaceContext + "'})";
```

**Problematic Names**: Anonymous/unnamed structs from C++11+ structured bindings generate names like:
- `(unnamed struct at C:\path\file.cpp:line:column)`
- Contains unescaped backslashes and parentheses that break Kuzu query parsing

**Example Trigger**: The `modern_cpp_features.cpp` example at line 490:
```cpp
struct { int first; const char* second; } p{100, "world"};
```

**Impact**: 
- Causes indexing failures for any C++ code with anonymous structures
- Affects modern C++ features like structured bindings
- Creates inconsistent database state (some records created, others fail)
- Verification still passes because actual database operations continue after batch failures

**How to Reproduce**:
1. Run `python Examples\run_examples.py --modern_cpp_features`
2. Check log file for "Batched query failed" errors
3. The indexing completes but with errors in stderr

**Related Files**:
- `source/DeclarationAnalyzer.cpp` (main bug location)
- `source/KuzuDatabase.cpp` (contains working `escapeString()` function)
- `Examples/cpp/modern_cpp_features/modern_cpp_features.cpp` (trigger case)

---

## Issue #2: Systemic Database Query Security and Consistency Issues

**Problem**: The codebase lacks a unified, comprehensive approach to secure Kuzu database query generation, creating vulnerabilities, maintenance challenges, and inconsistent patterns that will lead to future bugs as the project scales.

**Core Issues Requiring Systematic Fixes**:

**1. Scope of the Problem - ALL Kuzu Queries Need Auditing**:
The security and consistency issues extend to ALL database operations:
- **CREATE queries**: Node and relationship creation
- **MATCH queries**: Search and retrieval operations
- **UPDATE queries**: Data modification operations  
- **DELETE queries**: Data removal operations
- **Schema queries**: Database structure modifications
- **Batch operations**: Multiple query execution

**2. Fundamental Architecture Problems**:

**A. No Unified Query Generation Strategy**:
- String concatenation used throughout (injection-prone)
- Three different escaping approaches creating confusion
- No centralized query building infrastructure
- Ad-hoc query construction in multiple files

**B. Inconsistent Escaping Methods Currently Found**:
- **Method A**: `KuzuDatabase::escapeString()` - Handles backslashes and quotes (RECOMMENDED)
- **Method B**: `std::ranges::replace(str, '\'', '_')` - Only single quotes (INCOMPLETE)  
- **Method C**: No escaping (VULNERABLE)
- **Method D**: Manual character cleaning (INCONSISTENT)

**3. Complete Query Audit Requirements**:
The following query types require systematic review:

**A. All Query Types in KuzuDatabase.cpp**:
- Schema creation queries (`executeSchemaQuery()`)
- Batch execution operations (`executeBatch()`, `addToBatch()`)
- Direct query execution (`executeQuery()`)

**B. All Analyzer Classes**:
- `DeclarationAnalyzer.cpp`: Declaration and using declaration queries
- `StatementAnalyzer.cpp`: Statement and expression queries  
- `TypeAnalyzer.cpp`: Type and relationship queries
- `TemplateAnalyzer.cpp`: Template parameter and relationship queries
- `AdvancedAnalyzer.cpp`: Macro, assertion, and CFG queries
- `ASTNodeProcessor.cpp`: Base AST node creation queries

**C. Relationship and Reference Queries**:
- PARENT_OF relationships
- HAS_TYPE relationships  
- REFERENCES relationships
- IN_SCOPE relationships
- TEMPLATE_RELATION relationships
- INHERITS_FROM relationships
- All other relationship types

**CREATE Query Status**:

| File | Function | Field | Escaping Method | Status |
|------|----------|-------|----------------|---------|
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `name` | None | Requires fix |
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `access_specifier` | None | Requires fix |
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `storage_class` | None | Requires fix |
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `qualified_name` | `escapeString()` | ✅ Complete |
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `namespace_context` | None | Requires fix |
| `StatementAnalyzer.cpp` | `createStatementNode()` | `statement_kind` | None | Requires fix |
| `StatementAnalyzer.cpp` | `createStatementNode()` | `control_flow_type` | None | Requires fix |
| `StatementAnalyzer.cpp` | `createStatementNode()` | `condition_text` | `escapeString()` | ✅ Complete |

*Note: All fields that accept dynamic content require consistent escaping.

**Impact**:
- **PRIMARY**: Anonymous struct names with Windows paths cause immediate parsing failures
- **SECONDARY**: Inconsistent code patterns make maintenance difficult
- **SCOPE**: Current C++ language features already create problematic characters requiring fixes

**Systemic Issues**:
1. **No centralized escaping policy**: Different developers use different approaches
2. **Missing documentation**: No clear guidelines on when/how to escape strings
3. **Partial validation**: Some fields escaped, others not, creating security gaps

**How to Reproduce**:
1. Any C++ code with anonymous structs triggers parsing failures
2. Structured bindings, lambdas, and template instantiations commonly create complex names
3. Modern C++ features are most likely to trigger edge cases

**IMPLEMENTATION PLAN**:

**Phase 1: Systematic Root Cause Fix**
- **Task 1.1**: Complete audit of ALL query generation locations (CREATE, MATCH, relationships, etc.)
- **Task 1.2**: Apply systematic escaping pattern to all identified fields:
  ```cpp
  // SYSTEMATIC PATTERN: std::string query = "CREATE (...{field: '" + KuzuDatabase::escapeString(rawValue) + "'...)"
  ```
- **Task 1.3**: Create query generation pattern documentation and code review checklist
- **Completion Criteria**: All query generation uses consistent escaping patterns

**Phase 2: Verification Using Known Issues**
- **Task 2.1**: Test that anonymous struct parsing now works (`modern_cpp_features.cpp`)
- **Task 2.2**: Verify all known problematic fields no longer cause failures
- **Task 2.3**: Run comprehensive validation: `python Examples\run_examples.py --all`
- **Completion Criteria**: Zero parsing failures, proving root cause fix is complete

**IMPLEMENTATION MATRIX**:

| File | Function | Field | Status | Action Required |
|------|----------|-------|--------|-----------------|
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `name` | ❌ No escaping | Add escapeString() |
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `access_specifier` | ❌ No escaping | Add escapeString() |
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `storage_class` | ❌ No escaping | Add escapeString() |  
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `namespace_context` | ❌ No escaping | Add escapeString() |
| [**REQUIRES COMPLETION**] | - | - | - | Audit remaining 13+ queries |

*Note: These fields can contain user-defined content in template instantiations or complex declarations.

**TECHNICAL REQUIREMENTS**:

**Implementation Scope**:
- **Files to modify**: 9 source files containing query generation
- **Query patterns**: 17+ CREATE queries, plus MATCH/relationship queries  
- **Testing**: Use `Examples/cpp/modern_cpp_features/modern_cpp_features.cpp` for regression testing
- **Validation**: All queries must pass `python Examples\run_examples.py --all` without parsing errors

**Completion Criteria**:
1. **Systematic fix verification**: All string interpolations in query generation use `escapeString()`
2. **Known issue resolution**: Anonymous struct test case passes (modern_cpp_features.cpp)
3. **Comprehensive validation**: `python Examples\run_examples.py --all` completes without "Batched query failed" errors
4. **Code consistency**: Single escaping pattern across all analyzers

**Evidence of Success**:
1. ✅ **Build verification**: `python please.py build && python please.py test` passes
2. ✅ **End-to-end verification**: All 13 workflows in `--all` complete successfully  
3. ✅ **Specific test case**: Modern C++ features with anonymous structs index without errors
4. ✅ **Security audit**: No unescaped string interpolations remain in codebase

---

*This document tracks technical issues requiring investigation. For examples-specific issues, see `EXAMPLE_BUGS.md`.*