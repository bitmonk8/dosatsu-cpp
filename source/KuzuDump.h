//===--- KuzuDump.h - AST dumping implementation -------------------------===//
//
// Part of the Dosatsu project
// This demonstrates the refactored architecture using specialized analyzers
//
//===----------------------------------------------------------------------===//

#pragma once

// Include the new modular components
#include "ASTNodeProcessor.h"
#include "AdvancedAnalyzer.h"
#include "CommentProcessor.h"
#include "DeclarationAnalyzer.h"
#include "GlobalDatabaseManager.h"
#include "KuzuDatabase.h"
#include "ScopeManager.h"
#include "StatementAnalyzer.h"
#include "TemplateAnalyzer.h"
#include "TypeAnalyzer.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/ASTNodeTraverser.h"
#include "clang/AST/TextNodeDumper.h"
#include "clang/Basic/SourceManager.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <memory>
#include <string>

namespace clang
{

/// KuzuDump class that delegates to specialized analyzers
/// This demonstrates the improved architecture with separation of concerns
class KuzuDump : public ASTNodeTraverser<KuzuDump, TextNodeDumper>
{
private:
    // Text output support (legacy compatibility)
    std::unique_ptr<llvm::raw_null_ostream> nullStream;
    TextNodeDumper NodeDumper;
    raw_ostream& OS;
    bool databaseOnlyMode = false;

    // Modular components - each handles a specific responsibility
    KuzuDatabase* database;  // No longer owned by this instance
    std::unique_ptr<ASTNodeProcessor> nodeProcessor;
    std::unique_ptr<ScopeManager> scopeManager;
    std::unique_ptr<TypeAnalyzer> typeAnalyzer;
    std::unique_ptr<DeclarationAnalyzer> declarationAnalyzer;
    std::unique_ptr<StatementAnalyzer> statementAnalyzer;
    std::unique_ptr<TemplateAnalyzer> templateAnalyzer;
    std::unique_ptr<CommentProcessor> commentProcessor;
    std::unique_ptr<AdvancedAnalyzer> advancedAnalyzer;

public:
    // Legacy constructors (text output only)
    KuzuDump(raw_ostream& OS, ASTContext& Context, bool ShowColors);

    // Database constructors
    KuzuDump(std::string databasePath, ASTContext& Context, bool ShowColors = false);

    // Database-only constructor (no text output dependencies)
    KuzuDump(std::string databasePath, ASTContext& Context, bool ShowColors, bool pureDatabaseMode);

    // Destructor
    ~KuzuDump();

    // Required by ASTNodeTraverser
    auto doGetNodeDelegate() -> TextNodeDumper& { return NodeDumper; }

    // Core Visit methods - simplified implementation using analyzers
    void VisitDecl(const Decl* D);
    bool TraverseDecl(Decl* D);  // Override base traversal to dispatch correctly
    bool TraverseStmt(Stmt* S);  // Override base traversal for statements
    void VisitFunctionDecl(const FunctionDecl* D);
    void VisitVarDecl(const VarDecl* D);
    void VisitNamespaceDecl(const NamespaceDecl* D);
    void VisitUsingDecl(const UsingDecl* D);
    void VisitUsingDirectiveDecl(const UsingDirectiveDecl* D);
    void VisitNamespaceAliasDecl(const NamespaceAliasDecl* D);
    void VisitCXXRecordDecl(const CXXRecordDecl* D);
    void VisitClassTemplateDecl(const ClassTemplateDecl* D);
    void VisitFunctionTemplateDecl(const FunctionTemplateDecl* D);
    void VisitClassTemplateSpecializationDecl(const ClassTemplateSpecializationDecl* D);
    void VisitStaticAssertDecl(const StaticAssertDecl* D);
    void VisitTranslationUnitDecl(const TranslationUnitDecl* D);

    void VisitStmt(const Stmt* S);
    void VisitReturnStmt(const ReturnStmt* S);  // Specific return statement handler
    void VisitExpr(const Expr* E);

    // Legacy compatibility methods (delegating to analyzers)
    void dumpInvalidDeclContext(const DeclContext* DC);
    void dumpLookups(const DeclContext* DC, bool DumpDecls);

private:
    /// Initialize all analyzer components
    void initializeAnalyzers(ASTContext& Context);

    /// Process a declaration using the appropriate analyzers
    void processDeclaration(const Decl* D);

    /// Process a statement using the appropriate analyzers
    void processStatement(const Stmt* S);

    /// Get database instance (for legacy compatibility)
    [[nodiscard]] auto getDatabase() const -> KuzuDatabase* { return database; }
};

}  // namespace clang
