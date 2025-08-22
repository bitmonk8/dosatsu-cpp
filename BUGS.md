# Known Issues in MakeIndex

> **DOCUMENT PRINCIPLES:**
> - Issues present in this document are **NOT YET FIXED**
> - Issues absent from this document are either **FIXED** or **NOT YET DISCOVERED**
> - No status indicators are used - presence in document = unfixed
> - No numbering is used to avoid maintenance overhead

## Current Issues

### Test Query Schema Mismatches
- **Issue**: Tests query `Declaration.node_type` but schema has `node_type` only in `ASTNode` table
- **Root Cause**: Test expectations don't match actual database schema design
- **Impact**: Most inheritance/declaration tests fail
- **Fix Needed**: Update test queries to join ASTNode and Declaration tables correctly

### Missing Statement and Expression Nodes
- **Issue**: Database shows 0 Statement and 0 Expression nodes despite processing statements/expressions
- **Root Cause**: `KuzuDump::processStatement()` doesn't call `StatementAnalyzer::createStatementNode()`
- **Impact**: All statement and expression tests fail
- **Fix Needed**: Uncomment/implement statement and expression node creation in `KuzuDump.cpp`

### Test Files Don't Match Expected Content
- **Issue**: Tests look for classes like "Animal", "Mammal" etc. but current test files have different classes
- **Root Cause**: Test files were simplified to avoid standard library includes
- **Impact**: Content-specific tests fail even with correct queries
- **Fix Needed**: Either update tests to match current content or create richer test files without std library

### Database Query Result Conversion
- **Issue**: Some queries fail with "int() argument must be a string, a bytes-like object or a real number, not 'dict'"
- **Root Cause**: Kuzu Python driver returns results in unexpected format for some query types
- **Impact**: Some count queries fail intermittently
- **Fix Needed**: Improve result parsing robustness in test framework




