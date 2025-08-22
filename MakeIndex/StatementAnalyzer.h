//===--- StatementAnalyzer.h - Statement and expression analysis --------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#pragma once

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ASTContext.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <cstdint>
#include <string>

namespace clang
{

class KuzuDatabase;
class ASTNodeProcessor;

/// Handles statement and expression analysis for AST storage
class StatementAnalyzer
{
public:
    /// Constructor
    /// \param database Database instance for storage
    /// \param nodeProcessor Node processor for creating basic nodes
    /// \param astContext AST context for analysis
    StatementAnalyzer(KuzuDatabase& database, ASTNodeProcessor& nodeProcessor, const ASTContext& astContext);

    /// Create statement node
    /// \param nodeId Node ID for the statement
    /// \param stmt The statement to process
    void createStatementNode(int64_t nodeId, const clang::Stmt* stmt);

    /// Create expression node
    /// \param nodeId Node ID for the expression
    /// \param expr The expression to process
    void createExpressionNode(int64_t nodeId, const clang::Expr* expr);

    /// Extract statement kind
    /// \param stmt The statement
    /// \return String representation of statement kind
    auto extractStatementKind(const clang::Stmt* stmt) -> std::string;

    /// Extract control flow type
    /// \param stmt The statement
    /// \return String representation of control flow type
    auto extractControlFlowType(const clang::Stmt* stmt) -> std::string;

    /// Extract condition text from control flow statements
    /// \param stmt The statement
    /// \return String representation of condition
    auto extractConditionText(const clang::Stmt* stmt) -> std::string;

    /// Check if statement has side effects
    /// \param stmt The statement
    /// \return True if statement has side effects
    auto hasStatementSideEffects(const clang::Stmt* stmt) -> bool;

    /// Check if statement is compound
    /// \param stmt The statement
    /// \return True if statement is compound
    auto isCompoundStatement(const clang::Stmt* stmt) -> bool;

    /// Check if statement is constexpr
    /// \param stmt The statement
    /// \return True if statement is constexpr
    auto isStatementConstexpr(const clang::Stmt* stmt) -> bool;

    /// Extract expression kind
    /// \param expr The expression
    /// \return String representation of expression kind
    auto extractExpressionKind(const clang::Expr* expr) -> std::string;

    /// Extract value category
    /// \param expr The expression
    /// \return String representation of value category
    auto extractValueCategory(const clang::Expr* expr) -> std::string;

    /// Extract literal value
    /// \param expr The expression
    /// \return String representation of literal value
    auto extractLiteralValue(const clang::Expr* expr) -> std::string;

    /// Extract operator kind
    /// \param expr The expression
    /// \return String representation of operator kind
    auto extractOperatorKind(const clang::Expr* expr) -> std::string;

    /// Check if expression is constexpr
    /// \param expr The expression
    /// \return True if expression is constexpr
    auto isExpressionConstexpr(const clang::Expr* expr) -> bool;

    /// Extract evaluation result
    /// \param expr The expression
    /// \return String representation of evaluation result
    auto extractEvaluationResult(const clang::Expr* expr) -> std::string;

    /// Extract implicit cast kind
    /// \param expr The expression
    /// \return String representation of implicit cast kind
    auto extractImplicitCastKind(const clang::Expr* expr) -> std::string;

private:
    KuzuDatabase& database;
    ASTNodeProcessor& nodeProcessor;
    const ASTContext* astContext;
};

}  // namespace clang
