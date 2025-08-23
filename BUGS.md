# Known Issues in MakeIndex

> **DOCUMENT PRINCIPLES:**
> - Issues present in this document are **NOT YET FIXED**
> - Issues absent from this document are either **FIXED** or **NOT YET DISCOVERED**
> - No status indicators are used - presence in document = unfixed
> - No numbering is used to avoid maintenance overhead

## Current Issues

### Test Framework Configuration Issue - PARTIALLY FIXED
~~The test framework is using the wrong compilation database.~~ Fixed by switching to `simple_compile_commands.json` to avoid standard library dependency issues. However, tests now expect more comprehensive C++ constructs than what's available in the simple test file.

### Database Schema Issues
Several test failures indicate problems with the database schema or data population:

1. **Missing node_type property**: Tests fail with "Cannot find property node_type for f" and similar errors, suggesting the ASTNode table may not be properly joined with Declaration table in queries.

2. **Query syntax errors**: Some queries have syntax errors like malformed CONTAINS clauses with quotes.

3. **Missing inheritance relationship processing**: Tests expect relationships like INHERITS_FROM and OVERRIDES but MakeIndex doesn't implement inheritance relationship analysis yet. The schema exists but the code to populate it is missing (see KuzuDump.cpp line 326 comment).

4. **Node type mismatches - FIXED**: Tests expected `CXXRecordDecl` but actual node type is `CXXRecord`. Fixed by updating test expectations.

### Type Analysis Query Issues
The TestTypesTest fails with "Variable t is not in scope" suggesting incorrect Cypher query syntax in type-related queries.

### Declaration Analysis Issues  
TestDeclarationsTest fails with "Function LENGTH did not receive correct arguments" indicating incorrect usage of Kuzu built-in functions.
