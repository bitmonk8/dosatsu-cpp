# Known Issues in MakeIndex

> **DOCUMENT PRINCIPLES:**
> - Issues present in this document are **NOT YET FIXED**
> - Issues absent from this document are either **FIXED** or **NOT YET DISCOVERED**
> - No status indicators are used - presence in document = unfixed
> - No numbering is used to avoid maintenance overhead

## Current Issues

### Node ID Duplication Causing Database Constraint Violations
- **Issue**: MakeIndex generates duplicate primary key errors when processing complex C++ files
- **Symptoms**: 
  - Error: `Found duplicated primary key value X, which violates the uniqueness constraint of the primary key column`
  - Occurs when processing files with many classes, templates, and namespaces
  - Same node IDs are being assigned to different AST nodes
  - Database transaction fails due to primary key constraint violations
- **Root Cause**: Bug in node ID generation or management system within MakeIndex core
- **Technical Details**:
  - Node IDs should be unique across all AST nodes in a single compilation unit
  - The `ASTNodeProcessor::createASTNode()` method appears to be reusing IDs
  - Issue manifests when processing files with >100 AST nodes
  - Both `Declaration` and `Type` tables show duplicate ID insertions
  - Problem occurs during AST traversal, not during database schema creation
- **Impact**: 
  - Prevents processing of comprehensive test files
  - Limits testing to very simple C++ constructs
  - Remaining 6 test failures are primarily due to this issue
  - Cannot test complex inheritance, templates, or namespace hierarchies
- **Evidence**: 
  - Simple files (30-40 nodes) process successfully
  - Complex files (400+ nodes) consistently fail with duplicate IDs
  - Error pattern shows systematic ID reuse, not random collisions
  - Same node IDs appear for different AST node types (declarations, types, expressions)
- **Fix Needed**: 
  - Investigate node ID generation logic in `ASTNodeProcessor`
  - Ensure ID counter is properly maintained across AST traversal
  - Check for race conditions or reset conditions that might cause ID reuse
  - Verify ID scope is per-compilation-unit, not per-AST-subtree

### Test Files Don't Match Expected Content  
- **Issue**: Tests expect specific class names ("Animal", "Mammal", "Bat") but current test files use different classes
- **Root Cause**: Cannot use comprehensive test files due to node ID duplication issue above
- **Impact**: Content-specific tests fail even with correct database queries and schema
- **Fix Needed**: Resolve node ID duplication issue first, then create comprehensive test files
- **Dependency**: This issue is blocked by the node ID duplication problem
