//===--- KuzuDump.h - Dumping implementation for ASTs --------------------===//
//
// Based on LLVM Project's ASTDumper, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/ASTNodeTraverser.h"
#include "clang/AST/TextNodeDumper.h"
#include "clang/Basic/SourceManager.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <memory>
#include <string>
#include <unordered_map>

#include "kuzu.hpp"

namespace clang
{

class KuzuDump : public ASTNodeTraverser<KuzuDump, TextNodeDumper>
{
private:
    // Text dumping components (temporary during Phase 1-3)
    std::unique_ptr<llvm::raw_null_ostream> nullStream;  // For database-only mode
    TextNodeDumper NodeDumper;
    raw_ostream& OS;  // Keep as reference for compatibility with existing code
    const bool ShowColors;

    // Node tracking (always available)
    std::unordered_map<const void*, int64_t> nodeIdMap;  // Pointer -> node_id mapping
    int64_t nextNodeId = 1;

    // Hierarchy tracking (Phase 2)
    std::vector<int64_t> parentStack;  // Stack of parent node IDs during traversal
    int childIndex = 0;                // Index of current child within parent

    // Database components
    std::unique_ptr<kuzu::main::Database> database;
    std::unique_ptr<kuzu::main::Connection> connection;

    // Database configuration
    std::string databasePath;
    bool databaseEnabled = false;

    // Private methods for database operations
    void initializeDatabase();
    void createSchema();
    void executeSchemaQuery(const std::string& query, const std::string& schemaName);

    // Node creation methods
    auto createASTNode(const clang::Decl* decl) -> int64_t;
    auto createASTNode(const clang::Stmt* stmt) -> int64_t;
    auto createASTNode(const clang::Type* type) -> int64_t;

    // Relationship creation methods
    void createParentChildRelation(int64_t parentId, int64_t childId, int index);
    void createTypeRelation(int64_t declId, int64_t typeId);
    void createReferenceRelation(int64_t fromId, int64_t toId, const std::string& kind);

    // Enhanced declaration processing methods (Phase 2)
    void createDeclarationNode(int64_t nodeId, const clang::NamedDecl* decl);
    auto extractQualifiedName(const clang::NamedDecl* decl) -> std::string;
    auto extractAccessSpecifier(const clang::Decl* decl) -> std::string;
    auto extractStorageClass(const clang::Decl* decl) -> std::string;
    auto extractNamespaceContext(const clang::Decl* decl) -> std::string;
    auto isDefinition(const clang::Decl* decl) -> bool;

    // Type processing methods (Phase 2)
    auto createTypeNodeAndRelation(int64_t declNodeId, clang::QualType qualType) -> int64_t;
    auto createTypeNode(clang::QualType qualType) -> int64_t;
    auto extractTypeName(clang::QualType qualType) -> std::string;
    auto extractTypeCategory(clang::QualType qualType) -> std::string;
    auto extractTypeQualifiers(clang::QualType qualType) -> std::string;

    // Hierarchy processing methods (Phase 2)
    void pushParent(int64_t parentNodeId);
    void popParent();
    void createHierarchyRelationship(int64_t childNodeId);
    auto getCurrentParent() -> int64_t;
    auto isBuiltInType(clang::QualType qualType) -> bool;
    auto extractTypeSourceLocation(clang::QualType qualType) -> std::string;

    // Data extraction utilities (shared between text and database output)
    auto extractSourceLocation(const clang::SourceLocation& loc) -> std::string;
    auto extractSourceLocationDetailed(const clang::SourceLocation& loc) -> std::tuple<std::string, int64_t, int64_t>;
    auto extractNodeType(const clang::Decl* decl) -> std::string;
    auto extractNodeType(const clang::Stmt* stmt) -> std::string;
    auto extractNodeType(const clang::Type* type) -> std::string;
    auto isImplicitNode(const clang::Decl* decl) -> bool;

public:
    // Legacy constructors (text output only)
    KuzuDump(raw_ostream& OS, const ASTContext& Context, bool ShowColors)
        : nullStream(nullptr), NodeDumper(OS, Context, ShowColors), OS(OS), ShowColors(ShowColors)
    {
    }

    KuzuDump(raw_ostream& OS, bool ShowColors)
        : nullStream(nullptr), NodeDumper(OS, ShowColors), OS(OS), ShowColors(ShowColors)
    {
    }

    // New database constructors (Phase 1)
    KuzuDump(std::string databasePath, const ASTContext& Context, bool ShowColors = false)
        : nullStream(std::make_unique<llvm::raw_null_ostream>()), NodeDumper(*nullStream, Context, ShowColors),
          OS(*nullStream), ShowColors(ShowColors), databasePath(std::move(databasePath)), databaseEnabled(true)
    {
        initializeDatabase();
    }

    // Hybrid constructor (temporary for development/testing)
    KuzuDump(std::string databasePath, raw_ostream& OS, const ASTContext& Context, bool ShowColors = false)
        : nullStream(nullptr), NodeDumper(OS, Context, ShowColors), OS(OS), ShowColors(ShowColors),
          databasePath(std::move(databasePath)), databaseEnabled(true)
    {
        initializeDatabase();
    }

    auto doGetNodeDelegate() -> TextNodeDumper& { return NodeDumper; }

    void dumpInvalidDeclContext(const DeclContext* DC);
    void dumpLookups(const DeclContext* DC, bool DumpDecls);

    template <typename SpecializationDecl>
    void dumpTemplateDeclSpecialization(const SpecializationDecl* D, bool DumpExplicitInst, bool DumpRefOnly);
    template <typename TemplateDecl>
    void dumpTemplateDecl(const TemplateDecl* D, bool DumpExplicitInst);

    // Core Visit methods for basic AST nodes
    void VisitDecl(const Decl* D);
    void VisitFunctionDecl(const FunctionDecl* D);
    void VisitVarDecl(const VarDecl* D);
    void VisitParmVarDecl(const ParmVarDecl* D);

    void VisitStmt(const Stmt* S);
    void VisitCompoundStmt(const CompoundStmt* S);
    void VisitDeclStmt(const DeclStmt* S);
    void VisitReturnStmt(const ReturnStmt* S);
    void VisitIfStmt(const IfStmt* S);
    void VisitWhileStmt(const WhileStmt* S);
    void VisitForStmt(const ForStmt* S);

    void VisitExpr(const Expr* E);
    void VisitDeclRefExpr(const DeclRefExpr* E);
    void VisitIntegerLiteral(const IntegerLiteral* E);
    void VisitBinaryOperator(const BinaryOperator* E);
    void VisitUnaryOperator(const UnaryOperator* E);
    void VisitCallExpr(const CallExpr* E);
    void VisitImplicitCastExpr(const ImplicitCastExpr* E);

    // Template-specific visit methods
    void VisitFunctionTemplateDecl(const FunctionTemplateDecl* D);
    void VisitClassTemplateDecl(const ClassTemplateDecl* D);
    void VisitVarTemplateDecl(const VarTemplateDecl* D);
};

}  // namespace clang