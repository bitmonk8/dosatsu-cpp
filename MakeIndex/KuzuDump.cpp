//===--- KuzuDump.cpp - Dumping implementation for ASTs ------------------===//
//
// Based on LLVM Project's ASTDumper, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the AST dump methods, which dump out the
// AST in a form that exposes type details and other fields.
//
//===----------------------------------------------------------------------===//

#include "KuzuDump.h"

// clang-format off
#include "NoWarningScope_Enter.h"

#include "clang/AST/ASTConcept.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclLookups.h"
#include "clang/AST/JSONNodeDumper.h"
#include "clang/AST/Expr.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"

#include "kuzu.hpp"

#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>
#include <filesystem>
#include <ranges>
#include <sstream>
#include <stdexcept>

using namespace clang;
using namespace clang::comments;

void KuzuDump::dumpInvalidDeclContext(const DeclContext* DC)
{
    // Phase 4.3: Skip text dumping in database-only mode
    if (databaseOnlyMode)
    {
        return;
    }

    NodeDumper.AddChild(
        [=]
        {
            if (!DC)
            {
                ColorScope Color(OS, ShowColors, NullColor);
                OS << "<<<NULL>>>";
                return;
            }
            // An invalid DeclContext is one for which a dyn_cast() from a DeclContext
            // pointer to a Decl pointer would fail an assertion or otherwise fall prey
            // to undefined behavior as a result of an invalid associated DeclKind.
            // Such invalidity is not supposed to happen of course, but, when it does,
            // the information provided below is intended to provide some hints about
            // what might have gone awry.
            {
                ColorScope Color(OS, ShowColors, DeclKindNameColor);
                OS << "DeclContext";
            }
            NodeDumper.dumpPointer(DC);
            OS << " <";
            {
                ColorScope Color(OS, ShowColors, DeclNameColor);
                OS << "unrecognized Decl kind " << (unsigned)DC->getDeclKind();
            }
            OS << ">";
        });
}

void KuzuDump::dumpLookups(const DeclContext* DC, bool DumpDecls)
{
    // Phase 4.3: Skip text dumping in database-only mode
    if (databaseOnlyMode)
    {
        return;
    }

    NodeDumper.AddChild(
        [=]
        {
            OS << "StoredDeclsMap ";
            NodeDumper.dumpBareDeclRef(cast<Decl>(DC));

            const DeclContext* Primary = DC->getPrimaryContext();
            if (Primary != DC)
            {
                OS << " primary";
                NodeDumper.dumpPointer(cast<Decl>(Primary));
            }

            bool HasUndeserializedLookups = Primary->hasExternalVisibleStorage();

            auto Range =
                getDeserialize() ? Primary->lookups() : Primary->noload_lookups(/*PreserveInternalState=*/true);
            for (auto I = Range.begin(), E = Range.end(); I != E; ++I)
            {
                DeclarationName Name = I.getLookupName();
                DeclContextLookupResult R = *I;

                NodeDumper.AddChild(
                    [=]
                    {
                        OS << "DeclarationName ";
                        {
                            ColorScope Color(OS, ShowColors, DeclNameColor);
                            OS << '\'' << Name << '\'';
                        }

                        for (auto lookupResult : R)
                        {
                            NodeDumper.AddChild(
                                [=]
                                {
                                    NodeDumper.dumpBareDeclRef(lookupResult);

                                    if (!lookupResult->isUnconditionallyVisible())
                                        OS << " hidden";

                                    // If requested, dump the redecl chain for this lookup.
                                    if (DumpDecls)
                                    {
                                        // Dump earliest decl first.
                                        std::function<void(Decl*)> DumpWithPrev = [&](Decl* D)
                                        {
                                            if (Decl* Prev = D->getPreviousDecl())
                                                DumpWithPrev(Prev);
                                            Visit(D);
                                        };
                                        DumpWithPrev(lookupResult);
                                    }
                                });
                        }
                    });
            }

            if (HasUndeserializedLookups)
            {
                NodeDumper.AddChild(
                    [=]
                    {
                        ColorScope Color(OS, ShowColors, UndeserializedColor);
                        OS << "<undeserialized lookups>";
                    });
            }
        });
}

template <typename SpecializationDecl>
void KuzuDump::dumpTemplateDeclSpecialization(const SpecializationDecl* D, bool DumpExplicitInst, bool DumpRefOnly)
{
    // Phase 4.3: Skip text dumping in database-only mode but still process AST
    if (databaseOnlyMode)
    {
        for (const auto* RedeclWithBadType : D->redecls())
        {
            auto* Redecl = cast<SpecializationDecl>(RedeclWithBadType);
            switch (Redecl->getTemplateSpecializationKind())
            {
            case TSK_ExplicitInstantiationDeclaration:
            case TSK_ExplicitInstantiationDefinition:
                if (!DumpExplicitInst)
                    break;
                [[fallthrough]];
            case TSK_Undeclared:
            case TSK_ImplicitInstantiation:
                if (!DumpRefOnly)
                    Visit(Redecl);
                break;
            case TSK_ExplicitSpecialization:
                break;
            }
        }
        return;
    }

    bool DumpedAny = false;
    for (const auto* RedeclWithBadType : D->redecls())
    {
        // FIXME: The redecls() range sometimes has elements of a less-specific
        // type. (In particular, ClassTemplateSpecializationDecl::redecls() gives
        // us TagDecls, and should give CXXRecordDecls).
        auto* Redecl = cast<SpecializationDecl>(RedeclWithBadType);
        switch (Redecl->getTemplateSpecializationKind())
        {
        case TSK_ExplicitInstantiationDeclaration:
        case TSK_ExplicitInstantiationDefinition:
            if (!DumpExplicitInst)
                break;
            [[fallthrough]];
        case TSK_Undeclared:
        case TSK_ImplicitInstantiation:
            if (DumpRefOnly)
                NodeDumper.dumpDeclRef(Redecl);
            else
                Visit(Redecl);
            DumpedAny = true;
            break;
        case TSK_ExplicitSpecialization:
            break;
        }
    }

    // Ensure we dump at least one decl for each specialization.
    if (!DumpedAny)
        NodeDumper.dumpDeclRef(D);
}

template <typename TemplateDecl>
void KuzuDump::dumpTemplateDecl(const TemplateDecl* D, bool DumpExplicitInst)
{
    dumpTemplateParameters(D->getTemplateParameters());

    Visit(D->getTemplatedDecl());

    if (GetTraversalKind() == TK_AsIs)
    {
        for (const auto* Child : D->specializations())
            dumpTemplateDeclSpecialization(Child, DumpExplicitInst, !D->isCanonicalDecl());
    }
}

void KuzuDump::VisitFunctionTemplateDecl(const FunctionTemplateDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this function template declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this template (it's in its parent scope)
    createScopeRelationships(nodeId);

    // Push this template as a new scope for its contents
    pushScope(nodeId);

    // FIXME: We don't add a declaration of a function template specialization
    // to its context when it's explicitly instantiated, so dump explicit
    // instantiations when we dump the template itself.
    dumpTemplateDecl(D, true);

    // Pop this template scope after traversal
    popScope();
}

void KuzuDump::VisitClassTemplateDecl(const ClassTemplateDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this class template declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this template (it's in its parent scope)
    createScopeRelationships(nodeId);

    // Push this template as a new scope for its contents
    pushScope(nodeId);

    dumpTemplateDecl(D, false);

    // Pop this template scope after traversal
    popScope();
}

void KuzuDump::VisitVarTemplateDecl(const VarTemplateDecl* D)
{
    dumpTemplateDecl(D, false);
}

void KuzuDump::initializeDatabase()
{
    if (!databaseEnabled || databasePath.empty())
    {
        return;
    }

    try
    {
        // Create database directory if it doesn't exist
        std::filesystem::path dbPath(databasePath);
        if (dbPath.has_parent_path())
        {
            std::filesystem::create_directories(dbPath.parent_path());
        }

        // Initialize Kuzu database
        database = std::make_unique<kuzu::main::Database>(databasePath);
        connection = std::make_unique<kuzu::main::Connection>(database.get());

        // Create schema
        createSchema();

        llvm::outs() << "Database initialized at: " << databasePath << "\n";
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to initialize Kuzu database: " + std::string(e.what()));
    }
}

void KuzuDump::executeSchemaQuery(const std::string& query, const std::string& schemaName)
{
    auto result = connection->query(query);
    if (!result->isSuccess())
    {
        throw std::runtime_error("Failed to create " + schemaName + " table: " + result->getErrorMessage());
    }
}

// Performance optimization methods (Phase 4)
void KuzuDump::beginTransaction()
{
    if (!connection || transactionActive)
    {
        return;
    }

    try
    {
        auto result = connection->query("BEGIN TRANSACTION");
        if (result->isSuccess())
        {
            transactionActive = true;
            llvm::outs() << "Transaction started for batch operations\n";
        }
        else
        {
            llvm::errs() << "Failed to begin transaction: " << result->getErrorMessage() << "\n";
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception beginning transaction: " << e.what() << "\n";
    }
}

void KuzuDump::commitTransaction()
{
    if (!connection || !transactionActive)
    {
        return;
    }

    try
    {
        auto result = connection->query("COMMIT");
        if (result->isSuccess())
        {
            transactionActive = false;
            llvm::outs() << "Transaction committed. Total operations: " << totalOperations << "\n";
        }
        else
        {
            llvm::errs() << "Failed to commit transaction: " << result->getErrorMessage() << "\n";
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception committing transaction: " << e.what() << "\n";
    }
}

void KuzuDump::rollbackTransaction()
{
    if (!connection || !transactionActive)
    {
        return;
    }

    try
    {
        auto result = connection->query("ROLLBACK");
        if (result->isSuccess())
        {
            transactionActive = false;
            llvm::outs() << "Transaction rolled back\n";
        }
        else
        {
            llvm::errs() << "Failed to rollback transaction: " << result->getErrorMessage() << "\n";
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception rolling back transaction: " << e.what() << "\n";
    }
}

void KuzuDump::addToBatch(const std::string& query)
{
    if (!connection || query.empty())
    {
        return;
    }

    pendingQueries.push_back(query);
    totalOperations++;

    // Start transaction on first batched operation
    if (!transactionActive)
    {
        beginTransaction();
    }

    // Execute batch when it reaches the batch size
    if (pendingQueries.size() >= BATCH_SIZE)
    {
        executeBatch();
    }
}

void KuzuDump::executeBatch()
{
    if (!connection || pendingQueries.empty())
    {
        return;
    }

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

        llvm::outs() << "Executed batch of " << pendingQueries.size() << " operations\n";
        pendingQueries.clear();
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception executing batch: " << e.what() << "\n";
        pendingQueries.clear();
    }
}

void KuzuDump::flushOperations()
{
    if (!connection)
    {
        return;
    }

    // Execute any remaining batched operations
    if (!pendingQueries.empty())
    {
        executeBatch();
    }

    // Commit any active transaction
    if (transactionActive)
    {
        commitTransaction();
    }

    llvm::outs() << "All operations flushed. Total operations processed: " << totalOperations << "\n";
}

void KuzuDump::createSchema()
{
    if (!connection)
    {
        return;
    }

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

        llvm::outs() << "Database schema created successfully\n";
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to create database schema: " + std::string(e.what()));
    }
}

// Basic node creation methods (Phase 1 - minimal implementation)
auto KuzuDump::createASTNode(const clang::Decl* decl) -> int64_t
{
    if (!connection || (decl == nullptr))
    {
        return -1;
    }

    // Check if already processed
    auto it = nodeIdMap.find(decl);
    if (it != nodeIdMap.end())
    {
        return it->second;
    }

    int64_t nodeId = nextNodeId++;
    nodeIdMap[decl] = nodeId;

    try
    {
        // Extract basic information
        std::string nodeType = extractNodeType(decl);

        // Format memory address as hex string
        std::stringstream addrStream;
        addrStream << std::hex << (uintptr_t)decl;
        std::string memoryAddr = addrStream.str();

        bool isImplicit = isImplicitNode(decl);

        // Extract detailed source location information
        auto [filename, startLine, startColumn] = extractSourceLocationDetailed(decl->getLocation());
        auto [endFilename, endLine, endColumn] = extractSourceLocationDetailed(decl->getSourceRange().getEnd());

        // Use start location's filename for source_file
        std::string sourceFile = filename;

        // Create base AST node query using string concatenation
        std::string query =
            "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
            "', is_implicit: " + (isImplicit ? "true" : "false") + ", start_line: " + std::to_string(startLine) +
            ", start_column: " + std::to_string(startColumn) + ", end_line: " + std::to_string(endLine) +
            ", end_column: " + std::to_string(endColumn) + ", raw_text: ''})";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);

        // Create specialized Declaration node if this is a named declaration
        if (const auto* namedDecl = dyn_cast<NamedDecl>(decl))
        {
            createDeclarationNode(nodeId, namedDecl);

            // Create type relationships for typed declarations
            if (const auto* valueDecl = dyn_cast<ValueDecl>(namedDecl))
            {
                createTypeNodeAndRelation(nodeId, valueDecl->getType());
            }
            else if (const auto* functionDecl = dyn_cast<FunctionDecl>(namedDecl))
            {
                createTypeNodeAndRelation(nodeId, functionDecl->getType());
            }
        }

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node: " << e.what() << "\n";
        return -1;
    }
}

auto KuzuDump::createASTNode(const clang::Stmt* stmt) -> int64_t
{
    if (!connection || (stmt == nullptr))
    {
        return -1;
    }

    // Check if already processed
    auto it = nodeIdMap.find(stmt);
    if (it != nodeIdMap.end())
    {
        return it->second;
    }

    int64_t nodeId = nextNodeId++;
    nodeIdMap[stmt] = nodeId;

    try
    {
        // Extract basic information
        std::string nodeType = extractNodeType(stmt);
        // Format memory address as hex string
        std::stringstream addrStream;
        addrStream << std::hex << (uintptr_t)stmt;
        std::string memoryAddr = addrStream.str();

        // Extract detailed source location information
        auto [filename, startLine, startColumn] = extractSourceLocationDetailed(stmt->getBeginLoc());
        auto [endFilename, endLine, endColumn] = extractSourceLocationDetailed(stmt->getEndLoc());

        // Use start location's filename for source_file
        std::string sourceFile = filename;

        // Create base AST node query using string concatenation
        std::string query = "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
                            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
                            "', is_implicit: false, start_line: " + std::to_string(startLine) +
                            ", start_column: " + std::to_string(startColumn) +
                            ", end_line: " + std::to_string(endLine) + ", end_column: " + std::to_string(endColumn) +
                            ", raw_text: ''})";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node for statement: " << e.what() << "\n";
        return -1;
    }
}

auto KuzuDump::createASTNode(const clang::Type* type) -> int64_t
{
    if (!connection || (type == nullptr))
    {
        return -1;
    }

    // Check if already processed
    auto it = nodeIdMap.find(type);
    if (it != nodeIdMap.end())
    {
        return it->second;
    }

    int64_t nodeId = nextNodeId++;
    nodeIdMap[type] = nodeId;

    try
    {
        // Extract basic information
        std::string nodeType = extractNodeType(type);
        // Format memory address as hex string
        std::stringstream addrStream;
        addrStream << std::hex << (uintptr_t)type;
        std::string memoryAddr = addrStream.str();

        // Types don't have specific source locations, so use empty values
        std::string sourceFile = "";
        int64_t startLine = -1;
        int64_t startColumn = -1;
        int64_t endLine = -1;
        int64_t endColumn = -1;

        // Create base AST node query using string concatenation
        std::string query = "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
                            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
                            "', is_implicit: false, start_line: " + std::to_string(startLine) +
                            ", start_column: " + std::to_string(startColumn) +
                            ", end_line: " + std::to_string(endLine) + ", end_column: " + std::to_string(endColumn) +
                            ", raw_text: ''})";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node for type: " << e.what() << "\n";
        return -1;
    }
}

// Relationship creation methods (Phase 2 & 3)
void KuzuDump::createParentChildRelation(int64_t parentId, int64_t childId, int index)
{
    if (!connection || parentId == -1 || childId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (parent:ASTNode {node_id: " + std::to_string(parentId) + "}), " +
                            "(child:ASTNode {node_id: " + std::to_string(childId) + "}) " +
                            "CREATE (parent)-[:PARENT_OF {child_index: " + std::to_string(index) +
                            ", relationship_kind: 'child'}]->(child)";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating PARENT_OF relationship: " << e.what() << "\n";
    }
}

void KuzuDump::createTypeRelation(int64_t declId, int64_t typeId)
{
    if (!connection || declId == -1 || typeId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (decl:Declaration {node_id: " + std::to_string(declId) + "}), " +
                            "(type:Type {node_id: " + std::to_string(typeId) + "}) " +
                            "CREATE (decl)-[:HAS_TYPE {type_role: 'primary'}]->(type)";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating HAS_TYPE relationship: " << e.what() << "\n";
    }
}

void KuzuDump::createReferenceRelation(int64_t fromId, int64_t toId, const std::string& kind)
{
    if (!connection || fromId == -1 || toId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (from:ASTNode {node_id: " + std::to_string(fromId) + "}), " +
                            "(to:Declaration {node_id: " + std::to_string(toId) + "}) " +
                            "CREATE (from)-[:REFERENCES {reference_kind: '" + kind + "', is_direct: true}]->(to)";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating REFERENCES relationship: " << e.what() << "\n";
    }
}

void KuzuDump::createScopeRelation(int64_t nodeId, int64_t scopeId, const std::string& scopeKind)
{
    if (!connection || nodeId == -1 || scopeId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (node:ASTNode {node_id: " + std::to_string(nodeId) + "}), " +
                            "(scope:Declaration {node_id: " + std::to_string(scopeId) + "}) " +
                            "CREATE (node)-[:IN_SCOPE {scope_kind: '" + scopeKind + "'}]->(scope)";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating IN_SCOPE relationship: " << e.what() << "\n";
    }
}

// Type processing methods (Phase 2)
auto KuzuDump::createTypeNodeAndRelation(int64_t declNodeId, clang::QualType qualType) -> int64_t
{
    if (!connection || qualType.isNull())
    {
        return -1;
    }

    int64_t typeNodeId = createTypeNode(qualType);
    if (typeNodeId != -1)
    {
        createTypeRelation(declNodeId, typeNodeId);
    }

    return typeNodeId;
}

auto KuzuDump::createTypeNode(clang::QualType qualType) -> int64_t
{
    if (!connection || qualType.isNull())
    {
        return -1;
    }

    try
    {
        int64_t typeNodeId = nextNodeId++;

        std::string typeName = extractTypeName(qualType);
        std::string typeCategory = extractTypeCategory(qualType);
        std::string qualifiers = extractTypeQualifiers(qualType);
        bool isBuiltIn = isBuiltInType(qualType);

        std::string query = "CREATE (t:Type {node_id: " + std::to_string(typeNodeId) + ", type_name: '" + typeName +
                            "', canonical_type: '" + typeCategory +
                            "', size_bytes: -1, is_const: " + (qualType.isConstQualified() ? "true" : "false") +
                            ", is_volatile: " + (qualType.isVolatileQualified() ? "true" : "false") +
                            ", is_builtin: " + (isBuiltIn ? "true" : "false") + "})";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);

        return typeNodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Type node: " << e.what() << "\n";
        return -1;
    }
}

// Enhanced declaration processing methods (Phase 2)
void KuzuDump::createDeclarationNode(int64_t nodeId, const clang::NamedDecl* decl)
{
    if (!connection || (decl == nullptr))
    {
        return;
    }

    try
    {
        std::string name = decl->getNameAsString();
        std::string qualifiedName = extractQualifiedName(decl);
        std::string accessSpec = extractAccessSpecifier(decl);
        std::string storageClass = extractStorageClass(decl);
        std::string namespaceContext = extractNamespaceContext(decl);
        bool isDef = isDefinition(decl);

        // Create Declaration node with extracted properties
        std::string query = "CREATE (d:Declaration {node_id: " + std::to_string(nodeId) + ", name: '" + name +
                            "', qualified_name: '" + qualifiedName + "', access_specifier: '" + accessSpec +
                            "', storage_class: '" + storageClass + "', is_definition: " + (isDef ? "true" : "false") +
                            ", namespace_context: '" + namespaceContext + "'})";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Declaration node: " << e.what() << "\n";
    }
}

auto KuzuDump::extractQualifiedName(const clang::NamedDecl* decl) -> std::string
{
    if (decl == nullptr)
    {
        return "";
    }

    std::string qualifiedName = decl->getQualifiedNameAsString();
    // Replace any problematic characters for database storage
    std::ranges::replace(qualifiedName, '\'', '_');
    return qualifiedName;
}

auto KuzuDump::extractAccessSpecifier(const clang::Decl* decl) -> std::string
{
    if (decl == nullptr)
    {
        return "none";
    }

    switch (decl->getAccess())
    {
    case AS_public:
        return "public";
    case AS_protected:
        return "protected";
    case AS_private:
        return "private";
    case AS_none:
    default:
        return "none";
    }
}

auto KuzuDump::extractStorageClass(const clang::Decl* decl) -> std::string
{
    if (const auto* varDecl = dyn_cast<VarDecl>(decl))
    {
        switch (varDecl->getStorageClass())
        {
        case SC_None:
            return "none";
        case SC_Static:
            return "static";
        case SC_Extern:
            return "extern";
        case SC_Auto:
            return "auto";
        case SC_Register:
            return "register";
        default:
            return "unknown";
        }
    }
    else if (const auto* funcDecl = dyn_cast<FunctionDecl>(decl))
    {
        switch (funcDecl->getStorageClass())
        {
        case SC_None:
            return "none";
        case SC_Static:
            return "static";
        case SC_Extern:
            return "extern";
        default:
            return "unknown";
        }
    }

    return "none";
}

auto KuzuDump::extractNamespaceContext(const clang::Decl* decl) -> std::string
{
    if (decl == nullptr)
    {
        return "";
    }

    const DeclContext* context = decl->getDeclContext();
    std::vector<std::string> namespaces;

    while ((context != nullptr) && !context->isTranslationUnit())
    {
        if (const auto* nsDecl = dyn_cast<NamespaceDecl>(context))
        {
            if (!nsDecl->isAnonymousNamespace())
            {
                namespaces.push_back(nsDecl->getNameAsString());
            }
        }
        else if (const auto* recordDecl = dyn_cast<RecordDecl>(context))
        {
            if (recordDecl->getIdentifier() != nullptr)
            {
                namespaces.push_back(recordDecl->getNameAsString());
            }
        }
        context = context->getParent();
    }

    std::ranges::reverse(namespaces);

    std::string result;
    for (size_t i = 0; i < namespaces.size(); ++i)
    {
        if (i > 0)
            result += "::";
        result += namespaces[i];
    }

    return result;
}

auto KuzuDump::isDefinition(const clang::Decl* decl) -> bool
{
    if (decl == nullptr)
    {
        return false;
    }

    if (const auto* funcDecl = dyn_cast<FunctionDecl>(decl))
    {
        return funcDecl->isThisDeclarationADefinition();
    }
    if (const auto* varDecl = dyn_cast<VarDecl>(decl))
    {
        return varDecl->isThisDeclarationADefinition() != 0;
    }
    if (const auto* recordDecl = dyn_cast<RecordDecl>(decl))
    {
        return recordDecl->isThisDeclarationADefinition();
    }

    return false;
}

// Data extraction utility methods (enhanced for Phase 2)
auto KuzuDump::extractSourceLocation(const clang::SourceLocation& loc) -> std::string
{
    if (loc.isInvalid())
    {
        return "<invalid>";
    }

    // Get detailed location information
    auto [fileName, line, column] = extractSourceLocationDetailed(loc);

    // Return formatted location string
    if (fileName == "<invalid>" || line == -1 || column == -1)
    {
        return "<unknown_location>";
    }

    return fileName + ":" + std::to_string(line) + ":" + std::to_string(column);
}

auto KuzuDump::extractSourceLocationDetailed(const clang::SourceLocation& loc)
    -> std::tuple<std::string, int64_t, int64_t>
{
    if (loc.isInvalid() || (sourceManager == nullptr))
    {
        return std::make_tuple("<invalid>", -1, -1);
    }

    try
    {
        // Use SourceManager to get precise location information
        auto presumedLoc = sourceManager->getPresumedLoc(loc);
        if (presumedLoc.isInvalid())
        {
            return std::make_tuple("<invalid>", -1, -1);
        }

        // Extract filename, line, and column from the presumed location
        std::string filename = (presumedLoc.getFilename() != nullptr) ? presumedLoc.getFilename() : "<unknown>";
        int64_t line = presumedLoc.getLine();
        int64_t column = presumedLoc.getColumn();

        // Clean up filename for database storage (escape single quotes)
        std::ranges::replace(filename, '\'', '_');

        return std::make_tuple(filename, line, column);
    }
    catch (...)
    {
        // If any exception occurs, return invalid location
        return std::make_tuple("<exception>", -1, -1);
    }
}

auto KuzuDump::extractNodeType(const clang::Decl* decl) -> std::string
{
    if (decl == nullptr)
    {
        return "UnknownDecl";
    }
    return decl->getDeclKindName();
}

auto KuzuDump::extractNodeType(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
    {
        return "UnknownStmt";
    }
    return stmt->getStmtClassName();
}

auto KuzuDump::extractNodeType(const clang::Type* type) -> std::string
{
    if (type == nullptr)
    {
        return "UnknownType";
    }
    return type->getTypeClassName();
}

auto KuzuDump::isImplicitNode(const clang::Decl* decl) -> bool
{
    if (decl == nullptr)
    {
        return false;
    }
    return decl->isImplicit();
}

// Type extraction utility methods (Phase 2)
auto KuzuDump::extractTypeName(clang::QualType qualType) -> std::string
{
    if (qualType.isNull())
    {
        return "<invalid_type>";
    }

    // Get the unqualified type name
    return qualType.getUnqualifiedType().getAsString();
}

auto KuzuDump::extractTypeCategory(clang::QualType qualType) -> std::string
{
    if (qualType.isNull())
    {
        return "invalid";
    }

    const clang::Type* type = qualType.getTypePtr();
    if (type == nullptr)
    {
        return "invalid";
    }

    if (type->isBuiltinType())
    {
        return "builtin";
    }
    if (type->isPointerType())
    {
        return "pointer";
    }
    if (type->isReferenceType())
    {
        return "reference";
    }
    if (type->isArrayType())
    {
        return "array";
    }
    if (type->isFunctionType())
    {
        return "function";
    }
    if (type->isRecordType())
    {
        return "record";
    }
    if (type->isEnumeralType())
    {
        return "enum";
    }
    if (type->isTemplateTypeParmType())
    {
        return "template_param";
    }

    return "user_defined";
}

auto KuzuDump::extractTypeQualifiers(clang::QualType qualType) -> std::string
{
    if (qualType.isNull())
    {
        return "";
    }

    std::string qualifiers;

    if (qualType.isConstQualified())
    {
        qualifiers += "const ";
    }
    if (qualType.isVolatileQualified())
    {
        qualifiers += "volatile ";
    }
    if (qualType.isRestrictQualified())
    {
        qualifiers += "restrict ";
    }

    // Remove trailing space
    if (!qualifiers.empty() && qualifiers.back() == ' ')
    {
        qualifiers.pop_back();
    }

    return qualifiers;
}

auto KuzuDump::isBuiltInType(clang::QualType qualType) -> bool
{
    if (qualType.isNull())
    {
        return false;
    }

    const clang::Type* type = qualType.getTypePtr();
    return (type != nullptr) && type->isBuiltinType();
}

auto KuzuDump::extractTypeSourceLocation(clang::QualType /*qualType*/) -> std::string
{
    // For now, types don't have specific source locations
    // This could be enhanced in the future to show where types are defined
    return "<type_location>";
}

// Enhanced statement and expression processing methods implementation
void KuzuDump::createStatementNode(int64_t nodeId, const clang::Stmt* stmt)
{
    if (!connection || (stmt == nullptr))
    {
        return;
    }

    try
    {
        std::string statementKind = extractStatementKind(stmt);
        std::string controlFlowType = extractControlFlowType(stmt);
        std::string conditionText = extractConditionText(stmt);
        bool hasSideEffects = hasStatementSideEffects(stmt);
        bool isCompound = isCompoundStatement(stmt);
        bool isConstexpr = isStatementConstexpr(stmt);

        // Escape single quotes in text fields
        std::ranges::replace(conditionText, '\'', '_');

        std::string query = "CREATE (s:Statement {node_id: " + std::to_string(nodeId) + ", statement_kind: '" +
                            statementKind + "', has_side_effects: " + (hasSideEffects ? "true" : "false") +
                            ", is_compound: " + (isCompound ? "true" : "false") + ", control_flow_type: '" +
                            controlFlowType + "', condition_text: '" + conditionText +
                            "', is_constexpr: " + (isConstexpr ? "true" : "false") + "})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Statement node: " << e.what() << "\n";
    }
}

void KuzuDump::createExpressionNode(int64_t nodeId, const clang::Expr* expr)
{
    if (!connection || (expr == nullptr))
    {
        return;
    }

    try
    {
        std::string expressionKind = extractExpressionKind(expr);
        std::string valueCategory = extractValueCategory(expr);
        std::string literalValue = extractLiteralValue(expr);
        std::string operatorKind = extractOperatorKind(expr);
        std::string evaluationResult = extractEvaluationResult(expr);
        std::string implicitCastKind = extractImplicitCastKind(expr);
        bool isConstexpr = isExpressionConstexpr(expr);

        // Escape single quotes in text fields
        std::ranges::replace(literalValue, '\'', '_');
        std::ranges::replace(evaluationResult, '\'', '_');

        std::string query = "CREATE (e:Expression {node_id: " + std::to_string(nodeId) + ", expression_kind: '" +
                            expressionKind + "', value_category: '" + valueCategory + "', literal_value: '" +
                            literalValue + "', operator_kind: '" + operatorKind +
                            "', is_constexpr: " + (isConstexpr ? "true" : "false") + ", evaluation_result: '" +
                            evaluationResult + "', implicit_cast_kind: '" + implicitCastKind + "'})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Expression node: " << e.what() << "\n";
    }
}

auto KuzuDump::extractStatementKind(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
    {
        return "unknown";
    }
    return stmt->getStmtClassName();
}

auto KuzuDump::extractControlFlowType(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
    {
        return "none";
    }

    if (isa<IfStmt>(stmt))
        return "if";
    if (isa<WhileStmt>(stmt))
        return "while";
    if (isa<ForStmt>(stmt))
        return "for";
    if (isa<DoStmt>(stmt))
        return "do";
    if (isa<SwitchStmt>(stmt))
        return "switch";
    if (isa<CaseStmt>(stmt))
        return "case";
    if (isa<DefaultStmt>(stmt))
        return "default";
    if (isa<BreakStmt>(stmt))
        return "break";
    if (isa<ContinueStmt>(stmt))
        return "continue";
    if (isa<ReturnStmt>(stmt))
        return "return";
    if (isa<GotoStmt>(stmt))
        return "goto";
    if (isa<LabelStmt>(stmt))
        return "label";

    return "none";
}

auto KuzuDump::extractConditionText(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
    {
        return "";
    }

    if (const auto* ifStmt = dyn_cast<IfStmt>(stmt))
    {
        if ([[maybe_unused]] const Expr* cond = ifStmt->getCond())
        {
            // For now, return a placeholder. In a full implementation, we'd need
            // a way to get the source text of the condition expression
            return "if_condition";
        }
    }
    else if (const auto* whileStmt = dyn_cast<WhileStmt>(stmt))
    {
        if ([[maybe_unused]] const Expr* cond = whileStmt->getCond())
        {
            return "while_condition";
        }
    }
    else if (const auto* forStmt = dyn_cast<ForStmt>(stmt))
    {
        if ([[maybe_unused]] const Expr* cond = forStmt->getCond())
        {
            return "for_condition";
        }
    }

    return "";
}

auto KuzuDump::hasStatementSideEffects(const clang::Stmt* stmt) -> bool
{
    if (stmt == nullptr)
    {
        return false;
    }

    // Most statements that can have side effects
    if (isa<CallExpr>(stmt) || isa<CXXOperatorCallExpr>(stmt))
        return true;
    if (isa<BinaryOperator>(stmt))
    {
        const auto* binOp = cast<BinaryOperator>(stmt);
        return binOp->isAssignmentOp() || binOp->isCompoundAssignmentOp();
    }
    if (isa<UnaryOperator>(stmt))
    {
        const auto* unaryOp = cast<UnaryOperator>(stmt);
        return unaryOp->isIncrementDecrementOp();
    }
    if (isa<CXXNewExpr>(stmt) || isa<CXXDeleteExpr>(stmt))
        return true;
    if (isa<CXXThrowExpr>(stmt))
        return true;

    return false;
}

auto KuzuDump::isCompoundStatement(const clang::Stmt* stmt) -> bool
{
    return stmt != nullptr && isa<CompoundStmt>(stmt);
}

auto KuzuDump::isStatementConstexpr(const clang::Stmt* stmt) -> bool
{
    if (stmt == nullptr)
    {
        return false;
    }

    // Check if this is a constexpr if statement (C++17 feature)
    if (const auto* ifStmt = dyn_cast<IfStmt>(stmt))
    {
        return ifStmt->isConstexpr();
    }

    // For expressions, check if they're constant expressions
    if (const auto* expr = dyn_cast<Expr>(stmt))
    {
        return isExpressionConstexpr(expr);
    }

    return false;
}

auto KuzuDump::extractExpressionKind(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "unknown";
    }
    return expr->getStmtClassName();
}

auto KuzuDump::extractValueCategory(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "unknown";
    }

    switch (expr->getValueKind())
    {
    case VK_PRValue:
        return "prvalue";
    case VK_LValue:
        return "lvalue";
    case VK_XValue:
        return "xvalue";
    default:
        return "unknown";
    }
}

auto KuzuDump::extractLiteralValue(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "";
    }

    if (const auto* intLit = dyn_cast<IntegerLiteral>(expr))
    {
        return std::to_string(intLit->getValue().getSExtValue());
    }
    if (const auto* floatLit = dyn_cast<FloatingLiteral>(expr))
    {
        return std::to_string(floatLit->getValueAsApproximateDouble());
    }
    if (const auto* stringLit = dyn_cast<StringLiteral>(expr))
    {
        return stringLit->getString().str();
    }
    if (const auto* charLit = dyn_cast<CharacterLiteral>(expr))
    {
        return std::to_string(charLit->getValue());
    }
    if (const auto* boolLit = dyn_cast<CXXBoolLiteralExpr>(expr))
    {
        return boolLit->getValue() ? "true" : "false";
    }
    if (isa<CXXNullPtrLiteralExpr>(expr))
    {
        return "nullptr";
    }

    return "";
}

auto KuzuDump::extractOperatorKind(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "";
    }

    if (const auto* binOp = dyn_cast<BinaryOperator>(expr))
    {
        return binOp->getOpcodeStr().str();
    }
    if (const auto* unaryOp = dyn_cast<UnaryOperator>(expr))
    {
        return UnaryOperator::getOpcodeStr(unaryOp->getOpcode()).str();
    }
    if (const auto* cxxOp = dyn_cast<CXXOperatorCallExpr>(expr))
    {
        return getOperatorSpelling(cxxOp->getOperator());
    }

    return "";
}

auto KuzuDump::isExpressionConstexpr(const clang::Expr* expr) -> bool
{
    if (expr == nullptr)
    {
        return false;
    }

    // Check if the expression is a constant expression
    // Simplified approach - check if it's a literal or has constant value
    return isa<IntegerLiteral>(expr) || isa<FloatingLiteral>(expr) || isa<StringLiteral>(expr) ||
           isa<CharacterLiteral>(expr) || isa<CXXBoolLiteralExpr>(expr) || isa<CXXNullPtrLiteralExpr>(expr);
}

auto KuzuDump::extractEvaluationResult(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "";
    }

    // For constant expressions, try to get their evaluated value
    // Simplified approach - just check if it's a literal
    if (isa<IntegerLiteral>(expr) || isa<FloatingLiteral>(expr) || isa<StringLiteral>(expr) ||
        isa<CharacterLiteral>(expr) || isa<CXXBoolLiteralExpr>(expr) || isa<CXXNullPtrLiteralExpr>(expr))
    {
        return extractLiteralValue(expr);
    }

    return "";
}

auto KuzuDump::extractImplicitCastKind(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "";
    }

    if (const auto* castExpr = dyn_cast<ImplicitCastExpr>(expr))
    {
        return castExpr->getCastKindName();
    }

    return "";
}

// Hierarchy processing methods (Phase 2)
void KuzuDump::pushParent(int64_t parentNodeId)
{
    parentStack.push_back(parentNodeId);
    childIndex = 0;  // Reset child index for new parent
}

void KuzuDump::popParent()
{
    if (!parentStack.empty())
    {
        parentStack.pop_back();
        childIndex = 0;  // Reset child index when popping
    }
}

void KuzuDump::createHierarchyRelationship(int64_t childNodeId)
{
    if (parentStack.empty() || childNodeId == -1)
    {
        return;
    }

    int64_t parentNodeId = getCurrentParent();
    if (parentNodeId != -1)
    {
        createParentChildRelation(parentNodeId, childNodeId, childIndex++);
    }
}

auto KuzuDump::getCurrentParent() -> int64_t
{
    if (parentStack.empty())
    {
        return -1;
    }
    return parentStack.back();
}

// Scope processing methods (Phase 3)
void KuzuDump::pushScope(int64_t scopeNodeId)
{
    if (scopeNodeId != -1)
    {
        scopeStack.push_back(scopeNodeId);
    }
}

void KuzuDump::popScope()
{
    if (!scopeStack.empty())
    {
        scopeStack.pop_back();
    }
}

void KuzuDump::createScopeRelationships(int64_t nodeId)
{
    if (nodeId == -1 || scopeStack.empty())
    {
        return;
    }

    // Create IN_SCOPE relationships for all current scopes
    for (int64_t scopeId : scopeStack)
    {
        // Determine scope kind based on scope type
        // For now, we'll use "local" as default, but this could be enhanced
        createScopeRelation(nodeId, scopeId, "local");
    }
}

auto KuzuDump::getCurrentScope() -> int64_t
{
    if (scopeStack.empty())
    {
        return -1;
    }
    return scopeStack.back();
}

void KuzuDump::createTemplateRelation(int64_t specializationId, int64_t templateId, const std::string& kind)
{
    if (!connection || specializationId == -1 || templateId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (spec:ASTNode {node_id: " + std::to_string(specializationId) + "}), " +
                            "(tmpl:Declaration {node_id: " + std::to_string(templateId) + "}) " +
                            "CREATE (spec)-[:TEMPLATE_RELATION {relation_kind: '" + kind +
                            "', specialization_type: 'explicit'}]->(tmpl)";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating TEMPLATE_RELATION relationship: " << e.what() << "\n";
    }
}

// Core Visit method implementations
void KuzuDump::VisitDecl(const Decl* D)
{
    if (D == nullptr)
    {
        return;
    }

    // Create database node for this declaration
    int64_t nodeId = createASTNode(D);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitFunctionDecl(const FunctionDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this function declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this function (it's in its parent scope)
    createScopeRelationships(nodeId);

    // Check if this is a function template specialization
    if (const FunctionTemplateSpecializationInfo* specInfo = D->getTemplateSpecializationInfo())
    {
        if (const FunctionTemplateDecl* templateDecl = specInfo->getTemplate())
        {
            // Check if the template declaration has been processed
            auto it = nodeIdMap.find(templateDecl);
            if (it != nodeIdMap.end())
            {
                createTemplateRelation(nodeId, it->second, "specializes");
            }
            else
            {
                // Create the template node if it doesn't exist yet
                int64_t templateNodeId = createASTNode(templateDecl);
                createTemplateRelation(nodeId, templateNodeId, "specializes");
            }
        }
    }

    // Push this function as a new scope for its body and parameters
    pushScope(nodeId);

    // The ASTNodeTraverser will handle automatic traversal
    // Pop this function scope after traversal
    popScope();
}

void KuzuDump::VisitVarDecl(const VarDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this variable declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this variable (it's in its current scope)
    createScopeRelationships(nodeId);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitParmVarDecl(const ParmVarDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this parameter declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this parameter (it's in its parent function scope)
    createScopeRelationships(nodeId);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitNamespaceDecl(const NamespaceDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this namespace declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this namespace (it's in its parent scope)
    createScopeRelationships(nodeId);

    // Push this namespace as a new scope for its contents
    pushScope(nodeId);

    // The ASTNodeTraverser will handle automatic traversal
    // Pop this namespace scope after traversal
    popScope();
}

void KuzuDump::VisitClassTemplateSpecializationDecl(const ClassTemplateSpecializationDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this class template specialization
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this specialization (it's in its parent scope)
    createScopeRelationships(nodeId);

    // Create template relationship if we can find the template declaration
    if (const ClassTemplateDecl* templateDecl = D->getSpecializedTemplate())
    {
        // Check if the template declaration has been processed
        auto it = nodeIdMap.find(templateDecl);
        if (it != nodeIdMap.end())
        {
            createTemplateRelation(nodeId, it->second, "specializes");
        }
        else
        {
            // Create the template node if it doesn't exist yet
            int64_t templateNodeId = createASTNode(templateDecl);
            createTemplateRelation(nodeId, templateNodeId, "specializes");
        }
    }

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitStmt(const Stmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this statement
    int64_t nodeId = createASTNode(S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitCompoundStmt(const CompoundStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this compound statement
    int64_t nodeId = createASTNode(S);

    // Create enhanced Statement node with detailed information
    createStatementNode(nodeId, S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitDeclStmt(const DeclStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this declaration statement
    int64_t nodeId = createASTNode(S);

    // Create enhanced Statement node with detailed information
    createStatementNode(nodeId, S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Create scope relationships for this declaration statement
    createScopeRelationships(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // Process each declaration in this statement
    for (const auto* decl : S->decls())
    {
        if (const auto* varDecl = dyn_cast<VarDecl>(decl))
        {
            // Create the variable declaration node
            int64_t varNodeId = createASTNode(varDecl);

            // Create scope relationships for this local variable
            createScopeRelationships(varNodeId);
        }
    }

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitReturnStmt(const ReturnStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this return statement
    int64_t nodeId = createASTNode(S);

    // Create enhanced Statement node with detailed information
    createStatementNode(nodeId, S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitIfStmt(const IfStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this if statement
    int64_t nodeId = createASTNode(S);

    // Create enhanced Statement node with detailed information
    createStatementNode(nodeId, S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitWhileStmt(const WhileStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this while statement
    int64_t nodeId = createASTNode(S);

    // Create enhanced Statement node with detailed information
    createStatementNode(nodeId, S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitForStmt(const ForStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this for statement
    int64_t nodeId = createASTNode(S);

    // Create enhanced Statement node with detailed information
    createStatementNode(nodeId, S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitExpr(const Expr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this expression
    int64_t nodeId = createASTNode(E);

    // Create enhanced Expression node with detailed information
    createExpressionNode(nodeId, E);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitDeclRefExpr(const DeclRefExpr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this declaration reference expression
    int64_t nodeId = createASTNode(E);

    // Create reference relationship if we can find the referenced declaration
    if (const Decl* referencedDecl = E->getDecl())
    {
        // Check if the referenced declaration has been processed
        auto it = nodeIdMap.find(referencedDecl);
        if (it != nodeIdMap.end())
        {
            createReferenceRelation(nodeId, it->second, "uses");
        }
        else
        {
            // Create the referenced declaration node if it doesn't exist yet
            int64_t referencedNodeId = createASTNode(referencedDecl);
            if (referencedNodeId != -1)
            {
                createReferenceRelation(nodeId, referencedNodeId, "uses");
            }
        }
    }

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitIntegerLiteral(const IntegerLiteral* E)
{
    if (E == nullptr)
        return;

    // Create database node for this integer literal
    int64_t nodeId = createASTNode(E);

    // Create enhanced Expression node with detailed information
    createExpressionNode(nodeId, E);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitBinaryOperator(const BinaryOperator* E)
{
    if (E == nullptr)
        return;

    // Create database node for this binary operator
    int64_t nodeId = createASTNode(E);

    // Create enhanced Expression node with detailed information
    createExpressionNode(nodeId, E);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitUnaryOperator(const UnaryOperator* E)
{
    if (E == nullptr)
        return;

    // Create database node for this unary operator
    int64_t nodeId = createASTNode(E);

    // Create enhanced Expression node with detailed information
    createExpressionNode(nodeId, E);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitCallExpr(const CallExpr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this call expression
    int64_t nodeId = createASTNode(E);

    // Create reference relationship for function calls
    if (const FunctionDecl* calledFunc = E->getDirectCallee())
    {
        // Check if the called function has been processed
        auto it = nodeIdMap.find(calledFunc);
        if (it != nodeIdMap.end())
        {
            createReferenceRelation(nodeId, it->second, "calls");
        }
        else
        {
            // Create the called function node if it doesn't exist yet
            int64_t calledFuncNodeId = createASTNode(calledFunc);
            if (calledFuncNodeId != -1)
            {
                createReferenceRelation(nodeId, calledFuncNodeId, "calls");
            }
        }
    }

    // Handle indirect calls through function pointers or expressions
    if (const Expr* callee = E->getCallee())
    {
        if (const auto* declRef = dyn_cast<DeclRefExpr>(callee))
        {
            // This will be handled by VisitDeclRefExpr, so we don't duplicate
            static_cast<void>(declRef);  // Suppress unused variable warning
        }
        else if (const auto* memberExpr = dyn_cast<MemberExpr>(callee))
        {
            // Handle member function calls
            if (const ValueDecl* memberDecl = memberExpr->getMemberDecl())
            {
                auto it = nodeIdMap.find(memberDecl);
                if (it != nodeIdMap.end())
                {
                    createReferenceRelation(nodeId, it->second, "calls");
                }
                else
                {
                    int64_t memberNodeId = createASTNode(memberDecl);
                    if (memberNodeId != -1)
                    {
                        createReferenceRelation(nodeId, memberNodeId, "calls");
                    }
                }
            }
        }
    }

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitImplicitCastExpr(const ImplicitCastExpr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this implicit cast expression
    int64_t nodeId = createASTNode(E);

    // Create enhanced Expression node with detailed information
    createExpressionNode(nodeId, E);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}
