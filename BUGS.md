# Known Issues in Dosatsu

This document describes known technical issues in the Dosatsu project that require investigation.

---

## Issue #1: `--all` Flag Does Not Actually Run All Examples

**Problem**: The `python Examples\run_examples.py --all` command does not process all available example categories as the name suggests.

**Expected Behavior**: The `--all` flag should process examples from all available categories:
- `basic` (Examples/cpp/basic/): 10 C++ files
- `comprehensive` (Examples/cpp/comprehensive/): 11 C++ files  
- `nostd` (Examples/cpp/nostd/): 4 C++ files

**Actual Behavior**: The `--all` flag only processes the `nostd` category examples.

**Root Cause**: 
1. In `Examples/run_examples.py` line 699, the `--all` flag is hardcoded to only process `"comprehensive_no_std_compile_commands.json"` which maps to the `nostd` category.
2. The verification queries in `Examples/queries/database_operations.py` line 163 are also hardcoded to use `nostd_cmake_compile_commands.json`.

**How to Reproduce**:
1. Run `python Examples\run_examples.py --all`
2. Observe that only 4 files from the `nostd` category are processed:
   - `advanced_features_no_std.cpp`
   - `simple_no_includes.cpp` 
   - `inheritance_no_std.cpp`
   - `no_std_example.cpp`
3. The 21 files from `basic` and `comprehensive` categories are completely ignored.

**Impact**: Users expecting comprehensive testing of all example files will miss potential issues in the `basic` and `comprehensive` example sets.

---

## Issue #2: Kuzu Query Generation Bug - Unescaped Strings in Declaration Names

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

## Issue #3: Systemic Database Query Security and Consistency Issues

**Problem**: The codebase lacks a unified, comprehensive approach to secure Kuzu database query generation, creating vulnerabilities, maintenance challenges, and inconsistent patterns that will lead to future bugs as the project scales.

**Strategic Issues Requiring Long-Term Architecture Changes**:

**1. Scope of the Problem - ALL Kuzu Queries Need Auditing**:
The current analysis focused on CREATE queries, but the security and consistency issues extend to ALL database operations:
- **CREATE queries**: Node and relationship creation (currently analyzed)
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

**B. Lack of Developer Guidance Systems**:
- No compile-time safety mechanisms
- No coding standards documentation for database queries
- No code review checklists for database interactions
- No automated testing for query injection vulnerabilities

**C. Inconsistent Escaping Methods Currently Found**:
- **Method A**: `KuzuDatabase::escapeString()` - Handles backslashes and quotes (RECOMMENDED)
- **Method B**: `std::ranges::replace(str, '\'', '_')` - Only single quotes (INCOMPLETE)  
- **Method C**: No escaping (VULNERABLE)
- **Method D**: Manual character cleaning (INCONSISTENT)

**3. Comprehensive Query Audit Requirements**:
Beyond the CREATE queries already analyzed, the following need systematic review:

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

**Current CREATE Query Vulnerability Assessment** (Partial Analysis):

| File | Function | Field | Escaping Method | Risk Level |
|------|----------|-------|----------------|------------|
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `name` | None | **HIGH** |
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `access_specifier` | None | LOW* |
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `storage_class` | None | LOW* |
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `qualified_name` | `escapeString()` | Safe |
| `DeclarationAnalyzer.cpp` | `createDeclarationNode()` | `namespace_context` | None | MEDIUM* |
| `StatementAnalyzer.cpp` | `createStatementNode()` | `statement_kind` | None | LOW* |
| `StatementAnalyzer.cpp` | `createStatementNode()` | `control_flow_type` | None | LOW* |
| `StatementAnalyzer.cpp` | `createStatementNode()` | `condition_text` | `escapeString()` | Safe |

*LOW/MEDIUM risk: These fields typically contain predictable values, but could theoretically contain user-defined content in edge cases.

**Impact**:
- **PRIMARY**: Anonymous struct names with Windows paths cause immediate parsing failures
- **SECONDARY**: Inconsistent code patterns make maintenance difficult
- **POTENTIAL**: Future C++ language features might introduce new problematic characters

**Systemic Issues**:
1. **No centralized escaping policy**: Different developers use different approaches
2. **Missing documentation**: No clear guidelines on when/how to escape strings
3. **Partial validation**: Some fields escaped, others not, creating security gaps

**How to Reproduce**:
1. Any C++ code with anonymous structs triggers the HIGH risk issue
2. Structured bindings, lambdas, and template instantiations commonly create complex names
3. Modern C++ features are most likely to trigger edge cases

**Long-Term Strategic Solution Architecture**:

**Phase 1: Comprehensive Security Audit (Foundation)**
1. **Complete Query Inventory**: Systematically catalog ALL Kuzu queries across the entire codebase
2. **Security Assessment**: Evaluate each query type for injection vulnerabilities
3. **Pattern Analysis**: Document all current query generation approaches and their risks
4. **Priority Matrix**: Rank vulnerabilities by risk level and implementation effort

**Phase 2: Unified Query Infrastructure (Core Architecture)**
1. **Centralized Query Builder**: Design a type-safe query building system
   - Template-based parameter binding
   - Automatic escaping for all string values
   - Compile-time query validation where possible
   - Support for all Kuzu query types (CREATE, MATCH, UPDATE, DELETE, etc.)

2. **Standardized Escaping System**: 
   - Enhance `KuzuDatabase::escapeString()` to handle all edge cases
   - Create specialized escaping functions for different data types
   - Implement comprehensive unit tests for escaping edge cases
   - Document escaping requirements for all Kuzu data types

3. **Developer Safety Infrastructure**:
   - Deprecate direct string concatenation for queries
   - Create compile-time warnings/errors for unsafe query patterns
   - Implement automated static analysis to detect unescaped queries
   - Establish clear coding standards with examples

**Phase 3: Developer Education and Enforcement (Culture Change)**
1. **Documentation System**:
   - Comprehensive developer guide for secure database interactions
   - Code examples showing correct patterns for all query types
   - Migration guide from current patterns to new infrastructure
   - Decision tree for choosing appropriate query building methods

2. **Automated Quality Assurance**:
   - Unit tests for all query generation functions
   - Integration tests with problematic edge cases (anonymous structs, complex paths, etc.)
   - Automated security scanning in CI/CD pipeline
   - Code review checklists specifically for database interactions

3. **Refactoring Strategy**:
   - Incremental migration plan for existing code
   - Backward compatibility during transition period
   - Performance benchmarking to ensure new infrastructure doesn't degrade performance
   - Comprehensive testing to prevent regressions

**Phase 4: Long-Term Maintenance (Sustainability)**
1. **Monitoring and Alerting**: 
   - Runtime detection of query failures
   - Logging system for query security events
   - Performance monitoring for query generation overhead

2. **Continuous Improvement**:
   - Regular security audits as Kuzu database evolves
   - Updates to escaping logic as new edge cases are discovered
   - Community feedback integration for query building patterns

**Success Criteria**:
- **Zero injection vulnerabilities**: All user-provided data properly escaped
- **Consistent patterns**: Single, well-documented approach to query generation
- **Developer confidence**: Clear guidance prevents future security issues
- **Maintainability**: Centralized query logic reduces duplication and errors
- **Performance**: New infrastructure has minimal overhead compared to current implementation

**Immediate Actions** (while planning long-term solution):
1. Apply `KuzuDatabase::escapeString()` to the `name` field in `DeclarationAnalyzer.cpp`
2. Document the known vulnerable fields to prevent introduction of new similar bugs
3. Begin cataloging all query generation locations for comprehensive audit

---

*This document tracks technical issues requiring investigation. For examples-specific issues, see `EXAMPLE_BUGS.md`.*