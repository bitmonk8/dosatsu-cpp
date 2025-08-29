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

**Root Cause Confirmed**:
**AST Traversal Mechanism Defect**: The core issue is in the AST traversal architecture in `KuzuDump.cpp`. The current implementation inherits from `ASTNodeTraverser<KuzuDump, TextNodeDumper>` but only processes nodes through generic `VisitDecl()` and `VisitStmt()` methods. The specific visitor methods (`VisitCXXRecordDecl`, `VisitVarDecl`, etc.) are implemented but **never called** by the base traverser.

**Technical Details**:
- The `ASTNodeTraverser` base class doesn't automatically dispatch to specific visitor methods
- Current traversal flow: `Visit(TranslationUnitDecl)` → `VisitDecl()` → `processDeclaration()` 
- Missing dispatch: The traversal never calls `VisitCXXRecordDecl`, `VisitVarDecl`, or processes `ReturnStmt` nodes
- Only nodes processed in `VisitDecl` and `VisitStmt` get created (FunctionDecl, basic statements, expressions)
- Database operations and batch processing are working correctly - the issue is purely in node traversal

**Evidence**: Database contains 12 node types instead of expected 20+, specifically missing all CXXRecordDecl, VarDecl, and ReturnStmt nodes that should be created by the unexecuted visitor methods.

**Recommended Fix**:
The AST traversal mechanism needs to be redesigned to properly dispatch to specific visitor methods. Two approaches:
1. **Override TraverseDecl/TraverseStmt**: Modify the traversal to call specific Visit methods based on node type
2. **Dynamic Dispatch**: Add type checking in VisitDecl/VisitStmt to call appropriate specific methods
3. **RecursiveASTVisitor**: Consider switching from ASTNodeTraverser to RecursiveASTVisitor for automatic dispatch

**Files Involved**:
- `source/KuzuDump.cpp` - AST visitor implementations (PRIMARY FIX LOCATION)
- `source/ASTNodeProcessor.cpp` - Node creation and type extraction (working correctly)
- `source/KuzuDatabase.cpp` - Batch processing and database operations (working correctly)
- `Examples/queries/verifiers/ast_queries.py` - Verification logic (correct)

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

**Root Cause Confirmed**:
**Dependency on Bug 1**: This is a direct consequence of Bug 1. The inheritance verifier correctly queries for `CXXRecordDecl` nodes using the query:
```cypher
MATCH (a:ASTNode), (d:Declaration) WHERE a.node_type = 'CXXRecordDecl' AND a.node_id = d.node_id RETURN d LIMIT 1
```

Since Bug 1 causes `CXXRecordDecl` nodes to never be created during AST traversal, this query returns no results, causing the verifier to report "No C++ class declarations found" even when classes like `SimpleClass`, `DerivedClass`, and `Container` exist in the source code.

**Technical Details**:
- Verification logic in `inheritance_queries.py:69` is correct
- The query syntax and database schema are properly designed
- Issue occurs in `has_class_declarations()` function which fails the assertion
- All subsequent inheritance verification steps are bypassed due to this early failure

**Recommended Fix**:
This bug will be automatically resolved when Bug 1 is fixed. No changes needed to the inheritance verifier itself.

**Files Involved**:
- `Examples/queries/verifiers/inheritance_queries.py` - Inheritance verification logic (working correctly)
- Dependency: Fix Bug 1 first to resolve this issue

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

**Root Cause Confirmed**:
**Kuzu Database Design**: This is an inherent limitation of Kuzu database rather than a bug. Kuzu uses Cypher query language and doesn't support standard SQL introspection commands like `SHOW TABLES`, `DESCRIBE`, or `INFORMATION_SCHEMA`.

**Technical Details**:
- Kuzu parser explicitly rejects `SHOW TABLES` with error: "extraneous input 'SHOW' expecting {ALTER, ATTACH, BEGIN, CALL, ...}"
- This is expected behavior for a graph database using Cypher syntax
- Standard SQL commands are not part of Kuzu's supported query language

**Workaround Implementation**:
- Use direct Cypher queries: `MATCH (n:ASTNode) RETURN DISTINCT labels(n)`
- Query specific tables directly: `MATCH (n:ASTNode) RETURN count(n)`
- `scripts/debug_database.py` properly implements Cypher-based introspection
- All database debugging tools correctly avoid SQL-style commands

**Status**: Workaround is complete and working. No further action needed.

**Files Involved**:
- `scripts/debug_database.py` - Correctly uses Cypher queries instead of SQL
- All database debugging tools properly avoid SQL-style commands

---

*This document tracks technical issues requiring investigation. For examples-specific issues, see `EXAMPLE_BUGS.md`.*