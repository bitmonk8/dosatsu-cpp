# Known Issues in Dosatsu

This document describes known technical issues in the Dosatsu project that require investigation.

## AST Node Processing Bugs

### Bug 1: Missing Core AST Node Types in Database

**Severity**: High  
**Status**: Identified, not fixed  
**Introduced**: Likely during recent batch processing optimizations

**Description**:  
The AST processing pipeline is failing to properly store several critical node types in the Kuzu database, causing example verification failures.

**Symptoms**:  
- Examples fail with errors like "Expected node type CXXRecordDecl not found"
- Missing node types include:
  - `CXXRecordDecl` (class declarations)
  - `VarDecl` (variable declarations)  
  - `ReturnStmt` (return statements)
- Database contains only 12 node types instead of expected ~20+ for simple C++ examples

**Reproduction Steps**:
1. Run `python "Examples/run_examples.py" --all`
2. Observe verification failures for simple examples containing classes and variables
3. Or run: `"artifacts\debug\bin\dosatsu_cpp.exe" --output-db=test_db "artifacts\examples\simple_cmake_compile_commands.json"`
4. Query the database: `python scripts/debug_database.py test_db`
5. Note missing CXXRecordDecl, VarDecl, ReturnStmt types

**Expected vs Actual**:
- **Expected**: simple.cpp contains classes (SimpleClass, DerivedClass, Container), variables (value, size, capacity), return statements
- **Actual**: Database only contains FunctionDecl, CXXConstructorDecl, CompoundStmt, ConstantExpr, DeclRefExpr, ImplicitCastExpr, MaterializeTemporaryExpr, MemberExpr, RValueReference, TranslationUnitDecl, CaseStmt, FunctionProto

**Investigation Notes**:
- AST visitor methods are properly declared in KuzuDump.h (`VisitCXXRecordDecl`, `VisitVarDecl`)
- ASTNodeProcessor.extractNodeType() method appears correctly implemented
- Issue likely in AST traversal, node filtering, or batch commit process
- Database creation succeeds (23MB file created) but missing core node types

**Root Cause Hypotheses**:
1. **Traversal Issue**: AST traversal not calling expected visitor methods
2. **Batch Processing Bug**: Nodes created but batch operations not committed properly
3. **Filtering Issue**: Nodes being filtered out by implicit node detection or other filters
4. **Visitor Registration**: Visitor methods declared but not properly registered in traversal

**Files Involved**:
- `source/KuzuDump.cpp` - AST visitor implementations
- `source/ASTNodeProcessor.cpp` - Node creation and type extraction
- `source/KuzuDatabase.cpp` - Batch processing and database operations
- `Examples/queries/verifiers/ast_queries.py` - Verification logic

### Bug 2: Inheritance Verifier False Negatives

**Severity**: Medium  
**Status**: Identified, not fixed

**Description**:  
The inheritance verification logic is incorrectly reporting "No C++ class declarations found" even when C++ classes are present in the source code and should be detectable.

**Symptoms**:  
- InheritanceVerifier fails with "No C++ class declarations found" 
- Occurs on examples that contain clear class declarations (simple.cpp has SimpleClass, DerivedClass, Container)
- Error appears across multiple examples (simple, standard, etc.)

**Reproduction Steps**:
1. Run `python "Examples/run_examples.py" --all`
2. Observe InheritanceVerifier failures on examples with classes
3. Examples like simple.cpp clearly contain classes but verification fails

**Root Cause Hypothesis**:
- Related to Bug 1 - if `CXXRecordDecl` nodes are missing from database, inheritance verification cannot find class declarations
- Verification logic may be querying for wrong node type or using incorrect Kuzu query syntax

**Files Involved**:
- `Examples/queries/verifiers/inheritance_queries.py` - Inheritance verification logic
- Related to core AST processing bug above

### Bug 3: Kuzu Database Lacks Standard SQL Introspection Commands

**Severity**: Low  
**Status**: Identified, workaround implemented

**Description**:  
Kuzu database doesn't support standard SQL table introspection commands like "SHOW TABLES", which can make database debugging more challenging.

**Symptoms**:  
- `SHOW TABLES` query fails with syntax error: `Parser exception: extraneous input 'SHOW' expecting {ALTER, ATTACH, BEGIN, CALL, ...}`
- Standard SQL introspection commands not available
- Must use Cypher MATCH patterns for table exploration

**Reproduction Steps**:
1. Connect to any Kuzu database
2. Try executing `SHOW TABLES` or similar SQL commands
3. Observe parser error

**Workaround**:
- Use direct Cypher queries: `MATCH (n:ASTNode) RETURN DISTINCT labels(n)`
- Query specific tables directly: `MATCH (n:ASTNode) RETURN count(n)`
- `scripts/debug_database.py` has been updated to avoid SHOW TABLES

**Files Involved**:
- `scripts/debug_database.py` - Updated to use Cypher queries instead
- Any other database introspection code should avoid SQL-style commands

---

*This document tracks technical issues requiring investigation. For examples-specific issues, see `EXAMPLE_BUGS.md`.*