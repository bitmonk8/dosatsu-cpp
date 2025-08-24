//===--- StatementAnalyzer.cpp - Statement and expression analysis ------===//
//
// Part of the Dosatsu project
//
//===----------------------------------------------------------------------===//

#include "StatementAnalyzer.h"

#include "ASTNodeProcessor.h"
#include "GlobalDatabaseManager.h"
#include "KuzuDatabase.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/StmtCXX.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>

using namespace clang;

StatementAnalyzer::StatementAnalyzer(KuzuDatabase& database, ASTNodeProcessor& nodeProcessor, ASTContext& astContext)
    : database(database), nodeProcessor(nodeProcessor), astContext(&astContext)
{
}

void StatementAnalyzer::createStatementNode(int64_t nodeId, const clang::Stmt* stmt)
{
    if (!database.isInitialized() || (stmt == nullptr))
        return;

    // Check if Statement node already exists for this nodeId
    auto& dbManager = GlobalDatabaseManager::getInstance();
    if (dbManager.hasStatementNode(nodeId))
        return;

    try
    {
        std::string statementKind = extractStatementKind(stmt);
        bool hasSideEffects = hasStatementSideEffects(stmt);
        bool isCompound = isCompoundStatement(stmt);
        std::string controlFlowType = extractControlFlowType(stmt);
        std::string conditionText = extractConditionText(stmt);
        bool isConstexpr = isStatementConstexpr(stmt);

        // Escape single quotes for database storage
        std::ranges::replace(conditionText, '\'', '_');

        std::string query = "CREATE (s:Statement {node_id: " + std::to_string(nodeId) + ", statement_kind: '" +
                            statementKind + "', has_side_effects: " + (hasSideEffects ? "true" : "false") +
                            ", is_compound: " + (isCompound ? "true" : "false") + ", control_flow_type: '" +
                            controlFlowType + "', condition_text: '" + conditionText +
                            "', is_constexpr: " + (isConstexpr ? "true" : "false") + "})";

        database.addToBatch(query);

        // Register that this Statement node has been created
        dbManager.registerStatementNode(nodeId);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Statement node: " << e.what() << "\n";
    }
}

void StatementAnalyzer::createExpressionNode(int64_t nodeId, const clang::Expr* expr)
{
    if (!database.isInitialized() || (expr == nullptr))
        return;

    // Check if Expression node already exists for this nodeId
    auto& dbManager = GlobalDatabaseManager::getInstance();
    if (dbManager.hasExpressionNode(nodeId))
        return;

    try
    {
        std::string expressionKind = extractExpressionKind(expr);
        std::string valueCategory = extractValueCategory(expr);
        std::string literalValue = extractLiteralValue(expr);
        std::string operatorKind = extractOperatorKind(expr);
        bool isConstexpr = isExpressionConstexpr(expr);
        std::string evaluationResult = extractEvaluationResult(expr);
        std::string implicitCastKind = extractImplicitCastKind(expr);

        // Escape single quotes for database storage
        std::ranges::replace(literalValue, '\'', '_');
        std::ranges::replace(evaluationResult, '\'', '_');

        std::string query = "CREATE (e:Expression {node_id: " + std::to_string(nodeId) + ", expression_kind: '" +
                            expressionKind + "', value_category: '" + valueCategory + "', literal_value: '" +
                            literalValue + "', operator_kind: '" + operatorKind +
                            "', is_constexpr: " + (isConstexpr ? "true" : "false") + ", evaluation_result: '" +
                            evaluationResult + "', implicit_cast_kind: '" + implicitCastKind + "'})";

        database.addToBatch(query);

        // Register that this Expression node has been created
        dbManager.registerExpressionNode(nodeId);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Expression node: " << e.what() << "\n";
    }
}

auto StatementAnalyzer::extractStatementKind(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
        return "unknown";
    return stmt->getStmtClassName();
}

auto StatementAnalyzer::extractControlFlowType(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
        return "none";

    if (isa<IfStmt>(stmt))
        return "conditional";
    if (isa<WhileStmt>(stmt) || isa<ForStmt>(stmt) || isa<DoStmt>(stmt))
        return "loop";
    if (isa<SwitchStmt>(stmt))
        return "switch";
    if (isa<CaseStmt>(stmt) || isa<DefaultStmt>(stmt))
        return "case";
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
    if (isa<CXXTryStmt>(stmt))
        return "try";
    if (isa<CXXCatchStmt>(stmt))
        return "catch";
    if (isa<CXXThrowExpr>(stmt))
        return "throw";
    return "none";
}

auto StatementAnalyzer::extractConditionText(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
        return "";

    if (const auto* ifStmt = dyn_cast<IfStmt>(stmt))
    {
        if (const Expr* cond = ifStmt->getCond())
            return nodeProcessor.extractSourceLocation(cond->getBeginLoc());
    }
    else if (const auto* whileStmt = dyn_cast<WhileStmt>(stmt))
    {
        if (const Expr* cond = whileStmt->getCond())
            return nodeProcessor.extractSourceLocation(cond->getBeginLoc());
    }
    else if (const auto* forStmt = dyn_cast<ForStmt>(stmt))
    {
        if (const Expr* cond = forStmt->getCond())
            return nodeProcessor.extractSourceLocation(cond->getBeginLoc());
    }

    return "";
}

auto StatementAnalyzer::hasStatementSideEffects(const clang::Stmt* stmt) -> bool
{
    if (stmt == nullptr)
        return false;

    // Expressions with side effects
    if (const auto* expr = dyn_cast<Expr>(stmt))
    {
        if (isa<CallExpr>(expr) || isa<CXXOperatorCallExpr>(expr))
            return true;
        if (isa<BinaryOperator>(expr))
        {
            const auto* binOp = cast<BinaryOperator>(expr);
            return binOp->isAssignmentOp() || binOp->isCompoundAssignmentOp();
        }
        if (isa<UnaryOperator>(expr))
        {
            const auto* unOp = cast<UnaryOperator>(expr);
            return unOp->isIncrementDecrementOp();
        }
    }

    // Statements with side effects
    return isa<DeclStmt>(stmt) || isa<ReturnStmt>(stmt) || isa<BreakStmt>(stmt) || isa<ContinueStmt>(stmt) ||
           isa<CXXThrowExpr>(stmt);
}

auto StatementAnalyzer::isCompoundStatement(const clang::Stmt* stmt) -> bool
{
    return stmt != nullptr && isa<CompoundStmt>(stmt);
}

auto StatementAnalyzer::isStatementConstexpr(const clang::Stmt* stmt) -> bool
{
    if (stmt == nullptr)
        return false;

    // Check if this is a constexpr expression
    if (const auto* expr = dyn_cast<Expr>(stmt))
    {
        if (expr->isValueDependent() || expr->isTypeDependent())
            return false;

        // Try to evaluate as constant expression
        Expr::EvalResult result;
        return expr->EvaluateAsConstantExpr(result, *astContext);
    }

    return false;
}

auto StatementAnalyzer::extractExpressionKind(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
        return "unknown";
    return expr->getStmtClassName();
}

auto StatementAnalyzer::extractValueCategory(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
        return "unknown";

    switch (expr->getValueKind())
    {
    case clang::VK_PRValue:
        return "prvalue";
    case clang::VK_LValue:
        return "lvalue";
    case clang::VK_XValue:
        return "xvalue";
    default:
        return "unknown";
    }
}

auto StatementAnalyzer::extractLiteralValue(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
        return "";

    if (const auto* intLiteral = dyn_cast<IntegerLiteral>(expr))
        return std::to_string(intLiteral->getValue().getLimitedValue());
    if (const auto* floatLiteral = dyn_cast<FloatingLiteral>(expr))
    {
        llvm::SmallString<16> str;
        floatLiteral->getValue().toString(str);
        return str.str().str();
    }
    if (const auto* stringLiteral = dyn_cast<StringLiteral>(expr))
        return stringLiteral->getString().str();
    if (const auto* charLiteral = dyn_cast<CharacterLiteral>(expr))
        return std::to_string(charLiteral->getValue());
    if (isa<CXXBoolLiteralExpr>(expr))
    {
        const auto* boolLiteral = cast<CXXBoolLiteralExpr>(expr);
        return boolLiteral->getValue() ? "true" : "false";
    }
    if (isa<CXXNullPtrLiteralExpr>(expr))
        return "nullptr";

    return "";
}

auto StatementAnalyzer::extractOperatorKind(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
        return "none";

    if (const auto* binOp = dyn_cast<BinaryOperator>(expr))
        return BinaryOperator::getOpcodeStr(binOp->getOpcode()).str();
    if (const auto* unOp = dyn_cast<UnaryOperator>(expr))
        return UnaryOperator::getOpcodeStr(unOp->getOpcode()).str();
    if (dyn_cast<ConditionalOperator>(expr) != nullptr)
        return "?:";

    return "none";
}

auto StatementAnalyzer::isExpressionConstexpr(const clang::Expr* expr) -> bool
{
    if (expr == nullptr)
        return false;

    if (expr->isValueDependent() || expr->isTypeDependent())
        return false;

    // Try to evaluate as constant expression
    Expr::EvalResult result;
    return expr->EvaluateAsConstantExpr(result, *astContext);
}

auto StatementAnalyzer::extractEvaluationResult(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
        return "";

    if (expr->isValueDependent() || expr->isTypeDependent())
        return "dependent";

    Expr::EvalResult result;
    if (expr->EvaluateAsConstantExpr(result, *astContext))
    {
        if (result.Val.isInt())
            return std::to_string(result.Val.getInt().getLimitedValue());
        if (result.Val.isFloat())
        {
            llvm::SmallString<16> str;
            result.Val.getFloat().toString(str);
            return str.str().str();
        }
        return "constant";
    }

    return "not_constant";
}

auto StatementAnalyzer::extractImplicitCastKind(const clang::Expr* expr) -> std::string
{
    if (const auto* implicitCast = dyn_cast<ImplicitCastExpr>(expr))
        return {implicitCast->getCastKindName()};
    return "none";
}
