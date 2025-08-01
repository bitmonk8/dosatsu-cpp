#pragma once

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTNodeTraverser.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceLocation.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

namespace kuzu
{

/// Forward declarations for Kuzu database interface
class Database;
class Connection;
class Table;

}  // namespace kuzu

/// A minimal node dumper interface for compatibility with ASTNodeTraverser
class KuzuNodeDumper
{
public:
    KuzuNodeDumper() = default;

    // Required methods for ASTNodeTraverser compatibility
    template <typename... Args>
    void AddChild(Args&&... args)
    {
        // Placeholder - actual implementation would handle child nodes
    }

    template <typename T>
    void Visit(const T* node)
    {
        // Placeholder - actual implementation would visit nodes
    }

    template <typename T, typename U>
    void Visit(const T* node, const U* context)
    {
        // Placeholder - overload for two-argument visits
    }

    // Overload for QualType (not a pointer)
    void Visit(clang::QualType qt)
    {
        // Placeholder - overload for QualType
    }

    // Generic visit for any type
    template <typename T>
    void Visit(const T& node)
    {
        // Placeholder - overload for reference types
    }

    // 4-argument visit for template arguments
    template <typename T, typename U, typename V, typename W>
    void Visit(const T& arg1, U arg2, const V* arg3, const W* arg4)
    {
        // Placeholder - overload for 4-argument visits
    }
};

/// KuzuDump traverses an AST and dumps it to a Kuzu graph database.
/// This class is modeled after clang's ASTDumper but instead of dumping to text,
/// it creates nodes and relationships in a Kuzu database for later querying.
class KuzuDump : public clang::ASTNodeTraverser<KuzuDump, KuzuNodeDumper>
{
public:
    /// Constructor for dumping to a Kuzu database
    /// @param Context The AST context
    /// @param DatabasePath Path to the Kuzu database
    KuzuDump(const clang::ASTContext& Context, std::string DatabasePath);

    /// Constructor for testing without database
    /// @param Context The AST context
    explicit KuzuDump(const clang::ASTContext& Context);

    ~KuzuDump();

    // Visit methods for different AST node types
    void VisitDecl(const clang::Decl* D);
    void VisitFunctionDecl(const clang::FunctionDecl* D);
    void VisitVarDecl(const clang::VarDecl* D);
    void VisitRecordDecl(const clang::RecordDecl* D);
    void VisitCXXRecordDecl(const clang::CXXRecordDecl* D);
    void VisitFieldDecl(const clang::FieldDecl* D);
    void VisitParmVarDecl(const clang::ParmVarDecl* D);
    void VisitTypedefDecl(const clang::TypedefDecl* D);
    void VisitNamespaceDecl(const clang::NamespaceDecl* D);
    void VisitUsingDecl(const clang::UsingDecl* D);
    void VisitCXXMethodDecl(const clang::CXXMethodDecl* D);
    void VisitCXXConstructorDecl(const clang::CXXConstructorDecl* D);
    void VisitCXXDestructorDecl(const clang::CXXDestructorDecl* D);

    void VisitStmt(const clang::Stmt* S);
    void VisitExpr(const clang::Expr* E);
    void VisitCallExpr(const clang::CallExpr* E);
    void VisitDeclRefExpr(const clang::DeclRefExpr* E);
    void VisitMemberExpr(const clang::MemberExpr* E);
    void VisitCompoundStmt(const clang::CompoundStmt* S);
    void VisitIfStmt(const clang::IfStmt* S);
    void VisitForStmt(const clang::ForStmt* S);
    void VisitWhileStmt(const clang::WhileStmt* S);
    void VisitReturnStmt(const clang::ReturnStmt* S);

    void VisitType(const clang::Type* T);
    void VisitQualType(clang::QualType T);

    /// Dump the entire translation unit
    void dumpTranslationUnit(const clang::TranslationUnitDecl* TU);

    // Required by ASTNodeTraverser
    KuzuNodeDumper& doGetNodeDelegate() { return NodeDumper; }

private:
    const clang::ASTContext& Context;
    std::string DatabasePath;
    KuzuNodeDumper NodeDumper;

    // Kuzu database components (will be implemented later)
    std::unique_ptr<kuzu::Database> Database;
    std::unique_ptr<kuzu::Connection> Connection;

    // Helper methods for database operations
    void initializeDatabase();
    void createDatabaseSchema();
    void insertASTNode(const std::string& nodeType,
                       const std::string& nodeId,
                       const std::string& name,
                       const std::string& location);
    void insertRelationship(const std::string& fromId, const std::string& toId, const std::string& relationshipType);

    // Helper methods for AST processing
    std::string getNodeId(const void* ptr);
    std::string getSourceLocation(clang::SourceLocation loc);
    std::string getQualifiedName(const clang::NamedDecl* decl);
    std::string getTypeString(clang::QualType type);

    // Relationship tracking
    void createParentChildRelationship(const void* parent, const void* child);
    void createTypeRelationship(const void* decl, const clang::Type* type);
    void createUsageRelationship(const void* user, const void* used);
};