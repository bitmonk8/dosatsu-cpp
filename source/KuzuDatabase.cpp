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

#include <filesystem>
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

        // Create schema
        createSchema();
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
        return;

    try
    {
        auto result = connection->query("COMMIT");
        if (result->isSuccess())
            transactionActive = false;
        else
            llvm::errs() << "Failed to commit transaction: " << result->getErrorMessage() << "\n";
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception committing transaction: " << e.what() << "\n";
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

    // Start transaction on first batched operation
    if (!transactionActive)
        beginTransaction();

    // Execute batch when it reaches the batch size
    if (pendingQueries.size() >= BATCH_SIZE)
        executeBatch();
}

void KuzuDatabase::executeBatch()
{
    if (!connection || pendingQueries.empty())
        return;

    try
    {
        // Execute all queries in the batch
        for (const auto& query : pendingQueries)
        {
            auto result = connection->query(query);
            if (!result->isSuccess())
            {
                llvm::errs() << "Batched query failed: " << result->getErrorMessage() << "\n";
                llvm::errs() << "Query: " << query << "\n";
                // Continue with other queries rather than failing completely
            }
        }

        pendingQueries.clear();
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception executing batch: " << e.what() << "\n";
        pendingQueries.clear();
    }
}

void KuzuDatabase::flushOperations()
{
    if (!connection)
        return;

    // Execute any remaining batched operations
    if (!pendingQueries.empty())
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
