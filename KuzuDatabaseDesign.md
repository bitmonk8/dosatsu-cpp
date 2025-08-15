# Kuzu Database Integration Design for MakeIndex

## 1. Executive Summary

This document outlines the design for converting MakeIndex/KuzuDump from text-based AST output to Kuzu graph database storage. The goal is to model C++ Abstract Syntax Tree (AST) nodes as vertices and their relationships as edges in a structured property graph, leveraging Kuzu's native graph capabilities for better querying and analysis.

## 2. Research Findings

### 2.1 Current Implementation Analysis

**KuzuDump Class Structure:**
- Inherits from `ASTNodeTraverser<KuzuDump, TextNodeDumper>`
- Uses Clang's `TextNodeDumper` to output hierarchical text representation
- Processes AST nodes through visitor pattern (`Visit()` methods)
- Currently outputs to `raw_ostream` (text files)

**Current AST Output Format:**
```
TranslationUnitDecl 0x19205c27698 <<invalid sloc>> <invalid sloc>
|-CXXRecordDecl 0x19205c27f18 <<invalid sloc>> <invalid sloc> implicit struct _GUID
| `-TypeVisibilityAttr 0x19205c27fd0 <<invalid sloc>> Implicit Default
|-TypedefDecl 0x19205c28170 <<invalid sloc>> <invalid sloc> implicit __int128_t '__int128'
| `-BuiltinType 0x19205c27c60 '__int128'
|-NamespaceDecl 0x19206ad6478 <file:17:1, line:39:1> line:17:11 clang
```

**Key Information Extracted:**
- Node type (TranslationUnitDecl, CXXRecordDecl, TypedefDecl, etc.)
- Memory address (unique identifier)
- Source location (file, line, column)
- Node-specific attributes (implicit, struct name, type info)
- Hierarchical relationships (parent-child via tree structure)

### 2.2 Kuzu C++ API Research

**Core Kuzu C++ API Components:**
```cpp
#include "kuzu.hpp"

// Database connection
kuzu::main::Database database(databasePath);
kuzu::main::Connection connection(&database);

// Query execution
auto result = connection.query("CREATE NODE TABLE ...");

// Processing results
while (result->hasNext()) {
    auto tuple = result->getNext();
    // Process tuple data
}
```

**Key API Capabilities:**
- **Database Management**: Create/open databases with `kuzu::main::Database`
- **Connection Handling**: Thread-safe connections via `kuzu::main::Connection`
- **DDL Operations**: `CREATE NODE TABLE`, `CREATE REL TABLE` for schema definition
- **DML Operations**: `CREATE`, `MATCH`, `SET` for data manipulation
- **Query Results**: Iterate through results with `QueryResult::getNext()`
- **Data Types**: STRING, INT64, BOOLEAN, DOUBLE, etc.

**Important Performance Notes:**
- `getNext()` reuses the same `FlatTuple` object - data must be processed immediately
- For deferred processing, must explicitly copy values using `tuple->getValue(i)->copy()`

### 2.3 Graph Database Schema Design Principles

**Structured Property Graph Model:**
- **Nodes (Vertices)**: Represent AST declarations, statements, expressions
- **Relationships (Edges)**: Represent AST parent-child relationships, references
- **Properties**: Store node-specific data (names, types, source locations)

## 3. Schema Design

### 3.1 Node Tables (AST Node Types)

**Primary AST Node Categories:**
```cypher
// Base node for all AST nodes
CREATE NODE TABLE ASTNode(
    node_id INT64 PRIMARY KEY,        -- Hash of memory address
    node_type STRING,                 -- e.g., "TranslationUnitDecl"
    memory_address STRING,            -- Original pointer address
    source_file STRING,               -- Source file path
    start_line INT64,                 -- Starting line number
    start_column INT64,               -- Starting column number
    end_line INT64,                   -- Ending line number  
    end_column INT64,                 -- Ending column number
    is_implicit BOOLEAN,              -- Whether node is compiler-generated
    raw_text STRING                   -- Raw textual representation
);

// Declaration nodes (functions, variables, classes, etc.)
CREATE NODE TABLE Declaration(
    node_id INT64 PRIMARY KEY,
    name STRING,                      -- Declaration name
    qualified_name STRING,            -- Fully qualified name
    access_specifier STRING,          -- public/private/protected
    storage_class STRING,             -- static/extern/etc
    is_definition BOOLEAN,            -- Declaration vs definition
    namespace_context STRING         -- Containing namespace
);

// Type nodes (built-in types, user-defined types)
CREATE NODE TABLE Type(
    node_id INT64 PRIMARY KEY,
    type_name STRING,                 -- e.g., "int", "__int128"
    canonical_type STRING,            -- Canonical type representation
    size_bytes INT64,                 -- Type size if known
    is_const BOOLEAN,                 -- Const qualifier
    is_volatile BOOLEAN,              -- Volatile qualifier
    is_builtin BOOLEAN                -- Built-in vs user-defined
);

// Statement nodes (control flow, expressions)  
CREATE NODE TABLE Statement(
    node_id INT64 PRIMARY KEY,
    statement_kind STRING,            -- e.g., "CompoundStmt", "IfStmt"
    has_side_effects BOOLEAN          -- Whether statement has side effects
);

// Expression nodes (literals, operators, function calls)
CREATE NODE TABLE Expression(
    node_id INT64 PRIMARY KEY,
    expression_kind STRING,           -- e.g., "IntegerLiteral", "CallExpr"
    value_category STRING,            -- lvalue/rvalue/xvalue
    literal_value STRING,             -- For literal expressions
    operator_kind STRING              -- For operator expressions
);

// Attribute nodes (compiler attributes, annotations)
CREATE NODE TABLE Attribute(
    node_id INT64 PRIMARY KEY,
    attribute_kind STRING,            -- e.g., "TypeVisibilityAttr"
    attribute_value STRING            -- Attribute-specific data
);
```

### 3.2 Relationship Tables (AST Structure)

```cypher
// Parent-child relationships in AST tree
CREATE REL TABLE PARENT_OF(
    FROM ASTNode TO ASTNode,
    child_index INT64,                -- Order of child in parent
    relationship_kind STRING          -- e.g., "body", "condition", "declaration"
);

// Type relationships
CREATE REL TABLE HAS_TYPE(
    FROM Declaration TO Type,
    type_role STRING                  -- e.g., "return_type", "parameter_type"
);

// Reference relationships (variable usage, function calls)
CREATE REL TABLE REFERENCES(
    FROM ASTNode TO Declaration,
    reference_kind STRING,            -- e.g., "calls", "uses", "declares"
    is_direct BOOLEAN                 -- Direct vs indirect reference
);

// Scope relationships (which declarations are visible)
CREATE REL TABLE IN_SCOPE(
    FROM ASTNode TO Declaration,
    scope_kind STRING                 -- e.g., "local", "global", "namespace"
);

// Template relationships
CREATE REL TABLE TEMPLATE_RELATION(
    FROM Declaration TO Declaration,
    relation_type STRING              -- e.g., "specializes", "instantiates"
);
```

### 3.3 Schema Benefits

**Query Capabilities:**
- Find all functions in a namespace: `MATCH (n:Declaration)-[:IN_SCOPE]->(ns:Declaration {name: "std"}) WHERE n.node_type = "FunctionDecl"`
- Analyze call graphs: `MATCH (caller:Declaration)-[:REFERENCES]->(callee:Declaration) WHERE caller.node_type = "FunctionDecl"`
- Find complex type dependencies: `MATCH (d:Declaration)-[:HAS_TYPE*1..3]->(t:Type)`
- Source location analysis: `MATCH (n:ASTNode) WHERE n.source_file CONTAINS "MakeIndex.cpp"`

**Graph Analytics:**
- AST depth analysis
- Dead code detection (unreferenced declarations)
- Circular dependency detection
- Code complexity metrics

## 4. Implementation Design

### 4.1 Modified KuzuDump Architecture

**Initial Implementation (Phases 1-3):**
```cpp
class KuzuDump : public ASTNodeTraverser<KuzuDump, TextNodeDumper>
{
private:
    // Database components
    std::unique_ptr<kuzu::main::Connection> connection;
    std::unordered_map<const void*, int64_t> nodeIdMap;  // Pointer -> node_id mapping
    int64_t nextNodeId = 1;
    
    // Temporary: Text output support (will be removed in Phase 4)
    TextNodeDumper NodeDumper;
    raw_ostream* OS;  // nullptr if database-only mode
    
    // Schema creation
    void createSchema();
    
    // Node creation methods
    int64_t createASTNode(const clang::Decl* node);
    int64_t createASTNode(const clang::Stmt* node);  
    int64_t createASTNode(const clang::Type* node);
    
    // Relationship creation methods
    void createParentChildRelation(int64_t parentId, int64_t childId, int index);
    void createTypeRelation(int64_t declId, int64_t typeId);
    void createReferenceRelation(int64_t fromId, int64_t toId, const std::string& kind);
    
    // Data extraction utilities
    std::string extractSourceLocation(const clang::SourceLocation& loc);
    std::string extractNodeType(const clang::Decl* node);
    bool isImplicitNode(const clang::Decl* node);

public:
    // Temporary: Dual constructor support
    KuzuDump(const std::string& databasePath, const ASTContext& Context);
    KuzuDump(const std::string& databasePath, raw_ostream& OS, const ASTContext& Context);
    
    // Override visitor methods to store in database (and optionally text)
    void Visit(const Decl* D);
    void Visit(const Stmt* S);
    void Visit(const Type* T);
};
```

**Final Implementation (Phase 4+):**
```cpp
class KuzuDump : public ASTNodeTraverser<KuzuDump>
{
private:
    // Database-only components
    std::unique_ptr<kuzu::main::Connection> connection;
    std::unordered_map<const void*, int64_t> nodeIdMap;
    int64_t nextNodeId = 1;
    
    // Schema and data methods (same as above)
    void createSchema();
    int64_t createASTNode(const clang::Decl* node);
    // ... other methods

public:
    KuzuDump(const std::string& databasePath, const ASTContext& Context);
    
    // Database-only visitor methods
    void Visit(const Decl* D);
    void Visit(const Stmt* S);
    void Visit(const Type* T);
};
```

### 4.2 Database Initialization

```cpp
void KuzuDump::createSchema() {
    // Create node tables
    connection->query("CREATE NODE TABLE ASTNode("
                     "node_id INT64 PRIMARY KEY, "
                     "node_type STRING, "
                     "memory_address STRING, "
                     "source_file STRING, "
                     "start_line INT64, "
                     "start_column INT64, "
                     "end_line INT64, "
                     "end_column INT64, "
                     "is_implicit BOOLEAN, "
                     "raw_text STRING)");

    connection->query("CREATE NODE TABLE Declaration("
                     "node_id INT64 PRIMARY KEY, "
                     "name STRING, "
                     "qualified_name STRING, "
                     "access_specifier STRING, "
                     "storage_class STRING, "
                     "is_definition BOOLEAN, "
                     "namespace_context STRING)");
    
    // ... create other node tables
    
    // Create relationship tables
    connection->query("CREATE REL TABLE PARENT_OF("
                     "FROM ASTNode TO ASTNode, "
                     "child_index INT64, "
                     "relationship_kind STRING)");
    
    // ... create other relationship tables
}
```

### 4.3 Node Processing Pipeline

```cpp
int64_t KuzuDump::createASTNode(const clang::Decl* decl) {
    // Check if already processed
    auto it = nodeIdMap.find(decl);
    if (it != nodeIdMap.end()) {
        return it->second;
    }
    
    int64_t nodeId = nextNodeId++;
    nodeIdMap[decl] = nodeId;
    
    // Extract common AST node properties
    std::string nodeType = extractNodeType(decl);
    std::string memoryAddr = llvm::formatv("{0:x}", (uintptr_t)decl);
    std::string sourceFile = extractSourceLocation(decl->getLocation());
    bool isImplicit = isImplicitNode(decl);
    
    // Insert base AST node
    auto query = llvm::formatv(
        "CREATE (n:ASTNode {{node_id: {0}, node_type: '{1}', "
        "memory_address: '{2}', source_file: '{3}', is_implicit: {4}}})",
        nodeId, nodeType, memoryAddr, sourceFile, isImplicit ? "true" : "false");
    
    connection->query(query.str());
    
    // Create specialized node based on type
    if (llvm::isa<clang::NamedDecl>(decl)) {
        createDeclarationNode(nodeId, llvm::cast<clang::NamedDecl>(decl));
    }
    
    return nodeId;
}
```

### 4.4 Integration with MakeIndex

**Command Line Interface:**
```cmd
rem Current text output
MakeIndex.exe --output=text_dump.txt --filter="*MakeIndex*" compile_commands.json

rem New database output
MakeIndex.exe --output-db=ast_analysis.kuzu --filter="*MakeIndex*" compile_commands.json

rem Hybrid mode (both text and database) - temporary during development
MakeIndex.exe --output=text_dump.txt --output-db=ast_analysis.kuzu compile_commands.json
```

**Configuration Options:**
- `--output-db <path>`: Kuzu database output path  
- `--overwrite-db`: Overwrite existing database
- `--append-db`: Append to existing database
- `--db-batch-size <N>`: Transaction batch size for performance
- `--output <path>`: Text output path (temporary, for development comparison only)

**Note on Text Output Support:**
The existing `--output` text dumping functionality will be maintained temporarily during development to:
1. Verify that the database output captures the same information as text output
2. Enable side-by-side comparison during implementation
3. Provide a fallback during early development phases

Once the database output is fully functional and validated, the text output support should be **removed** to:
- Simplify the codebase and reduce maintenance burden
- Eliminate the dual-mode complexity in KuzuDump
- Focus development efforts on the graph database capabilities
- Avoid confusion about which output format to use

## 5. Implementation Plan

### 5.1 Phase 1: Database Infrastructure (Week 1)

**Tasks:**
1. **Database Connection Management**
   - Add database path parameter to KuzuDump constructor
   - Implement database creation/opening logic
   - Add connection error handling

2. **Schema Creation**
   - Implement `createSchema()` method
   - Define all node and relationship tables
   - Add schema validation

3. **Basic Node Creation**
   - Implement node ID generation and mapping
   - Create basic AST node insertion logic
   - Add memory address to node ID conversion

**Verification:**
```cmd
python build.py build && python build.py test
artifacts\Debug\bin\MakeIndex.exe --output-db=test.kuzu --filter="*MakeIndex.cpp" artifacts\debug\build\compile_commands.json
```

**Success Criteria:**
- Database creates successfully
- Schema tables exist
- Basic AST nodes are inserted

### 5.2 Phase 2: Core AST Processing (Week 2)

**Tasks:**
1. **Declaration Processing**
   - Implement `createDeclarationNode()` for named declarations
   - Extract declaration names, types, and attributes
   - Handle namespace and scope information

2. **Type Processing**  
   - Implement type node creation for built-in and user types
   - Create type relationships (HAS_TYPE)
   - Handle type qualifiers (const, volatile)

3. **Hierarchy Processing**
   - Implement parent-child relationship creation
   - Track AST traversal order and child indices
   - Create PARENT_OF relationships

**Verification:**
```cmd
rem Test with limited scope
artifacts\Debug\bin\MakeIndex.exe --output-db=makeindex.kuzu --filter="*MakeIndex*" artifacts\debug\build\compile_commands.json

rem Query results
artifacts\Debug\bin\kuzu_shell makeindex.kuzu
rem > MATCH (n:ASTNode) RETURN COUNT(*);
rem > MATCH (d:Declaration) RETURN d.name, d.node_type;
```

**Success Criteria:**
- All major AST node types are stored
- Parent-child relationships are correct
- Declaration names and types are captured

### 5.3 Phase 3: Advanced Relationships (Week 3)

**Tasks:**
1. **Reference Tracking**
   - Implement REFERENCES relationships for function calls
   - Track variable usage and declarations
   - Handle template instantiations

2. **Scope Analysis**
   - Implement IN_SCOPE relationships
   - Track namespace and local scope visibility
   - Handle using declarations and directives

3. **Performance Optimization**
   - Implement batched insertions
   - Add transaction management
   - Optimize memory usage for large ASTs

**Verification:**
```cmd
rem Test with full MakeIndex project
artifacts\Debug\bin\MakeIndex.exe --output-db=full_project.kuzu artifacts\debug\build\compile_commands.json

rem Complex queries
rem > MATCH (caller:Declaration)-[:REFERENCES]->(callee:Declaration) 
rem   WHERE caller.node_type = 'FunctionDecl' AND callee.node_type = 'FunctionDecl'
rem   RETURN caller.name, callee.name;
```

### 5.4 Phase 4: Integration & Testing (Week 4)

**Tasks:**
1. **Command Line Integration**
   - Add database output options to CLI
   - Implement hybrid text/database output (temporary)
   - Add database configuration options

2. **Comprehensive Testing**
   - Test with various C++ constructs (templates, inheritance, etc.)
   - Performance testing with large codebases
   - Error handling and recovery testing
   - Validate database output matches text output completeness

3. **Text Output Removal**
   - Once database functionality is validated, remove text output code
   - Simplify KuzuDump class to database-only mode
   - Remove TextNodeDumper dependency
   - Update CLI to database-only options

4. **Documentation & Examples**
   - Create usage examples and query samples
   - Document schema and relationship types
   - Provide performance tuning guidelines

**Success Criteria:**
- Complete MakeIndex source analysis in <5 minutes
- Database queries return correct results
- Integration with existing build system works
- Text output support cleanly removed without functionality loss

## 6. Example Usage Scenarios

### 6.1 Development Workflow

```cmd
rem 1. Build project and generate AST database
python build.py build
artifacts\Debug\bin\MakeIndex.exe --output-db=project_ast.kuzu artifacts\debug\build\compile_commands.json

rem 2. Analyze codebase with Kuzu queries
artifacts\Debug\bin\kuzu_shell project_ast.kuzu

rem Find all public methods in classes
rem > MATCH (class:Declaration {node_type: 'CXXRecordDecl'})-[:PARENT_OF]->(method:Declaration {node_type: 'CXXMethodDecl', access_specifier: 'public'})
rem   RETURN class.name, method.name;

rem Find functions with complex parameter types
rem > MATCH (func:Declaration {node_type: 'FunctionDecl'})-[:PARENT_OF]->(param:Declaration)-[:HAS_TYPE]->(type:Type)
rem   WHERE type.type_name CONTAINS 'std::'
rem   RETURN func.name, param.name, type.type_name;

rem Analyze include dependencies
rem > MATCH (file:ASTNode {node_type: 'TranslationUnitDecl'})
rem   RETURN file.source_file, COUNT(*) as ast_node_count
rem   ORDER BY ast_node_count DESC;
```

### 6.2 Code Analysis Queries

```cypher
-- Find potential memory leaks (new without delete)
MATCH (new_expr:Expression {expression_kind: 'CXXNewExpr'})
WHERE NOT EXISTS {
    MATCH (new_expr)-[:REFERENCES*1..10]->(delete_expr:Expression {expression_kind: 'CXXDeleteExpr'})
}
RETURN new_expr.source_file, new_expr.start_line;

-- Identify large functions (high AST node count)
MATCH (func:Declaration {node_type: 'FunctionDecl'})-[:PARENT_OF*]->(child:ASTNode)
WITH func, COUNT(child) as complexity
WHERE complexity > 100
RETURN func.name, func.source_file, complexity
ORDER BY complexity DESC;

-- Find circular dependencies between classes
MATCH path = (class1:Declaration {node_type: 'CXXRecordDecl'})-[:REFERENCES*2..10]->(class1)
RETURN class1.name, LENGTH(path) as cycle_length;
```

## 7. Success Metrics

### 7.1 Functional Metrics

1. **Completeness**: 100% of AST nodes from text output are stored in database
2. **Correctness**: Parent-child relationships match original AST structure  

### 7.2 Quality Metrics

1. **Code Quality**: All code passes existing linting and formatting standards
2. **Test Coverage**: >90% code coverage for new database functionality
3. **Documentation**: Complete API documentation and usage examples
4. **Stability**: Zero crashes or data corruption during normal operation

