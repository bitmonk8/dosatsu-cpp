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

#include "kuzu.hpp"

#include "NoWarningScope_Leave.h"
// clang-format on

#include <memory>
#include <string>
#include <unordered_map>

namespace clang
{

class KuzuDump : public ASTNodeTraverser<KuzuDump, TextNodeDumper>
{
private:
    // Text dumping components (temporary during Phase 1-3) - Phase 4.3: Legacy support
    std::unique_ptr<llvm::raw_null_ostream> nullStream;  // For database-only mode
    TextNodeDumper NodeDumper;
    raw_ostream& OS;  // Keep as reference for compatibility with existing code
    const bool ShowColors;

    // Database-only mode flag (Phase 4.3)
    bool databaseOnlyMode = false;

    // Source location tracking (for precise location information)
    const SourceManager* sourceManager = nullptr;

    // Node tracking (always available)
    std::unordered_map<const void*, int64_t> nodeIdMap;  // Pointer -> node_id mapping
    int64_t nextNodeId = 1;

    // Hierarchy tracking (Phase 2)
    std::vector<int64_t> parentStack;  // Stack of parent node IDs during traversal
    int childIndex = 0;                // Index of current child within parent

    // Scope tracking (Phase 3)
    std::vector<int64_t> scopeStack;  // Stack of scope node IDs (namespaces, functions, etc.)

    // Database components
    std::unique_ptr<kuzu::main::Database> database;
    std::unique_ptr<kuzu::main::Connection> connection;

    // Database configuration
    std::string databasePath;
    bool databaseEnabled = false;

    // Performance optimization - batching (Phase 4)
    static constexpr size_t BATCH_SIZE = 100;
    std::vector<std::string> pendingQueries;
    bool transactionActive = false;
    size_t totalOperations = 0;

    // Private methods for database operations
    void initializeDatabase();
    void createSchema();
    void executeSchemaQuery(const std::string& query, const std::string& schemaName);

    // Performance optimization methods (Phase 4)
    void beginTransaction();
    void commitTransaction();
    void rollbackTransaction();
    void addToBatch(const std::string& query);
    void executeBatch();
    void flushOperations();

    // Node creation methods
    auto createASTNode(const clang::Decl* decl) -> int64_t;
    auto createASTNode(const clang::Stmt* stmt) -> int64_t;
    auto createASTNode(const clang::Type* type) -> int64_t;

    // Relationship creation methods
    void createParentChildRelation(int64_t parentId, int64_t childId, int index);
    void createTypeRelation(int64_t declId, int64_t typeId);
    void createReferenceRelation(int64_t fromId, int64_t toId, const std::string& kind);
    void createScopeRelation(int64_t nodeId, int64_t scopeId, const std::string& scopeKind);
    void createTemplateRelation(int64_t specializationId, int64_t templateId, const std::string& kind);
    void createSpecializesRelation(int64_t specializationId,
                                   int64_t templateId,
                                   const std::string& specializationKind,
                                   const std::string& templateArguments,
                                   const std::string& instantiationContext);
    void createInheritanceRelation(int64_t derivedId,
                                   int64_t baseId,
                                   const std::string& inheritanceType,
                                   bool isVirtual,
                                   const std::string& accessPath);
    void createOverrideRelation(int64_t overridingId,
                                int64_t overriddenId,
                                const std::string& overrideType,
                                bool isCovariantReturn);

    // Enhanced declaration processing methods (Phase 2)
    void createDeclarationNode(int64_t nodeId, const clang::NamedDecl* decl);
    void createUsingDeclarationNode(int64_t nodeId, const clang::UsingDecl* decl);
    void createUsingDirectiveNode(int64_t nodeId, const clang::UsingDirectiveDecl* decl);
    void createNamespaceAliasNode(int64_t nodeId, const clang::NamespaceAliasDecl* decl);
    auto extractQualifiedName(const clang::NamedDecl* decl) -> std::string;
    auto extractAccessSpecifier(const clang::Decl* decl) -> std::string;
    auto extractStorageClass(const clang::Decl* decl) -> std::string;
    auto extractNamespaceContext(const clang::Decl* decl) -> std::string;
    auto isDefinition(const clang::Decl* decl) -> bool;

    // Enhanced statement and expression processing methods
    void createStatementNode(int64_t nodeId, const clang::Stmt* stmt);
    void createExpressionNode(int64_t nodeId, const clang::Expr* expr);
    auto extractStatementKind(const clang::Stmt* stmt) -> std::string;
    auto extractControlFlowType(const clang::Stmt* stmt) -> std::string;
    auto extractConditionText(const clang::Stmt* stmt) -> std::string;
    auto hasStatementSideEffects(const clang::Stmt* stmt) -> bool;
    auto isCompoundStatement(const clang::Stmt* stmt) -> bool;
    auto isStatementConstexpr(const clang::Stmt* stmt) -> bool;
    auto extractExpressionKind(const clang::Expr* expr) -> std::string;
    auto extractValueCategory(const clang::Expr* expr) -> std::string;
    auto extractLiteralValue(const clang::Expr* expr) -> std::string;
    auto extractOperatorKind(const clang::Expr* expr) -> std::string;
    auto isExpressionConstexpr(const clang::Expr* expr) -> bool;
    auto extractEvaluationResult(const clang::Expr* expr) -> std::string;
    auto extractImplicitCastKind(const clang::Expr* expr) -> std::string;

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

    // Scope processing methods (Phase 3)
    void pushScope(int64_t scopeNodeId);
    void popScope();
    void createScopeRelationships(int64_t nodeId);
    auto getCurrentScope() -> int64_t;
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
        : nullStream(nullptr), NodeDumper(OS, Context, ShowColors), OS(OS), ShowColors(ShowColors),
          sourceManager(&Context.getSourceManager())
    {
    }

    KuzuDump(raw_ostream& OS, bool ShowColors)
        : nullStream(nullptr), NodeDumper(OS, ShowColors), OS(OS), ShowColors(ShowColors)
    {
    }

    // New database constructors (Phase 1)
    KuzuDump(std::string databasePath, const ASTContext& Context, bool ShowColors = false)
        : nullStream(std::make_unique<llvm::raw_null_ostream>()), NodeDumper(*nullStream, Context, ShowColors),
          OS(*nullStream), ShowColors(ShowColors), sourceManager(&Context.getSourceManager()),
          databasePath(std::move(databasePath)), databaseEnabled(true)
    {
        initializeDatabase();
    }

    // Database-only constructor (Phase 4.3) - No TextNodeDumper dependencies
    KuzuDump(std::string databasePath, const ASTContext& Context, bool ShowColors, bool pureDatabaseMode)
        : nullStream(std::make_unique<llvm::raw_null_ostream>()), NodeDumper(*nullStream, Context, ShowColors),
          OS(*nullStream), ShowColors(ShowColors), databaseOnlyMode(pureDatabaseMode),
          sourceManager(&Context.getSourceManager()), databasePath(std::move(databasePath)), databaseEnabled(true)
    {
        initializeDatabase();
    }

    // Hybrid constructor (temporary for development/testing)
    KuzuDump(std::string databasePath, raw_ostream& OS, const ASTContext& Context, bool ShowColors = false)
        : nullStream(nullptr), NodeDumper(OS, Context, ShowColors), OS(OS), ShowColors(ShowColors),
          sourceManager(&Context.getSourceManager()), databasePath(std::move(databasePath)), databaseEnabled(true)
    {
        initializeDatabase();
    }

    // Destructor to ensure proper cleanup (Phase 4)
    ~KuzuDump()
    {
        if (databaseEnabled && connection)
        {
            flushOperations();
        }
    }

    auto doGetNodeDelegate() -> TextNodeDumper& { return NodeDumper; }

    void dumpInvalidDeclContext(const DeclContext* DC);
    void dumpLookups(const DeclContext* DC, bool DumpDecls);

    template <typename SpecializationDecl>
    void dumpTemplateDeclSpecialization(const SpecializationDecl* D, bool DumpExplicitInst, bool DumpRefOnly);
    template <typename TemplateDecl>
    void dumpTemplateDecl(const TemplateDecl* D, bool DumpExplicitInst);

    // Enhanced template parameter processing
    void dumpTemplateParameters(const clang::TemplateParameterList* templateParams);
    void createTemplateParameterNode(int64_t nodeId, const clang::NamedDecl* param);
    auto extractTemplateArguments(const clang::TemplateArgumentList& args) -> std::string;

    // Preprocessor and Macro processing methods
    void createMacroDefinitionNode(int64_t nodeId,
                                   const std::string& macroName,
                                   bool isFunctionLike,
                                   const std::vector<std::string>& parameters,
                                   const std::string& replacementText,
                                   bool isBuiltin = false,
                                   bool isConditional = false);
    void createIncludeDirectiveNode(int64_t nodeId,
                                    const std::string& includePath,
                                    bool isSystemInclude,
                                    bool isAngled,
                                    const std::string& resolvedPath,
                                    int64_t includeDepth);
    void createConditionalDirectiveNode(int64_t nodeId,
                                        const std::string& directiveType,
                                        const std::string& conditionText,
                                        bool isTrueBranch,
                                        int64_t nestingLevel);
    void createPragmaDirectiveNode(int64_t nodeId,
                                   const std::string& pragmaName,
                                   const std::string& pragmaText,
                                   const std::string& pragmaKind);
    void createMacroExpansionRelation(int64_t fromId,
                                      int64_t macroId,
                                      const std::string& expansionContext,
                                      const std::string& expansionArguments);
    void createIncludesRelation(int64_t fromId, int64_t includeId, int64_t includeOrder);
    void createDefinesRelation(int64_t fromId, int64_t macroId, const std::string& definitionContext);

    // Core Visit methods for basic AST nodes
    void VisitDecl(const Decl* D);
    void VisitFunctionDecl(const FunctionDecl* D);
    void VisitVarDecl(const VarDecl* D);
    void VisitParmVarDecl(const ParmVarDecl* D);
    void VisitNamespaceDecl(const NamespaceDecl* D);
    void VisitUsingDecl(const UsingDecl* D);
    void VisitUsingDirectiveDecl(const UsingDirectiveDecl* D);
    void VisitNamespaceAliasDecl(const NamespaceAliasDecl* D);
    void VisitCXXRecordDecl(const CXXRecordDecl* D);
    void VisitClassTemplateDecl(const ClassTemplateDecl* D);
    void VisitFunctionTemplateDecl(const FunctionTemplateDecl* D);
    void VisitVarTemplateDecl(const VarTemplateDecl* D);
    void VisitClassTemplateSpecializationDecl(const ClassTemplateSpecializationDecl* D);
    void VisitClassTemplatePartialSpecializationDecl(const ClassTemplateSpecializationDecl* D);

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

    // Template-specific visit methods (already declared above)
};

}  // namespace clang