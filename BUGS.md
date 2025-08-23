# Known Issues in MakeIndex

> **DOCUMENT PRINCIPLES:**
> - Issues present in this document are **NOT YET FIXED**
> - Issues absent from this document are either **FIXED** or **NOT YET DISCOVERED**
> - Each issue includes reproduction steps and expected vs actual behavior

## Current Issues

### Missing Inheritance Relationship Processing

**Description**: MakeIndex does not analyze or store C++ inheritance relationships between classes.

**How to reproduce**:
1. Create a C++ file with class inheritance: `class Derived : public Base { };`
2. Run MakeIndex on the file: `MakeIndex.exe compile_commands.json --output-db test_db`
3. Query for inheritance relationships: `MATCH (d:Declaration)-[:INHERITS_FROM]->(b:Declaration) RETURN d, b`

**Expected behavior**: Query should return inheritance relationships showing Derived inherits from Base.

**Actual behavior**: Query returns 0 results. No `INHERITS_FROM` relationships are created in the database.

**Technical details**: The database schema includes `INHERITS_FROM` relationship table, but the code to populate it is missing (see comment in `KuzuDump.cpp` line 326).

### Query Schema Mismatches in Tests

**Description**: Multiple test queries fail with "Cannot find property node_type for X" errors.

**How to reproduce**:
1. Run `python tests/run_tests.py`
2. Observe failures in TestDeclarationsTest, TestStatementsTest, TestTypesTest

**Expected behavior**: Queries should successfully access node_type property from appropriate table aliases.

**Actual behavior**: Queries fail with binder exceptions like "Cannot find property node_type for f".

**Technical details**: Queries attempt to access `node_type` property on Declaration table aliases instead of ASTNode table aliases. The `node_type` property only exists on ASTNode table.

### Kuzu Query Syntax Errors

**Description**: Some test queries use incorrect Kuzu/Cypher syntax causing parser or function errors.

**How to reproduce**:
1. Run `python tests/run_tests.py`
2. Observe TestTypesTest failure: "Variable t is not in scope"
3. Observe TestDeclarationsTest failure: "Function LENGTH did not receive correct arguments"

**Expected behavior**: Queries should use correct Kuzu syntax and built-in functions.

**Actual behavior**: 
- Type queries fail with variable scoping errors
- Declaration queries fail with incorrect function argument types (STRING vs RECURSIVE_REL)

### Preprocessor Query Syntax Error

**Description**: Preprocessor test has malformed query with invalid quote handling.

**How to reproduce**:
1. Run `python tests/run_tests.py`
2. Observe TestPreprocessorTest parser error: `WHERE m.replacement_text CONTAINS '>'`

**Expected behavior**: Query should properly escape quotes in CONTAINS clause.

**Actual behavior**: Parser exception due to malformed quote handling in query string.
