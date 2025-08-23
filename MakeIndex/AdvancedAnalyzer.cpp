//===--- AdvancedAnalyzer.cpp - Advanced analysis (CFG, macros, etc.) --===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#include "AdvancedAnalyzer.h"

#include "ASTNodeProcessor.h"
#include "KuzuDatabase.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Analysis/CFG.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>
#include <sstream>

using namespace clang;

AdvancedAnalyzer::AdvancedAnalyzer(KuzuDatabase& database, ASTNodeProcessor& nodeProcessor, ASTContext& astContext)
    : database(database), nodeProcessor(nodeProcessor), astContext(&astContext)
{
}

void AdvancedAnalyzer::analyzeCFGForFunction(const clang::FunctionDecl* func, int64_t functionNodeId)
{
    if (!database.isInitialized() || (func == nullptr) || functionNodeId == -1)
        return;

    // Only analyze functions with bodies
    if (!func->hasBody())
        return;

    try
    {
        // Build CFG for the function
        CFG::BuildOptions buildOptions;
        buildOptions.AddEHEdges = true;
        buildOptions.AddInitializers = true;
        buildOptions.AddImplicitDtors = true;
        buildOptions.AddLifetime = true;
        buildOptions.AddLoopExit = true;
        buildOptions.AddTemporaryDtors = true;

        std::unique_ptr<CFG> cfg = CFG::buildCFG(func, func->getBody(), astContext, buildOptions);
        if (!cfg)
            return;

        // Process each CFG block
        for (CFG::const_iterator blockIt = cfg->begin(); blockIt != cfg->end(); ++blockIt)
        {
            const CFGBlock* block = *blockIt;
            if (block == nullptr)
                continue;

            int blockIndex = block->getBlockID();
            bool isEntry = (block == &cfg->getEntry());
            bool isExit = (block == &cfg->getExit());

            // Create node for this CFG block
            static int64_t cfgBlockIdCounter = 2000000;  // Start high to avoid conflicts
            int64_t blockNodeId = cfgBlockIdCounter++;

            createCFGBlockNode(blockNodeId, functionNodeId, block, blockIndex, isEntry, isExit);

            // Create edges to successor blocks
            for (CFGBlock::const_succ_iterator succIt = block->succ_begin(); succIt != block->succ_end(); ++succIt)
            {
                const CFGBlock* succBlock = succIt->getReachableBlock();
                if (succBlock != nullptr)
                {
                    int64_t succBlockNodeId = cfgBlockIdCounter + succBlock->getBlockID();
                    std::string edgeType = extractCFGEdgeType(*block);
                    std::string condition = extractCFGCondition(block);

                    createCFGEdgeRelation(blockNodeId, succBlockNodeId, edgeType, condition);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception analyzing CFG: " << e.what() << "\n";
    }
}

void AdvancedAnalyzer::createConstantExpressionNode(int64_t nodeId,
                                                    const clang::Expr* expr,
                                                    bool isConstexprFunction,
                                                    const std::string& evaluationContext)
{
    if (!database.isInitialized() || (expr == nullptr))
        return;

    try
    {
        std::string evaluationResult = evaluateConstantExpression(expr);
        std::string resultType = expr->getType().isNull() ? "unknown" : expr->getType().getAsString();
        bool isCompileTimeConstant = expr->isEvaluatable(*astContext);
        auto [constantValue, constantType] = extractConstantValue(expr);
        std::string evaluationStatus = extractEvaluationStatus(expr);

        // Escape single quotes for database storage
        std::string cleanEvaluationContext = evaluationContext;
        std::string cleanEvaluationResult = evaluationResult;
        std::string cleanResultType = resultType;
        std::string cleanConstantValue = constantValue;
        std::string cleanConstantType = constantType;

        std::ranges::replace(cleanEvaluationContext, '\'', '_');
        std::ranges::replace(cleanEvaluationResult, '\'', '_');
        std::ranges::replace(cleanResultType, '\'', '_');
        std::ranges::replace(cleanConstantValue, '\'', '_');
        std::ranges::replace(cleanConstantType, '\'', '_');

        std::string query = "CREATE (ce:ConstantExpression {node_id: " + std::to_string(nodeId) +
                            ", is_constexpr_function: " + (isConstexprFunction ? "true" : "false") +
                            ", evaluation_context: '" + cleanEvaluationContext + "', evaluation_result: '" +
                            cleanEvaluationResult + "', result_type: '" + cleanResultType +
                            "', is_compile_time_constant: " + (isCompileTimeConstant ? "true" : "false") +
                            ", constant_value: '" + cleanConstantValue + "', constant_type: '" + cleanConstantType +
                            "', evaluation_status: '" + evaluationStatus + "'})";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating ConstantExpression node: " << e.what() << "\n";
    }
}

void AdvancedAnalyzer::createStaticAssertionNode(int64_t nodeId, const clang::StaticAssertDecl* assertDecl)
{
    if (!database.isInitialized() || (assertDecl == nullptr))
        return;

    try
    {
        auto [assertionExpression, assertionMessage, assertionResult] = extractStaticAssertInfo(assertDecl);
        std::string failureReason = assertionResult ? "" : "static_assert_failed";
        std::string evaluationContext = "compile_time";

        // Escape single quotes for database storage
        std::ranges::replace(assertionExpression, '\'', '_');
        std::ranges::replace(assertionMessage, '\'', '_');
        std::ranges::replace(failureReason, '\'', '_');

        std::string query = "CREATE (sa:StaticAssertion {node_id: " + std::to_string(nodeId) +
                            ", assertion_expression: '" + assertionExpression + "', assertion_message: '" +
                            assertionMessage + "', assertion_result: " + (assertionResult ? "true" : "false") +
                            ", failure_reason: '" + failureReason + "', evaluation_context: '" + evaluationContext +
                            "'})";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating StaticAssertion node: " << e.what() << "\n";
    }
}

void AdvancedAnalyzer::createMacroDefinitionNode(int64_t nodeId,
                                                 const std::string& macroName,
                                                 bool isFunctionLike,
                                                 const std::vector<std::string>& parameters,
                                                 const std::string& replacementText,
                                                 bool isBuiltin,
                                                 bool isConditional)
{
    if (!database.isInitialized())
        return;

    try
    {
        auto parameterCount = static_cast<int64_t>(parameters.size());

        // Join parameters into a single string
        std::string parameterNames;
        for (size_t i = 0; i < parameters.size(); ++i)
        {
            if (i > 0)
                parameterNames += ", ";
            parameterNames += parameters[i];
        }

        // Escape single quotes for database storage
        std::string cleanMacroName = macroName;
        std::string cleanParameterNames = parameterNames;
        std::string cleanReplacementText = replacementText;

        std::ranges::replace(cleanMacroName, '\'', '_');
        std::ranges::replace(cleanParameterNames, '\'', '_');
        std::ranges::replace(cleanReplacementText, '\'', '_');

        // Limit text length for database storage
        if (cleanReplacementText.length() > 1000)
            cleanReplacementText = cleanReplacementText.substr(0, 1000) + "...";

        std::string query = "CREATE (md:MacroDefinition {node_id: " + std::to_string(nodeId) + ", macro_name: '" +
                            cleanMacroName + "', is_function_like: " + (isFunctionLike ? "true" : "false") +
                            ", parameter_count: " + std::to_string(parameterCount) + ", parameter_names: '" +
                            cleanParameterNames + "', replacement_text: '" + cleanReplacementText +
                            "', is_builtin: " + (isBuiltin ? "true" : "false") +
                            ", is_conditional: " + (isConditional ? "true" : "false") + "})";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating MacroDefinition node: " << e.what() << "\n";
    }
}

auto AdvancedAnalyzer::evaluateConstantExpression(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
        return "null_expression";

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
        if (result.Val.isLValue())
            return "lvalue_constant";
        return "other_constant";
    }

    return "not_constant";
}

auto AdvancedAnalyzer::extractConstantValue(const clang::Expr* expr) -> std::pair<std::string, std::string>
{
    if (expr == nullptr)
        return {"", ""};

    if (expr->isValueDependent() || expr->isTypeDependent())
        return {"dependent", "dependent"};

    Expr::EvalResult result;
    if (expr->EvaluateAsConstantExpr(result, *astContext))
    {
        std::string type = expr->getType().isNull() ? "unknown" : expr->getType().getAsString();

        if (result.Val.isInt())
            return {std::to_string(result.Val.getInt().getLimitedValue()), type};
        if (result.Val.isFloat())
        {
            llvm::SmallString<16> str;
            result.Val.getFloat().toString(str);
            return {str.str().str(), type};
        }
        return {"constant", type};
    }

    return {"not_constant", expr->getType().isNull() ? "unknown" : expr->getType().getAsString()};
}

auto AdvancedAnalyzer::extractEvaluationStatus(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
        return "null";

    if (expr->isValueDependent())
        return "value_dependent";
    if (expr->isTypeDependent())
        return "type_dependent";
    if (expr->isInstantiationDependent())
        return "instantiation_dependent";
    if (expr->containsUnexpandedParameterPack())
        return "unexpanded_pack";

    if (expr->isEvaluatable(*astContext))
        return "evaluatable";

    return "not_evaluatable";
}

auto AdvancedAnalyzer::detectConstexprFunction(const clang::FunctionDecl* func) -> bool
{
    return func != nullptr && func->isConstexpr();
}

auto AdvancedAnalyzer::extractStaticAssertInfo(const clang::StaticAssertDecl* assertDecl)
    -> std::tuple<std::string, std::string, bool>
{
    if (assertDecl == nullptr)
        return {"", "", false};

    std::string expression = nodeProcessor.extractSourceLocation(assertDecl->getAssertExpr()->getBeginLoc());

    std::string message;
    if (const Expr* messageExpr = assertDecl->getMessage())
    {
        if (const auto* stringLiteral = dyn_cast<StringLiteral>(messageExpr))
            message = stringLiteral->getString().str();
        else
            message = nodeProcessor.extractSourceLocation(messageExpr->getBeginLoc());
    }

    // Try to evaluate the assertion
    bool result = false;
    if (const Expr* assertExpr = assertDecl->getAssertExpr())
    {
        Expr::EvalResult evalResult;
        if (assertExpr->EvaluateAsConstantExpr(evalResult, *astContext))
        {
            if (evalResult.Val.isInt())
                result = evalResult.Val.getInt().getBoolValue();
        }
    }

    return {expression, message, result};
}

void AdvancedAnalyzer::createCFGBlockNode(int64_t blockNodeId,
                                          int64_t functionNodeId,
                                          const clang::CFGBlock* block,
                                          int blockIndex,
                                          bool isEntry,
                                          bool isExit)
{
    if (!database.isInitialized() || (block == nullptr))
        return;

    try
    {
        std::string terminatorKind = "none";
        std::string blockContent = extractCFGBlockContent(block);
        std::string conditionExpression = extractCFGCondition(block);
        bool hasTerminator = (block->getTerminator().getStmt() != nullptr);
        bool reachable = !block->hasNoReturnElement();

        if (hasTerminator)
        {
            const Stmt* terminator = block->getTerminator().getStmt();
            if (terminator != nullptr)
                terminatorKind = terminator->getStmtClassName();
        }

        // Escape single quotes for database storage
        std::ranges::replace(blockContent, '\'', '_');
        std::ranges::replace(conditionExpression, '\'', '_');

        std::string query = "CREATE (cfgb:CFGBlock {node_id: " + std::to_string(blockNodeId) +
                            ", function_id: " + std::to_string(functionNodeId) +
                            ", block_index: " + std::to_string(blockIndex) +
                            ", is_entry_block: " + (isEntry ? "true" : "false") +
                            ", is_exit_block: " + (isExit ? "true" : "false") + ", terminator_kind: '" +
                            terminatorKind + "', block_content: '" + blockContent + "', condition_expression: '" +
                            conditionExpression + "', has_terminator: " + (hasTerminator ? "true" : "false") +
                            ", reachable: " + (reachable ? "true" : "false") + "})";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating CFGBlock node: " << e.what() << "\n";
    }
}

void AdvancedAnalyzer::createCFGEdgeRelation(int64_t fromBlockId,
                                             int64_t toBlockId,
                                             const std::string& edgeType,
                                             const std::string& condition)
{
    if (!database.isInitialized() || fromBlockId == -1 || toBlockId == -1)
        return;

    try
    {
        std::string cleanCondition = condition;
        std::ranges::replace(cleanCondition, '\'', '_');

        std::string query = "MATCH (from:CFGBlock {node_id: " + std::to_string(fromBlockId) + "}), " +
                            "(to:CFGBlock {node_id: " + std::to_string(toBlockId) + "}) " +
                            "CREATE (from)-[:CFG_EDGE {edge_type: '" + edgeType + "', condition: '" + cleanCondition +
                            "'}]->(to)";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating CFG_EDGE relationship: " << e.what() << "\n";
    }
}

auto AdvancedAnalyzer::extractCFGBlockContent(const clang::CFGBlock* block) -> std::string
{
    if (block == nullptr)
        return "";

    std::ostringstream content;
    content << "block_" << block->getBlockID() << "_elements_" << block->size();
    return content.str();
}

auto AdvancedAnalyzer::extractCFGEdgeType(const clang::CFGBlock& from) -> std::string
{
    if (from.getTerminator().getStmt() != nullptr)
    {
        const Stmt* terminator = from.getTerminator().getStmt();
        if (isa<IfStmt>(terminator))
            return "conditional";
        if (isa<WhileStmt>(terminator) || isa<ForStmt>(terminator))
            return "loop";
        if (isa<SwitchStmt>(terminator))
            return "switch";
        return "terminator";
    }
    return "fallthrough";
}

auto AdvancedAnalyzer::extractCFGCondition(const clang::CFGBlock* block) -> std::string
{
    if (block == nullptr || block->getTerminator().getStmt() == nullptr)
        return "";

    const Stmt* terminator = block->getTerminator().getStmt();
    if (const auto* ifStmt = dyn_cast<IfStmt>(terminator))
    {
        if (const Expr* cond = ifStmt->getCond())
            return nodeProcessor.extractSourceLocation(cond->getBeginLoc());
    }
    else if (const auto* whileStmt = dyn_cast<WhileStmt>(terminator))
    {
        if (const Expr* cond = whileStmt->getCond())
            return nodeProcessor.extractSourceLocation(cond->getBeginLoc());
    }

    return "";
}
