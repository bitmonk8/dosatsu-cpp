# Kuzu Database Query Generation Guidelines

## Overview

This document provides guidelines for secure and consistent database query generation in the Dosatsu project. Following these patterns is critical to prevent SQL injection vulnerabilities and parsing errors.

## Systematic Query Generation Pattern

**MANDATORY PATTERN for all database queries:**

```cpp
// ✅ CORRECT: Always escape user-provided strings
std::string escapedUserInput = KuzuDatabase::escapeString(userInputString);
std::string query = "CREATE (n:Node {field: '" + escapedUserInput + "'})";
database.addToBatch(query);
```

**❌ NEVER DO: Raw string interpolation**

```cpp
// ❌ VULNERABLE: Direct interpolation without escaping
std::string query = "CREATE (n:Node {field: '" + userInputString + "'})";  // DANGEROUS!
```

## Required Escaping for All Field Types

### String Fields That MUST Be Escaped

All fields containing user-provided or AST-extracted content require escaping:

#### Node Properties
- **Declaration names**: `name`, `qualified_name`, `namespace_context`
- **Access specifiers**: `access_specifier`, `storage_class`
- **Statement data**: `statement_kind`, `control_flow_type`, `condition_text`
- **Expression data**: `expression_kind`, `value_category`, `literal_value`, `operator_kind`
- **Type information**: `type_name`, `canonical_type`
- **Template data**: `parameter_kind`, `template_arguments`, `instantiation_context`
- **Comment text**: `comment_text`, `comment_kind`, `brief_text`, `detailed_text`
- **Macro data**: `macro_name`, `parameter_names`, `replacement_text`

#### Relationship Properties
- **Reference kinds**: `reference_kind`, `relation_kind`, `specialization_kind`
- **Scope information**: `scope_kind`, `relationship_kind`
- **Inheritance data**: `inheritance_type`, `base_access_path`
- **CFG data**: `edge_type`, `condition`

### Safe Fields (No Escaping Required)

- **Node IDs**: Always integers from `std::to_string()`
- **Boolean flags**: Always `"true"` or `"false"` literals
- **Numeric values**: Always from `std::to_string()` of numeric types

## Implementation Examples

### ✅ Node Creation (Correct)

```cpp
void MyAnalyzer::createMyNode(int64_t nodeId, const std::string& userInput)
{
    try
    {
        // Escape all user-provided strings
        std::string escapedInput = KuzuDatabase::escapeString(userInput);
        std::string escapedKind = KuzuDatabase::escapeString(extractKind());
        
        std::string query = "CREATE (n:MyNode {node_id: " + std::to_string(nodeId) + 
                            ", user_field: '" + escapedInput + 
                            "', kind_field: '" + escapedKind + 
                            "', is_valid: " + (isValid ? "true" : "false") + "})";
        
        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating MyNode: " << e.what() << "\n";
    }
}
```

### ✅ Relationship Creation (Correct)

```cpp
void MyAnalyzer::createMyRelation(int64_t fromId, int64_t toId, const std::string& kind)
{
    try
    {
        std::string escapedKind = KuzuDatabase::escapeString(kind);
        std::string query = "MATCH (from:Node {node_id: " + std::to_string(fromId) + "}), " +
                            "(to:Node {node_id: " + std::to_string(toId) + "}) " +
                            "CREATE (from)-[:MY_RELATION {relation_kind: '" + escapedKind + "'}]->(to)";
        
        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating MY_RELATION: " << e.what() << "\n";
    }
}
```

## Escaping Function Details

### KuzuDatabase::escapeString()

The `KuzuDatabase::escapeString()` function handles:

1. **Backslashes**: `\` → `\\` (Critical for Windows file paths)
2. **Single quotes**: `'` → `\'` (Critical for query syntax)

**Example transformations:**
- `C:\path\file.cpp` → `C:\\path\\file.cpp`
- `operator'` → `operator\'`
- `"quoted string"` → `"quoted string"` (unchanged)

### Deprecated Escaping Methods

**❌ DO NOT USE:**

```cpp
// ❌ Incomplete escaping (only handles single quotes)
std::ranges::replace(str, '\'', '_');

// ❌ Character replacement (loses information)
std::ranges::replace(str, '\\', '_');
```

## Code Review Checklist

### For Query Generation Code

- [ ] All string interpolations use `KuzuDatabase::escapeString()`
- [ ] No raw `std::ranges::replace()` calls for escaping
- [ ] Node IDs use `std::to_string()` without escaping
- [ ] Boolean values use literal `"true"/"false"` strings
- [ ] All `CREATE`, `MATCH`, and relationship queries follow the pattern
- [ ] Exception handling wraps all query generation
- [ ] Batch operations use `database.addToBatch()`

### For New Analyzer Functions

- [ ] Identify all user-provided string fields
- [ ] Apply escaping to each string field individually
- [ ] Test with special characters: `\`, `'`, `()`, complex paths
- [ ] Verify no "Batched query failed" errors in logs
- [ ] Add appropriate error handling and logging

## Testing Requirements

### Required Test Cases

1. **Windows file paths**: Test with `C:\path\file.cpp` patterns
2. **Anonymous structures**: Test with `(unnamed struct at path:line:column)` patterns
3. **Template instantiations**: Test with complex template argument strings
4. **Special characters**: Test with single quotes, backslashes, parentheses
5. **Modern C++ features**: Test with structured bindings, lambdas, auto types

### Validation Commands

```bash
# Test specific example that previously failed
python Examples\run_examples.py --generate-cmake modern_cpp_features
python Examples\run_examples.py --index artifacts\examples\modern_cpp_features_cmake_compile_commands.json

# Comprehensive validation
python Examples\run_examples.py --all
```

### Success Criteria

- ✅ Zero "Batched query failed" errors in logs
- ✅ All indexing operations complete successfully
- ✅ All verification tests pass
- ✅ Database queries execute without parsing errors

## Historical Context

This systematic approach was implemented to resolve **Issue #1** and **Issue #2** from `BUGS.md`:

- **Root cause**: Anonymous struct names with Windows paths (`(unnamed struct at C:\path\file.cpp:line:col)`) contained unescaped backslashes and parentheses
- **Impact**: "Parser exception: Invalid input" errors during query execution
- **Solution**: Systematic application of `KuzuDatabase::escapeString()` to all user-provided string fields

## Maintenance

### When Adding New Query Generation

1. **Identify**: What string fields come from user code or AST analysis?
2. **Escape**: Apply `KuzuDatabase::escapeString()` to each string field
3. **Test**: Verify with complex C++ code patterns
4. **Document**: Update this guideline if new patterns emerge

### Regular Validation

- Run `python Examples\run_examples.py --all` as part of CI/CD
- Monitor logs for any "Batched query failed" messages
- Add new test cases for emerging C++ language features

---

**Remember**: When in doubt, escape the string. It's better to be safe than vulnerable to injection attacks or parsing failures.

