//===--- AdvancedAnalyzer.h - Advanced analysis (CFG, macros, etc.) ----===//
//
// Part of the Dosatsu project
//
//===----------------------------------------------------------------------===//

#pragma once

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/Analysis/CFG.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace clang
{

class KuzuDatabase;
class ASTNodeProcessor;

/// Handles advanced analysis including CFG, constant expressions, macros
class AdvancedAnalyzer
{
public:
    /// Constructor
    /// \param database Database instance for storage
    /// \param nodeProcessor Node processor for creating basic nodes
    /// \param astContext AST context for analysis
    AdvancedAnalyzer(KuzuDatabase& database, ASTNodeProcessor& nodeProcessor, ASTContext& astContext);

    /// Analyze CFG for a function
    /// \param func Function declaration to analyze
    /// \param functionNodeId Node ID of the function
    void analyzeCFGForFunction(const clang::FunctionDecl* func, int64_t functionNodeId);

    /// Create constant expression node
    /// \param nodeId Node ID for the constant expression
    /// \param expr Expression to analyze
    /// \param isConstexprFunction Whether this is in a constexpr function
    /// \param evaluationContext Context of evaluation
    void createConstantExpressionNode(int64_t nodeId,
                                      const clang::Expr* expr,
                                      bool isConstexprFunction,
                                      const std::string& evaluationContext);

    /// Create static assertion node
    /// \param nodeId Node ID for the static assert
    /// \param assertDecl Static assert declaration
    void createStaticAssertionNode(int64_t nodeId, const clang::StaticAssertDecl* assertDecl);

    /// Create macro definition node
    /// \param nodeId Node ID for the macro
    /// \param macroName Name of the macro
    /// \param isFunctionLike Whether macro is function-like
    /// \param parameters Macro parameters
    /// \param replacementText Replacement text
    /// \param isBuiltin Whether this is a builtin macro
    /// \param isConditional Whether this is conditional
    void createMacroDefinitionNode(int64_t nodeId,
                                   const std::string& macroName,
                                   bool isFunctionLike,
                                   const std::vector<std::string>& parameters,
                                   const std::string& replacementText,
                                   bool isBuiltin = false,
                                   bool isConditional = false);

    /// Evaluate constant expression
    /// \param expr Expression to evaluate
    /// \return String representation of the result
    auto evaluateConstantExpression(const clang::Expr* expr) -> std::string;

    /// Extract constant value from expression
    /// \param expr Expression to analyze
    /// \return Pair of (value, type)
    auto extractConstantValue(const clang::Expr* expr) -> std::pair<std::string, std::string>;

    /// Extract evaluation status
    /// \param expr Expression to analyze
    /// \return String representation of evaluation status
    auto extractEvaluationStatus(const clang::Expr* expr) -> std::string;

    /// Detect constexpr function
    /// \param func Function to check
    /// \return True if function is constexpr
    auto detectConstexprFunction(const clang::FunctionDecl* func) -> bool;

    /// Extract static assert information
    /// \param assertDecl Static assert declaration
    /// \return Tuple of (expression, message, result)
    auto extractStaticAssertInfo(const clang::StaticAssertDecl* assertDecl)
        -> std::tuple<std::string, std::string, bool>;

private:
    KuzuDatabase& database;
    ASTNodeProcessor& nodeProcessor;
    ASTContext* astContext;

    /// Create CFG block node
    /// \param blockNodeId Node ID for the block
    /// \param functionNodeId Function containing this block
    /// \param block CFG block
    /// \param blockIndex Index of block
    /// \param isEntry Whether this is entry block
    /// \param isExit Whether this is exit block
    void createCFGBlockNode(int64_t blockNodeId,
                            int64_t functionNodeId,
                            const clang::CFGBlock* block,
                            int blockIndex,
                            bool isEntry,
                            bool isExit);

    /// Create CFG edge relationship
    /// \param fromBlockId Source block node ID
    /// \param toBlockId Target block node ID
    /// \param edgeType Type of edge
    /// \param condition Condition (if applicable)
    void createCFGEdgeRelation(int64_t fromBlockId,
                               int64_t toBlockId,
                               const std::string& edgeType,
                               const std::string& condition = "");

    /// Extract CFG block content
    /// \param block CFG block to analyze
    /// \return String representation of block content
    auto extractCFGBlockContent(const clang::CFGBlock* block) -> std::string;

    /// Extract CFG edge type
    /// \param from Source CFG block
    /// \return String representation of edge type
    auto extractCFGEdgeType(const clang::CFGBlock& from) -> std::string;

    /// Extract CFG condition
    /// \param block CFG block to analyze
    /// \return String representation of condition
    auto extractCFGCondition(const clang::CFGBlock* block) -> std::string;
};

}  // namespace clang
