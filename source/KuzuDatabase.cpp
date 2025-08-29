//===--- KuzuDatabase.cpp - Database management for AST storage ----------===//
//
// Part of the Dosatsu project
//
//===----------------------------------------------------------------------===//

#include "KuzuDatabase.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>

using namespace clang;

KuzuDatabase::KuzuDatabase(std::string databasePath) : databasePath(std::move(databasePath))
{
}

KuzuDatabase::~KuzuDatabase()
{
    flushOperations();
}

void KuzuDatabase::initialize()
{
    if (databasePath.empty())
        return;

    try
    {
        // Create database directory if it doesn't exist
        std::filesystem::path dbPath(databasePath);
        if (dbPath.has_parent_path())
            std::filesystem::create_directories(dbPath.parent_path());

        // Initialize Kuzu database
        database = std::make_unique<kuzu::main::Database>(databasePath);
        connection = std::make_unique<kuzu::main::Connection>(database.get());

        // Initialize connection pool for better performance
        initializeConnectionPool();

        // Create schema
        createSchema();
        
        // Initialize relationship schema information
        initializeRelationshipSchemaInfo();
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to initialize Kuzu database: " + std::string(e.what()));
    }
}

void KuzuDatabase::executeSchemaQuery(const std::string& query, const std::string& schemaName)
{
    if (!connection)
        throw std::runtime_error("Database not initialized");

    auto result = connection->query(query);
    if (!result->isSuccess())
        throw std::runtime_error("Failed to create " + schemaName + " table: " + result->getErrorMessage());
}

void KuzuDatabase::beginTransaction()
{
    if (!connection || transactionActive)
        return;

    try
    {
        auto result = connection->query("BEGIN TRANSACTION");
        if (result->isSuccess())
            transactionActive = true;
        else
            llvm::errs() << "Failed to begin transaction: " << result->getErrorMessage() << "\n";
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception beginning transaction: " << e.what() << "\n";
    }
}

void KuzuDatabase::commitTransaction()
{
    if (!connection || !transactionActive)
    {
        // Silently ignore if no transaction is active
        return;
    }

    try
    {
        auto result = connection->query("COMMIT");
        transactionActive = false;  // Mark as inactive regardless of result
        if (!result->isSuccess())
            llvm::errs() << "Failed to commit transaction: " << result->getErrorMessage() << "\n";
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception committing transaction: " << e.what() << "\n";
        transactionActive = false;  // Ensure we reset state
    }
}

void KuzuDatabase::rollbackTransaction()
{
    if (!connection || !transactionActive)
        return;

    try
    {
        auto result = connection->query("ROLLBACK");
        if (result->isSuccess())
            transactionActive = false;
        else
            llvm::errs() << "Failed to rollback transaction: " << result->getErrorMessage() << "\n";
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception rolling back transaction: " << e.what() << "\n";
    }
}

void KuzuDatabase::addToBatch(const std::string& query)
{
    if (!connection || query.empty())
        return;

    pendingQueries.push_back(query);
    totalOperations++;
    operationsSinceLastCommit++;

    // Start transaction on first batched operation
    if (!transactionActive)
        beginTransaction();

    // Optimize transaction boundaries - commit periodically for better performance
    if (operationsSinceLastCommit >= TRANSACTION_COMMIT_THRESHOLD)
        optimizeTransactionBoundaries();

    // Execute batch when it reaches the configured batch size
    if (pendingQueries.size() >= BATCH_SIZE)
        executeBatch();
}

void KuzuDatabase::addRelationshipToBatch(int64_t fromNodeId,
                                          int64_t toNodeId,
                                          const std::string& relationshipType,
                                          const std::map<std::string, std::string>& properties)
{
    if (!connection)
        return;

    pendingRelationships.emplace_back(fromNodeId, toNodeId, relationshipType, properties);
    totalOperations++;
    operationsSinceLastCommit++;

    // Start transaction on first batched operation
    if (!transactionActive)
        beginTransaction();

    // Optimize transaction boundaries - commit periodically for better performance
    if (operationsSinceLastCommit >= TRANSACTION_COMMIT_THRESHOLD)
        optimizeTransactionBoundaries();

    // Execute batch when it reaches the batch size
    if (pendingQueries.size() + pendingRelationships.size() >= BATCH_SIZE)
        executeBatch();
}

void KuzuDatabase::addBulkRelationshipsToBatch(
    const std::vector<std::tuple<int64_t, int64_t, std::string, std::map<std::string, std::string>>>& relationships)
{
    if (!connection || relationships.empty())
        return;

    // Add all relationships to pending batch
    for (const auto& rel : relationships)
    {
        pendingRelationships.push_back(rel);
        totalOperations++;
        operationsSinceLastCommit++;
    }

    // Start transaction on first batched operation
    if (!transactionActive)
        beginTransaction();

    // Optimize transaction boundaries - commit periodically for better performance
    if (operationsSinceLastCommit >= TRANSACTION_COMMIT_THRESHOLD)
        optimizeTransactionBoundaries();

    // Execute batch when it reaches the batch size
    if (pendingQueries.size() + pendingRelationships.size() >= BATCH_SIZE)
        executeBatch();
}

void KuzuDatabase::executeBatch()
{
    if (!connection || (pendingQueries.empty() && pendingRelationships.empty()))
        return;

    try
    {
        // Group queries by type for true bulk operations
        executeBulkQueries();
        
        // Execute optimized relationship batching with schema awareness
        executeOptimizedRelationships();

        pendingQueries.clear();
        pendingRelationships.clear();
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception executing batch: " << e.what() << "\n";
        pendingQueries.clear();
        pendingRelationships.clear();
    }
}

void KuzuDatabase::executeBulkQueries()
{
    if (pendingQueries.empty())
        return;

    try
    {
        // Parse and group queries by table type
        std::map<std::string, std::vector<std::string>> groupedQueries;
        parseAndGroupQueries(groupedQueries);
        
        // Execute bulk CREATE for each node table
        for (const auto& [tableName, nodeDataList] : groupedQueries)
        {
            if (nodeDataList.empty())
                continue;
            
            // Handle unbatchable queries separately
            if (tableName == "__unbatchable__")
            {
                // Execute these individually as they were originally
                for (const auto& query : nodeDataList)
                {
                    auto result = connection->query(query);
                    if (!result->isSuccess())
                    {
                        llvm::errs() << "Query failed: " << result->getErrorMessage() << "\n";
                        llvm::errs() << "Query: " << query << "\n";
                    }
                }
                continue;
            }
                
            // Build bulk CREATE query with multiple nodes
            std::string bulkQuery;
            if (nodeDataList.size() == 1)
            {
                // Single node - use original format with CREATE prefix
                bulkQuery = "CREATE " + nodeDataList[0];
            }
            else if (nodeDataList.size() > 10)
            {
                // Very large batch - split into smaller chunks to avoid query size limits
                const size_t chunkSize = 50;
                for (size_t i = 0; i < nodeDataList.size(); i += chunkSize)
                {
                    std::string chunkQuery = "CREATE ";
                    size_t end = std::min(i + chunkSize, nodeDataList.size());
                    for (size_t j = i; j < end; ++j)
                    {
                        if (j > i) chunkQuery += ", ";
                        chunkQuery += nodeDataList[j];
                    }
                    
                    auto result = connection->query(chunkQuery);
                    if (!result->isSuccess())
                    {
                        llvm::errs() << "Chunk query failed: " << result->getErrorMessage() << "\n";
                        // Fallback to individual queries for this chunk
                        for (size_t j = i; j < end; ++j)
                        {
                            auto individualResult = connection->query("CREATE " + nodeDataList[j]);
                            if (!individualResult->isSuccess())
                            {
                                llvm::errs() << "Individual query also failed: " << individualResult->getErrorMessage() << "\n";
                            }
                        }
                    }
                }
                continue;
            }
            else
            {
                // Multiple nodes - create bulk query
                bulkQuery = "CREATE ";
                for (size_t i = 0; i < nodeDataList.size(); ++i)
                {
                    if (i > 0) bulkQuery += ", ";
                    bulkQuery += nodeDataList[i];
                }
            }
            
            auto result = connection->query(bulkQuery);
            if (!result->isSuccess())
            {
                llvm::errs() << "Bulk query failed: " << result->getErrorMessage() << "\n";
                // Fallback to individual queries
                for (const auto& nodeData : nodeDataList)
                {
                    auto individualResult = connection->query("CREATE " + nodeData);
                    if (!individualResult->isSuccess())
                    {
                        llvm::errs() << "Individual query also failed: " << individualResult->getErrorMessage() << "\n";
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception in bulk query execution: " << e.what() << "\n";
        // Fallback to original individual execution
        for (const auto& query : pendingQueries)
        {
            try
            {
                auto result = connection->query(query);
                if (!result->isSuccess())
                {
                    llvm::errs() << "Fallback query failed: " << result->getErrorMessage() << "\n";
                }
            }
            catch (...) {}
        }
    }
}

void KuzuDatabase::parseAndGroupQueries(std::map<std::string, std::vector<std::string>>& groupedQueries)
{
    // Parse queries and group by type
    // Only batch simple node creations with primary keys, not relationship or complex queries
    for (const auto& query : pendingQueries)
    {
        // Check if this is a simple CREATE statement for node creation
        size_t createPos = query.find("CREATE ");
        if (createPos != std::string::npos)
        {
            // Check if this is a MATCH...CREATE pattern (relationship creation)
            size_t matchPos = query.find("MATCH ");
            if (matchPos != std::string::npos && matchPos < createPos)
            {
                // This is a MATCH...CREATE query (like PARENT_OF relationships), execute individually
                groupedQueries["__unbatchable__"].push_back(query);
                continue;
            }
            
            // Simple CREATE query - try to batch if it's a node creation
            size_t nodeStart = query.find("(", createPos);
            if (nodeStart == std::string::npos)
            {
                groupedQueries["__unbatchable__"].push_back(query);
                continue;
            }
                
            size_t colonPos = query.find(":", nodeStart);
            if (colonPos == std::string::npos)
            {
                groupedQueries["__unbatchable__"].push_back(query);
                continue;
            }
                
            size_t spaceOrBrace = query.find_first_of(" {", colonPos);
            if (spaceOrBrace == std::string::npos)
            {
                groupedQueries["__unbatchable__"].push_back(query);
                continue;
            }
                
            std::string nodeType = query.substr(colonPos + 1, spaceOrBrace - colonPos - 1);
            
            // Check if query contains node_id (required for batching)
            if (query.find("node_id:") != std::string::npos)
            {
                // All AST nodes go to the ASTNode table regardless of their specific type
                // The node_type field stores the actual Clang type (FunctionDecl, etc.)
                // but the database table is always ASTNode
                std::string nodeData = query.substr(nodeStart);
                groupedQueries["ASTNode"].push_back(nodeData);
            }
            else
            {
                // Query missing node_id, can't batch it
                groupedQueries["__unbatchable__"].push_back(query);
            }
        }
        else
        {
            // Not a CREATE query, execute individually
            groupedQueries["__unbatchable__"].push_back(query);
        }
    }
}

void KuzuDatabase::executeOptimizedRelationships()
{
    if (pendingRelationships.empty())
        return;

    try
    {
        // Group relationships by type for bulk operations
        std::map<std::string, std::vector<std::tuple<int64_t, int64_t, std::map<std::string, std::string>>>>
            groupedRelationships;

        for (const auto& [fromId, toId, relType, properties] : pendingRelationships)
            groupedRelationships[relType].emplace_back(fromId, toId, properties);

        // Execute each relationship type as a bulk operation
        for (const auto& [relType, relationships] : groupedRelationships)
            executeBulkRelationshipType(relType, relationships);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception executing optimized relationships: " << e.what() << "\n";
    }
}

void KuzuDatabase::executeBulkRelationshipType(
    const std::string& relationshipType,
    const std::vector<std::tuple<int64_t, int64_t, std::map<std::string, std::string>>>& relationships)
{
    if (relationships.empty())
        return;

    try
    {
        // Get the correct node types for this relationship from schema
        auto [fromNodeType, toNodeType] = getRelationshipNodeTypes(relationshipType);
        
        // Build bulk relationship creation query with correct schema
        std::string bulkQuery = "UNWIND [";
        
        bool first = true;
        for (const auto& [fromId, toId, properties] : relationships)
        {
            if (!first) bulkQuery += ", ";
            first = false;
            
            bulkQuery += "{from_id: " + std::to_string(fromId) + 
                        ", to_id: " + std::to_string(toId);
            
            // Add properties with correct type handling
            for (const auto& [key, value] : properties)
            {
                if (isPropertyBoolean(relationshipType, key))
                {
                    // Handle boolean values without quotes
                    std::string boolValue = (value == "true" || value == "1") ? "true" : "false";
                    bulkQuery += ", " + key + ": " + boolValue;
                }
                else
                {
                    // Handle string values with quotes
                    bulkQuery += ", " + key + ": '" + escapeString(value) + "'";
                }
            }
            bulkQuery += "}";
        }
        
        bulkQuery += "] AS rel ";
        
        // Use correct node types from schema
        bulkQuery += "MATCH (from:" + fromNodeType + " {node_id: rel.from_id}), ";
        bulkQuery += "(to:" + toNodeType + " {node_id: rel.to_id}) ";
        bulkQuery += "CREATE (from)-[:" + relationshipType;
        
        // Add property mapping if needed
        if (!relationships.empty() && !std::get<2>(relationships[0]).empty())
        {
            bulkQuery += " {";
            bool firstProp = true;
            for (const auto& [key, _] : std::get<2>(relationships[0]))
            {
                if (!firstProp) bulkQuery += ", ";
                firstProp = false;
                bulkQuery += key + ": rel." + key;
            }
            bulkQuery += "}";
        }
        
        bulkQuery += "]->(to)";
        
        auto result = connection->query(bulkQuery);
        if (!result->isSuccess())
        {
            // Schema-aware fallback to individual queries
            executeSchemaAwareFallbackRelationships(relationshipType, relationships);
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception in bulk relationship creation: " << e.what() << "\n";
        executeSchemaAwareFallbackRelationships(relationshipType, relationships);
    }
}

void KuzuDatabase::executeFallbackRelationships(
    const std::string& relationshipType,
    const std::vector<std::tuple<int64_t, int64_t, std::map<std::string, std::string>>>& relationships)
{
    // Fallback: execute individual optimized queries (still better than original MATCH...CREATE)
    for (const auto& [fromId, toId, properties] : relationships)
    {
        std::string query = "MATCH (from:ASTNode {node_id: " + std::to_string(fromId) +
                            "}), (to:ASTNode {node_id: " + std::to_string(toId) + "}) " +
                            "CREATE (from)-[:" + relationshipType;

        if (!properties.empty())
        {
            query += " {";
            bool first = true;
            for (const auto& [key, value] : properties)
            {
                if (!first)
                    query += ", ";
                first = false;
                query += key + ": '" + escapeString(value) + "'";
            }
            query += "}";
        }

        query += "]->(to)";

        auto result = connection->query(query);
        if (!result->isSuccess())
            llvm::errs() << "Fallback relationship query failed: " << result->getErrorMessage() << "\n";
    }
}

void KuzuDatabase::executeSchemaAwareFallbackRelationships(
    const std::string& relationshipType,
    const std::vector<std::tuple<int64_t, int64_t, std::map<std::string, std::string>>>& relationships)
{
    // Get the correct node types for this relationship from schema
    auto [fromNodeType, toNodeType] = getRelationshipNodeTypes(relationshipType);
    
    // Execute individual queries with correct schema
    for (const auto& [fromId, toId, properties] : relationships)
    {
        std::string query = "MATCH (from:" + fromNodeType + " {node_id: " + std::to_string(fromId) +
                            "}), (to:" + toNodeType + " {node_id: " + std::to_string(toId) + "}) " +
                            "CREATE (from)-[:" + relationshipType;

        if (!properties.empty())
        {
            query += " {";
            bool first = true;
            for (const auto& [key, value] : properties)
            {
                if (!first)
                    query += ", ";
                first = false;
                
                if (isPropertyBoolean(relationshipType, key))
                {
                    // Handle boolean values without quotes
                    std::string boolValue = (value == "true" || value == "1") ? "true" : "false";
                    query += key + ": " + boolValue;
                }
                else
                {
                    // Handle string values with quotes
                    query += key + ": '" + escapeString(value) + "'";
                }
            }
            query += "}";
        }

        query += "]->(to)";

        auto result = connection->query(query);
        if (!result->isSuccess())
        {
            // Only log errors for debugging - some relationships might legitimately fail
            // due to nodes not existing or schema mismatches in complex cases
            // llvm::errs() << "Schema-aware relationship query failed: " << result->getErrorMessage() << "\n";
        }
    }
}

void KuzuDatabase::flushOperations()
{
    if (!connection)
        return;

    // Execute any remaining batched operations
    if (!pendingQueries.empty() || !pendingRelationships.empty())
        executeBatch();

    // Commit any active transaction
    if (transactionActive)
        commitTransaction();
}

void KuzuDatabase::createSchema()
{
    if (!connection)
        return;

    try
    {
        // Create base ASTNode table
        executeSchemaQuery("CREATE NODE TABLE ASTNode("
                           "node_id INT64 PRIMARY KEY, "
                           "node_type STRING, "
                           "memory_address STRING, "
                           "source_file STRING, "
                           "start_line INT64, "
                           "start_column INT64, "
                           "end_line INT64, "
                           "end_column INT64, "
                           "is_implicit BOOLEAN, "
                           "raw_text STRING)",
                           "ASTNode");

        // Create Declaration table
        executeSchemaQuery("CREATE NODE TABLE Declaration("
                           "node_id INT64 PRIMARY KEY, "
                           "name STRING, "
                           "qualified_name STRING, "
                           "access_specifier STRING, "
                           "storage_class STRING, "
                           "is_definition BOOLEAN, "
                           "namespace_context STRING)",
                           "Declaration");

        // Create Type table
        executeSchemaQuery("CREATE NODE TABLE Type("
                           "node_id INT64 PRIMARY KEY, "
                           "type_name STRING, "
                           "canonical_type STRING, "
                           "size_bytes INT64, "
                           "is_const BOOLEAN, "
                           "is_volatile BOOLEAN, "
                           "is_builtin BOOLEAN)",
                           "Type");

        // Create enhanced Statement table
        executeSchemaQuery("CREATE NODE TABLE Statement("
                           "node_id INT64 PRIMARY KEY, "
                           "statement_kind STRING, "
                           "has_side_effects BOOLEAN, "
                           "is_compound BOOLEAN, "
                           "control_flow_type STRING, "
                           "condition_text STRING, "
                           "is_constexpr BOOLEAN)",
                           "Statement");

        // Create enhanced Expression table
        executeSchemaQuery("CREATE NODE TABLE Expression("
                           "node_id INT64 PRIMARY KEY, "
                           "expression_kind STRING, "
                           "value_category STRING, "
                           "literal_value STRING, "
                           "operator_kind STRING, "
                           "is_constexpr BOOLEAN, "
                           "evaluation_result STRING, "
                           "implicit_cast_kind STRING)",
                           "Expression");

        // Create Attribute table
        executeSchemaQuery("CREATE NODE TABLE Attribute("
                           "node_id INT64 PRIMARY KEY, "
                           "attribute_kind STRING, "
                           "attribute_value STRING)",
                           "Attribute");

        // Create TemplateParameter table for enhanced template system
        executeSchemaQuery("CREATE NODE TABLE TemplateParameter("
                           "node_id INT64 PRIMARY KEY, "
                           "parameter_kind STRING, "
                           "parameter_name STRING, "
                           "has_default_argument BOOLEAN, "
                           "default_argument_text STRING, "
                           "is_parameter_pack BOOLEAN)",
                           "TemplateParameter");

        // Create UsingDeclaration table for namespace and using declarations
        executeSchemaQuery("CREATE NODE TABLE UsingDeclaration("
                           "node_id INT64 PRIMARY KEY, "
                           "using_kind STRING, "
                           "target_name STRING, "
                           "introduces_name STRING, "
                           "scope_impact STRING)",
                           "UsingDeclaration");

        // Create relationship tables
        executeSchemaQuery("CREATE REL TABLE PARENT_OF("
                           "FROM ASTNode TO ASTNode, "
                           "child_index INT64, "
                           "relationship_kind STRING)",
                           "PARENT_OF");

        executeSchemaQuery("CREATE REL TABLE HAS_TYPE("
                           "FROM Declaration TO Type, "
                           "type_role STRING)",
                           "HAS_TYPE");

        executeSchemaQuery("CREATE REL TABLE REFERENCES("
                           "FROM ASTNode TO Declaration, "
                           "reference_kind STRING, "
                           "is_direct BOOLEAN)",
                           "REFERENCES");

        executeSchemaQuery("CREATE REL TABLE IN_SCOPE("
                           "FROM ASTNode TO Declaration, "
                           "scope_kind STRING)",
                           "IN_SCOPE");

        executeSchemaQuery("CREATE REL TABLE TEMPLATE_RELATION("
                           "FROM ASTNode TO Declaration, "
                           "relation_kind STRING, "
                           "specialization_type STRING)",
                           "TEMPLATE_RELATION");

        // Inheritance relationship tables
        executeSchemaQuery("CREATE REL TABLE INHERITS_FROM("
                           "FROM Declaration TO Declaration, "
                           "inheritance_type STRING, "
                           "is_virtual BOOLEAN, "
                           "base_access_path STRING)",
                           "INHERITS_FROM");

        executeSchemaQuery("CREATE REL TABLE OVERRIDES("
                           "FROM Declaration TO Declaration, "
                           "override_type STRING, "
                           "is_covariant_return BOOLEAN)",
                           "OVERRIDES");

        // Enhanced template relationship table for detailed specializations
        executeSchemaQuery("CREATE REL TABLE SPECIALIZES("
                           "FROM Declaration TO Declaration, "
                           "specialization_kind STRING, "
                           "template_arguments STRING, "
                           "instantiation_context STRING)",
                           "SPECIALIZES");

        // Preprocessor and Macro Information tables
        executeSchemaQuery("CREATE NODE TABLE MacroDefinition("
                           "node_id INT64 PRIMARY KEY, "
                           "macro_name STRING, "
                           "is_function_like BOOLEAN, "
                           "parameter_count INT64, "
                           "parameter_names STRING, "
                           "replacement_text STRING, "
                           "is_builtin BOOLEAN, "
                           "is_conditional BOOLEAN)",
                           "MacroDefinition");

        executeSchemaQuery("CREATE NODE TABLE IncludeDirective("
                           "node_id INT64 PRIMARY KEY, "
                           "include_path STRING, "
                           "is_system_include BOOLEAN, "
                           "is_angled BOOLEAN, "
                           "resolved_path STRING, "
                           "include_depth INT64)",
                           "IncludeDirective");

        executeSchemaQuery("CREATE NODE TABLE ConditionalDirective("
                           "node_id INT64 PRIMARY KEY, "
                           "directive_type STRING, "
                           "condition_text STRING, "
                           "is_true_branch BOOLEAN, "
                           "nesting_level INT64)",
                           "ConditionalDirective");

        executeSchemaQuery("CREATE NODE TABLE PragmaDirective("
                           "node_id INT64 PRIMARY KEY, "
                           "pragma_name STRING, "
                           "pragma_text STRING, "
                           "pragma_kind STRING)",
                           "PragmaDirective");

        // Create Comment table for documentation and comments
        executeSchemaQuery("CREATE NODE TABLE Comment("
                           "node_id INT64 PRIMARY KEY, "
                           "comment_text STRING, "
                           "comment_kind STRING, "
                           "is_documentation BOOLEAN, "
                           "brief_text STRING, "
                           "detailed_text STRING)",
                           "Comment");

        // Create ConstantExpression table for compile-time evaluation tracking
        executeSchemaQuery("CREATE NODE TABLE ConstantExpression("
                           "node_id INT64 PRIMARY KEY, "
                           "is_constexpr_function BOOLEAN, "
                           "evaluation_context STRING, "
                           "evaluation_result STRING, "
                           "result_type STRING, "
                           "is_compile_time_constant BOOLEAN, "
                           "constant_value STRING, "
                           "constant_type STRING, "
                           "evaluation_status STRING)",
                           "ConstantExpression");

        // Create TemplateMetaprogramming table for template evaluation
        executeSchemaQuery("CREATE NODE TABLE TemplateMetaprogramming("
                           "node_id INT64 PRIMARY KEY, "
                           "template_kind STRING, "
                           "instantiation_depth INT64, "
                           "template_arguments STRING, "
                           "specialized_template_id INT64, "
                           "metaprogram_result STRING, "
                           "dependent_types STRING, "
                           "substitution_failure_reason STRING)",
                           "TemplateMetaprogramming");

        // Create StaticAssertion table for static_assert tracking
        executeSchemaQuery("CREATE NODE TABLE StaticAssertion("
                           "node_id INT64 PRIMARY KEY, "
                           "assertion_expression STRING, "
                           "assertion_message STRING, "
                           "assertion_result BOOLEAN, "
                           "failure_reason STRING, "
                           "evaluation_context STRING)",
                           "StaticAssertion");

        // Create CFGBlock table for control flow graph blocks
        executeSchemaQuery("CREATE NODE TABLE CFGBlock("
                           "node_id INT64 PRIMARY KEY, "
                           "function_id INT64, "
                           "block_index INT64, "
                           "is_entry_block BOOLEAN, "
                           "is_exit_block BOOLEAN, "
                           "terminator_kind STRING, "
                           "block_content STRING, "
                           "condition_expression STRING, "
                           "has_terminator BOOLEAN, "
                           "reachable BOOLEAN)",
                           "CFGBlock");

        // Preprocessor relationships
        executeSchemaQuery("CREATE REL TABLE MACRO_EXPANSION("
                           "FROM ASTNode TO MacroDefinition, "
                           "expansion_context STRING, "
                           "expansion_arguments STRING)",
                           "MACRO_EXPANSION");

        executeSchemaQuery("CREATE REL TABLE INCLUDES("
                           "FROM ASTNode TO IncludeDirective, "
                           "include_order INT64)",
                           "INCLUDES");

        executeSchemaQuery("CREATE REL TABLE DEFINES("
                           "FROM ASTNode TO MacroDefinition, "
                           "definition_context STRING)",
                           "DEFINES");

        executeSchemaQuery("CREATE REL TABLE HAS_COMMENT("
                           "FROM Declaration TO Comment, "
                           "attachment_type STRING)",
                           "HAS_COMMENT");

        // Constant expression and compile-time evaluation relationships
        executeSchemaQuery("CREATE REL TABLE HAS_CONSTANT_VALUE("
                           "FROM Expression TO ConstantExpression, "
                           "evaluation_stage STRING)",
                           "HAS_CONSTANT_VALUE");

        executeSchemaQuery("CREATE REL TABLE TEMPLATE_EVALUATES_TO("
                           "FROM Declaration TO TemplateMetaprogramming, "
                           "instantiation_context STRING)",
                           "TEMPLATE_EVALUATES_TO");

        executeSchemaQuery("CREATE REL TABLE CONTAINS_STATIC_ASSERT("
                           "FROM Declaration TO StaticAssertion, "
                           "assertion_scope STRING)",
                           "CONTAINS_STATIC_ASSERT");

        // Control Flow Graph relationships
        executeSchemaQuery("CREATE REL TABLE CFG_EDGE("
                           "FROM CFGBlock TO CFGBlock, "
                           "edge_type STRING, "
                           "condition STRING)",
                           "CFG_EDGE");

        executeSchemaQuery("CREATE REL TABLE CONTAINS_CFG("
                           "FROM Declaration TO CFGBlock, "
                           "cfg_role STRING)",
                           "CONTAINS_CFG");

        executeSchemaQuery("CREATE REL TABLE CFG_CONTAINS_STMT("
                           "FROM CFGBlock TO Statement, "
                           "statement_index INT64)",
                           "CFG_CONTAINS_STMT");
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to create database schema: " + std::string(e.what()));
    }
}

auto KuzuDatabase::escapeString(const std::string& str) -> std::string
{
    std::string escaped = str;

    // Escape backslashes first (for Windows paths)
    std::string::size_type pos = 0;
    while ((pos = escaped.find('\\', pos)) != std::string::npos)
    {
        escaped.replace(pos, 1, "\\\\");
        pos += 2;  // Move past the escaped backslash
    }

    // Escape single quotes
    pos = 0;
    while ((pos = escaped.find('\'', pos)) != std::string::npos)
    {
        escaped.replace(pos, 1, "\\'");
        pos += 2;  // Move past the escaped quote
    }

    return escaped;
}

void KuzuDatabase::enableCSVBulkMode(const std::string& directory)
{
    csvBulkMode = true;
    csvDirectory = directory;
    pendingCSVNodes.clear();
    csvFilesCreated.clear();
    
    // Create CSV directory if it doesn't exist
    std::filesystem::create_directories(directory);
}

void KuzuDatabase::disableCSVBulkMode()
{
    if (!csvBulkMode)
        return;
        
    // Write any pending CSV nodes
    for (const auto& [nodeType, nodes] : pendingCSVNodes)
    {
        if (!nodes.empty())
            writeNodesToCSV(nodeType, nodes);
    }
    
    // Import all CSV files
    importCSVFiles();
    
    csvBulkMode = false;
    pendingCSVNodes.clear();
    csvFilesCreated.clear();
}

void KuzuDatabase::writeNodesToCSV(const std::string& nodeType, const std::vector<std::map<std::string, std::string>>& nodes)
{
    if (nodes.empty())
        return;
        
    std::string csvFile = csvDirectory + "/" + nodeType + ".csv";
    bool fileExists = csvFilesCreated.find(csvFile) != csvFilesCreated.end();
    
    std::ofstream file(csvFile, fileExists ? std::ios::app : std::ios::out);
    if (!file.is_open())
    {
        llvm::errs() << "Failed to open CSV file: " << csvFile << "\n";
        return;
    }
    
    // Write header if new file
    if (!fileExists)
    {
        // Get column names from first node
        bool first = true;
        for (const auto& [key, _] : nodes[0])
        {
            if (!first) file << ",";
            first = false;
            file << key;
        }
        file << "\n";
        csvFilesCreated.insert(csvFile);
    }
    
    // Write data rows
    for (const auto& node : nodes)
    {
        bool first = true;
        for (const auto& [_, value] : node)
        {
            if (!first) file << ",";
            first = false;
            
            // Quote strings that contain commas or quotes
            if (value.find(',') != std::string::npos || value.find('"') != std::string::npos)
            {
                file << "\"";
                // Escape quotes by doubling them
                for (char c : value)
                {
                    if (c == '"') file << "\"\"";
                    else file << c;
                }
                file << "\"";
            }
            else
            {
                file << value;
            }
        }
        file << "\n";
    }
    
    file.close();
}

void KuzuDatabase::importCSVFiles()
{
    if (!connection || csvFilesCreated.empty())
        return;
        
    try
    {
        beginTransaction();
        
        for (const auto& csvFile : csvFilesCreated)
        {
            // Extract table name from file path
            std::filesystem::path filePath(csvFile);
            std::string tableName = filePath.stem().string();
            
            // Use COPY FROM for bulk import
            std::string copyQuery = "COPY " + tableName + " FROM '" + csvFile + "' (HEADER=true)";
            
            auto result = connection->query(copyQuery);
            if (!result->isSuccess())
            {
                llvm::errs() << "CSV import failed for " << csvFile << ": " << result->getErrorMessage() << "\n";
            }
            else
            {
                llvm::outs() << "Successfully imported " << csvFile << " into " << tableName << "\n";
            }
        }
        
        commitTransaction();
        
        // Clean up CSV files after successful import
        for (const auto& csvFile : csvFilesCreated)
        {
            std::filesystem::remove(csvFile);
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception during CSV import: " << e.what() << "\n";
        rollbackTransaction();
    }
}

void KuzuDatabase::initializeRelationshipSchemaInfo()
{
    // Map relationship types to their FROM/TO node types
    relationshipNodeTypes = {
        {"PARENT_OF", {"ASTNode", "ASTNode"}},
        {"HAS_TYPE", {"Declaration", "Type"}},
        {"REFERENCES", {"ASTNode", "Declaration"}},
        {"IN_SCOPE", {"ASTNode", "Declaration"}},
        {"TEMPLATE_RELATION", {"ASTNode", "Declaration"}},
        {"INHERITS_FROM", {"Declaration", "Declaration"}},
        {"OVERRIDES", {"Declaration", "Declaration"}},
        {"SPECIALIZES", {"Declaration", "Declaration"}},
        {"MACRO_EXPANSION", {"ASTNode", "MacroDefinition"}},
        {"INCLUDES", {"ASTNode", "IncludeDirective"}},
        {"DEFINES", {"ASTNode", "MacroDefinition"}},
        {"HAS_COMMENT", {"Declaration", "Comment"}},
        {"HAS_CONSTANT_VALUE", {"Expression", "ConstantExpression"}},
        {"TEMPLATE_EVALUATES_TO", {"Declaration", "TemplateMetaprogramming"}},
        {"CONTAINS_STATIC_ASSERT", {"Declaration", "StaticAssertion"}},
        {"CFG_EDGE", {"CFGBlock", "CFGBlock"}},
        {"CONTAINS_CFG", {"Declaration", "CFGBlock"}},
        {"CFG_CONTAINS_STMT", {"CFGBlock", "Statement"}}
    };
    
    // Map relationship types to their boolean properties
    relationshipBooleanProperties = {
        {"REFERENCES", {"is_direct"}},
        {"INHERITS_FROM", {"is_virtual"}},
        {"OVERRIDES", {"is_covariant_return"}},
        {"CFGBlock", {"is_entry_block", "is_exit_block", "has_terminator", "reachable"}}  // For node properties
    };
}

std::pair<std::string, std::string> KuzuDatabase::getRelationshipNodeTypes(const std::string& relationshipType)
{
    auto it = relationshipNodeTypes.find(relationshipType);
    if (it != relationshipNodeTypes.end())
        return it->second;
    
    // Default to ASTNode for unknown relationships
    return {"ASTNode", "ASTNode"};
}

bool KuzuDatabase::isPropertyBoolean(const std::string& relationshipType, const std::string& propertyName)
{
    auto it = relationshipBooleanProperties.find(relationshipType);
    if (it != relationshipBooleanProperties.end())
        return it->second.count(propertyName) > 0;
    return false;
}

void KuzuDatabase::initializeConnectionPool()
{
    if (!database)
        return;

    try
    {
        std::lock_guard<std::mutex> lock(connectionPoolMutex);

        // Create multiple connections for the pool
        for (size_t i = 0; i < CONNECTION_POOL_SIZE; ++i)
        {
            auto pooledConnection = std::make_unique<kuzu::main::Connection>(database.get());
            connectionPool.push(std::move(pooledConnection));
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Failed to initialize connection pool: " << e.what() << "\n";
        // If pool initialization fails, we can still use the main connection
    }
}

auto KuzuDatabase::getPooledConnection() -> kuzu::main::Connection*
{
    std::lock_guard<std::mutex> lock(connectionPoolMutex);

    if (!connectionPool.empty())
    {
        // Get a connection from the pool
        auto conn = connectionPool.front().get();
        connectionPool.pop();

        // Return the connection to the pool after use (this is simplified - in a real implementation
        // you'd want a RAII wrapper to automatically return the connection)
        connectionPool.push(std::unique_ptr<kuzu::main::Connection>(conn));
        return conn;
    }

    // Fallback to main connection if pool is empty
    return connection.get();
}

void KuzuDatabase::optimizeTransactionBoundaries()
{
    if (!connection)
        return;

    try
    {
        // Commit current transaction and immediately start a new one for better performance
        if (transactionActive)
        {
            commitTransaction();
            operationsSinceLastCommit = 0;

            // Immediately start a new transaction if we have pending operations
            if (!pendingQueries.empty() || !pendingRelationships.empty())
                beginTransaction();
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception optimizing transaction boundaries: " << e.what() << "\n";
    }
}
