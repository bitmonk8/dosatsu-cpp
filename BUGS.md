# Known Issues in Dosatsu

This document describes known technical issues in the Dosatsu project that require investigation.

## AST Node Processing Bugs

### Bug 1: Missing Core AST Node Types in Database

**Severity**: High  
**Status**: ✅ **FIXED** (2025-08-29)
**Introduced**: Likely during recent batch processing optimizations

**Description**:  
The AST processing pipeline was failing to properly store several critical node types in the Kuzu database, causing example verification failures.

**Symptoms**:  
- Examples failed with errors like "Expected node type CXXRecordDecl not found"
- Missing node types included:
  - `CXXRecordDecl` (class declarations)
  - `VarDecl` (variable declarations)  
  - `ReturnStmt` (return statements)
- Database contained only 12 node types instead of expected ~20+ for simple C++ examples

**Root Cause Identified**: **Cypher Variable Name Conflicts in Bulk Queries**

After extensive investigation, the issue was identified in the database batch processing layer in `KuzuDatabase.cpp`. The bulk query construction was creating invalid Cypher syntax due to variable name conflicts.

**Technical Details**:
- Individual queries created nodes with variable name `n`: `CREATE (n:ASTNode {...})`  
- Bulk processing combined multiple queries: `CREATE (n:ASTNode {...}), (n:ASTNode {...}), (n:ASTNode {...})`
- This created multiple variables with the same name `n` in a single Cypher query, which is invalid syntax
- Kuzu database silently rejected these malformed bulk queries without error reporting
- Only nodes that went through individual query execution (small batches) were successfully stored

**Evidence Found**:
1. ✅ AST traversal working correctly - `VisitCXXRecordDecl`, `VisitVarDecl` etc. called successfully
2. ✅ Node creation working correctly - nodes created in memory with valid IDs  
3. ✅ Batch processing functioning - queries added to batches correctly
4. ❌ Bulk query execution failing silently due to variable name conflicts

**Fix Applied**:
Modified bulk query construction in `KuzuDatabase.cpp` to generate unique variable names for each node in bulk queries:
- Before: `CREATE (n:ASTNode {...}), (n:ASTNode {...})`  
- After: `CREATE (n0:ASTNode {...}), (n1:ASTNode {...})`

**Files Modified**:
- `source/KuzuDatabase.cpp` - Fixed bulk query variable naming in chunk processing and regular bulk operations
- `source/Dosatsu.cpp` - Added explicit database flush call before program exit to ensure all operations are committed

**Verification Results**:
- ✅ Database now contains 39+ node types instead of 12
- ✅ All critical node types present: CXXRecordDecl, VarDecl, ReturnStmt  
- ✅ Example verification: 3/3 passed
- ✅ All AST processing examples now pass completely

**Performance Impact**: 
- Significant improvement in data completeness
- Database files now properly sized (28MB+ vs 23MB before)
- All bulk query optimizations retained with correct syntax

### Bug 2: Inheritance Verifier False Negatives

**Severity**: Medium  
**Status**: ✅ **FIXED** (2025-08-29) - Resolved by Bug 1 fix

**Description**:  
The inheritance verification logic was incorrectly reporting "No C++ class declarations found" even when C++ classes were present in the source code and should be detectable.

**Symptoms**:  
- InheritanceVerifier failed with "No C++ class declarations found" 
- Occurred on examples that contained clear class declarations (simple.cpp has SimpleClass, DerivedClass, Container)
- Error appeared across multiple examples (simple, standard, etc.)

**Root Cause Confirmed**:
**Direct dependency on Bug 1**: This was a direct consequence of Bug 1. The inheritance verifier correctly queries for `CXXRecordDecl` nodes using the query:
```cypher
MATCH (a:ASTNode), (d:Declaration) WHERE a.node_type = 'CXXRecordDecl' AND a.node_id = d.node_id RETURN d LIMIT 1
```

Since Bug 1 caused `CXXRecordDecl` nodes to be silently rejected during bulk database operations, this query returned no results, causing the verifier to report "No C++ class declarations found" even when classes like `SimpleClass`, `DerivedClass`, and `Container` existed in the source code.

**Fix Applied**:
Bug 2 was automatically resolved when Bug 1 was fixed. With `CXXRecordDecl` nodes now properly stored in the database due to the Cypher variable name conflict fix, inheritance verification works correctly.

**Verification Results**:
- ✅ All example verifications now pass: "Verification: 3/3 passed"
- ✅ Class declarations properly detected in database
- ✅ Inheritance verification logic working as originally designed

**Technical Details**:
- Verification logic in `inheritance_queries.py:69` was correct all along
- The query syntax and database schema were properly designed
- Issue was resolved by database layer fixes from Bug 1

**Files Involved**:
- `Examples/queries/verifiers/inheritance_queries.py` - Inheritance verification logic (unchanged)
- Fixed by database layer improvements from Bug 1

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