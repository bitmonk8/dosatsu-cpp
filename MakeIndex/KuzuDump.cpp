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

// clang-format off
#include "NoWarningScope_Enter.h"
#include "KuzuDump.h"
#include "clang/AST/ASTConcept.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclLookups.h"
#include "clang/AST/JSONNodeDumper.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"

#include "NoWarningScope_Leave.h"
// clang-format on

#include <filesystem>
#include <sstream>
#include <stdexcept>

#include "kuzu.hpp"

using namespace clang;
using namespace clang::comments;

void KuzuDump::dumpInvalidDeclContext(const DeclContext* DC)
{
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
    // FIXME: We don't add a declaration of a function template specialization
    // to its context when it's explicitly instantiated, so dump explicit
    // instantiations when we dump the template itself.
    dumpTemplateDecl(D, true);
}

void KuzuDump::VisitClassTemplateDecl(const ClassTemplateDecl* D)
{
    dumpTemplateDecl(D, false);
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

        // Create Statement table
        executeSchemaQuery("CREATE NODE TABLE Statement("
                           "node_id INT64 PRIMARY KEY, "
                           "statement_kind STRING, "
                           "has_side_effects BOOLEAN)",
                           "Statement");

        // Create Expression table
        executeSchemaQuery("CREATE NODE TABLE Expression("
                           "node_id INT64 PRIMARY KEY, "
                           "expression_kind STRING, "
                           "value_category STRING, "
                           "literal_value STRING, "
                           "operator_kind STRING)",
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
                           "FROM Declaration TO Declaration, "
                           "relation_type STRING)",
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

        std::string sourceFile = extractSourceLocation(decl->getLocation());

        bool isImplicit = isImplicitNode(decl);

        // Create base AST node query using string concatenation
        std::string query = "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
                            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
                            "', is_implicit: " + (isImplicit ? "true" : "false") +
                            ", start_line: -1, start_column: -1, end_line: -1, end_column: -1, raw_text: ''})";

        auto result = connection->query(query);

        if (!result->isSuccess())
        {
            llvm::errs() << "Failed to create AST node: " << result->getErrorMessage() << "\n";
            return -1;
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

        std::string sourceFile = extractSourceLocation(stmt->getBeginLoc());

        // Create base AST node query using string concatenation
        std::string query =
            "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
            "', is_implicit: false, start_line: -1, start_column: -1, end_line: -1, end_column: -1, raw_text: ''})";

        auto result = connection->query(query);
        if (!result->isSuccess())
        {
            llvm::errs() << "Failed to create AST node for statement: " << result->getErrorMessage() << "\n";
            return -1;
        }

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

        // Create base AST node query using string concatenation
        std::string query = "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
                            "', memory_address: '" + memoryAddr +
                            "', source_file: '', is_implicit: false, start_line: -1, start_column: -1, end_line: -1, "
                            "end_column: -1, raw_text: ''})";

        auto result = connection->query(query);
        if (!result->isSuccess())
        {
            llvm::errs() << "Failed to create AST node for type: " << result->getErrorMessage() << "\n";
            return -1;
        }

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node for type: " << e.what() << "\n";
        return -1;
    }
}

// Placeholder relationship creation methods (Phase 1)
void KuzuDump::createParentChildRelation(int64_t /*parentId*/, int64_t /*childId*/, int /*index*/)
{
    // TODO: Implement in Phase 2
}

void KuzuDump::createTypeRelation(int64_t /*declId*/, int64_t /*typeId*/)
{
    // TODO: Implement in Phase 2
}

void KuzuDump::createReferenceRelation(int64_t /*fromId*/, int64_t /*toId*/, const std::string& /*kind*/)
{
    // TODO: Implement in Phase 3
}

// Data extraction utility methods
auto KuzuDump::extractSourceLocation(const clang::SourceLocation& loc) -> std::string
{
    if (loc.isInvalid())
    {
        return "<invalid>";
    }

    // For now, return a simple representation
    // TODO: Enhance with full file path and line/column information in Phase 2
    return "<source_location>";
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

// Core Visit method implementations
void KuzuDump::VisitDecl(const Decl* D)
{
    if (D == nullptr)
    {
        return;
    }

    // Create database node for this declaration
    [[maybe_unused]] int64_t nodeId = createASTNode(D);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitFunctionDecl(const FunctionDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this function declaration
    [[maybe_unused]] int64_t nodeId = createASTNode(D);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitVarDecl(const VarDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this variable declaration
    [[maybe_unused]] int64_t nodeId = createASTNode(D);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitParmVarDecl(const ParmVarDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this parameter declaration
    [[maybe_unused]] int64_t nodeId = createASTNode(D);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitStmt(const Stmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitCompoundStmt(const CompoundStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this compound statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitDeclStmt(const DeclStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this declaration statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitReturnStmt(const ReturnStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this return statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitIfStmt(const IfStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this if statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitWhileStmt(const WhileStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this while statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitForStmt(const ForStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this for statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitExpr(const Expr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this expression
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitDeclRefExpr(const DeclRefExpr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this declaration reference expression
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitIntegerLiteral(const IntegerLiteral* E)
{
    if (E == nullptr)
        return;

    // Create database node for this integer literal
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitBinaryOperator(const BinaryOperator* E)
{
    if (E == nullptr)
        return;

    // Create database node for this binary operator
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitUnaryOperator(const UnaryOperator* E)
{
    if (E == nullptr)
        return;

    // Create database node for this unary operator
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitCallExpr(const CallExpr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this call expression
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitImplicitCastExpr(const ImplicitCastExpr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this implicit cast expression
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}
