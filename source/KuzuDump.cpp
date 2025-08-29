//===--- KuzuDump.cpp - AST dumping implementation -----------------------===//
//
// Part of the Dosatsu project
// This demonstrates the refactored architecture using specialized analyzers
//
//===----------------------------------------------------------------------===//

#include "KuzuDump.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/DeclCXX.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

using namespace clang;

KuzuDump::KuzuDump(raw_ostream& OS, ASTContext& Context, bool ShowColors)
    : nullStream(nullptr), NodeDumper(OS, Context, ShowColors), OS(OS), database(nullptr)
{
    initializeAnalyzers(Context);
}

KuzuDump::KuzuDump(std::string databasePath, ASTContext& Context, bool ShowColors)
    : nullStream(std::make_unique<llvm::raw_null_ostream>()), NodeDumper(*nullStream, Context, ShowColors),
      OS(*nullStream)
{
    // Get global database instance - initialize if not already done
    auto& dbManager = GlobalDatabaseManager::getInstance();
    if (!dbManager.isInitialized())
        dbManager.initializeDatabase(databasePath);
    database = dbManager.getDatabase();

    initializeAnalyzers(Context);
}

KuzuDump::KuzuDump(std::string databasePath, ASTContext& Context, bool ShowColors, bool pureDatabaseMode)
    : nullStream(std::make_unique<llvm::raw_null_ostream>()), NodeDumper(*nullStream, Context, ShowColors),
      OS(*nullStream), databaseOnlyMode(pureDatabaseMode)
{
    // Get global database instance - initialize if not already done
    auto& dbManager = GlobalDatabaseManager::getInstance();
    if (!dbManager.isInitialized())
        dbManager.initializeDatabase(databasePath);
    database = dbManager.getDatabase();

    initializeAnalyzers(Context);
}

KuzuDump::~KuzuDump() = default;

void KuzuDump::initializeAnalyzers(ASTContext& Context)
{
    if (database == nullptr)
    {
        // For text-only mode, we don't need a database
        return;
    }

    // Initialize all analyzers
    nodeProcessor = std::make_unique<ASTNodeProcessor>(*database, Context);
    scopeManager = std::make_unique<ScopeManager>(*database);
    typeAnalyzer = std::make_unique<TypeAnalyzer>(*database, *nodeProcessor, Context);
    declarationAnalyzer = std::make_unique<DeclarationAnalyzer>(*database);
    statementAnalyzer = std::make_unique<StatementAnalyzer>(*database, *nodeProcessor, Context);
    templateAnalyzer = std::make_unique<TemplateAnalyzer>(*database, *nodeProcessor, Context);
    commentProcessor = std::make_unique<CommentProcessor>(*database, *nodeProcessor, Context);
    advancedAnalyzer = std::make_unique<AdvancedAnalyzer>(*database, *nodeProcessor, Context);
}


void KuzuDump::VisitDecl(const Decl* D)
{
    if (D == nullptr)
        return;

    // Text output (if enabled)
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);

    // Database processing using analyzers
    processDeclaration(D);
}

void KuzuDump::VisitFunctionDecl(const FunctionDecl* D)
{
    if (D == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // Create declaration node using declaration analyzer
    if (const auto* namedDecl = dyn_cast<NamedDecl>(D))
        declarationAnalyzer->createDeclarationNode(nodeId, namedDecl);

    // Create type relationship using type analyzer
    typeAnalyzer->createTypeNodeAndRelation(nodeId, D->getType());

    // Process comments using comment processor
    commentProcessor->processComments(D, nodeId);

    // Handle template functions using template analyzer
    if (D->isFunctionTemplateSpecialization())
        templateAnalyzer->processTemplateSpecialization(nodeId, D);

    // Handle constexpr functions using advanced analyzer
    if (advancedAnalyzer->detectConstexprFunction(D))
    {
        // Create constant expression analysis for constexpr functions
        if (D->hasBody())
        {
            int64_t constexprNodeId = nodeProcessor->createASTNode(D->getBody());
            if (constexprNodeId != -1)
                advancedAnalyzer->createConstantExpressionNode(
                    constexprNodeId, dyn_cast<Expr>(D->getBody()), true, "function_body");
        }
    }

    // Analyze CFG for function body using advanced analyzer
    if (D->hasBody())
        advancedAnalyzer->analyzeCFGForFunction(D, nodeId);

    // Manage scope relationships
    scopeManager->createScopeRelationships(nodeId);

    // Push function as new scope for its contents
    scopeManager->pushScope(nodeId);

    // Push function as parent for its contents (for PARENT_OF relationships)
    scopeManager->pushParent(nodeId);

    // Standard AST traversal
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);

    // Continue recursive traversal for child nodes (always needed for database)
    ASTNodeTraverser<KuzuDump, TextNodeDumper>::VisitFunctionDecl(D);

    // Pop function scope and parent
    scopeManager->popParent();
    scopeManager->popScope();
}

void KuzuDump::VisitVarDecl(const VarDecl* D)
{
    if (D == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // Create declaration node using declaration analyzer
    declarationAnalyzer->createDeclarationNode(nodeId, D);

    // Create type relationship using type analyzer
    typeAnalyzer->createTypeNodeAndRelation(nodeId, D->getType());

    // Manage scope relationships
    scopeManager->createScopeRelationships(nodeId);

    // Standard AST traversal
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);

    // Continue recursive traversal for child nodes (needed for initializer expressions)
    ASTNodeTraverser<KuzuDump, TextNodeDumper>::VisitVarDecl(D);

    // Note: In full implementation, would also:
    // - Process variable initializers
    // - Handle static/extern storage classes
    // - Process variable templates
    // - Analyze constant expressions
}

void KuzuDump::VisitNamespaceDecl(const NamespaceDecl* D)
{
    if (D == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // Create declaration node using declaration analyzer
    declarationAnalyzer->createDeclarationNode(nodeId, D);

    // Create scope relationships for this namespace
    scopeManager->createScopeRelationships(nodeId);

    // Push this namespace as a new scope for its contents
    scopeManager->pushScope(nodeId);

    // Push namespace as parent for its contents (for PARENT_OF relationships)
    scopeManager->pushParent(nodeId);

    // Standard AST traversal
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);

    // Continue recursive traversal for child nodes
    ASTNodeTraverser<KuzuDump, TextNodeDumper>::VisitNamespaceDecl(D);

    // Pop namespace scope and parent after traversal
    scopeManager->popParent();
    scopeManager->popScope();
}

void KuzuDump::VisitUsingDecl(const UsingDecl* D)
{
    if (D == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // Create using declaration node using declaration analyzer
    declarationAnalyzer->createUsingDeclarationNode(nodeId, D);

    // Create scope relationships
    scopeManager->createScopeRelationships(nodeId);

    // Standard AST traversal
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);
}

void KuzuDump::VisitUsingDirectiveDecl(const UsingDirectiveDecl* D)
{
    if (D == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // Create using directive node using declaration analyzer
    declarationAnalyzer->createUsingDirectiveNode(nodeId, D);

    // Create scope relationships
    scopeManager->createScopeRelationships(nodeId);

    // Create reference relationship to the nominated namespace
    if (const auto* nominatedNS = D->getNominatedNamespace())
    {
        int64_t nsNodeId = nodeProcessor->getNodeId(nominatedNS);
        if (nsNodeId == -1)
            nsNodeId = nodeProcessor->createASTNode(nominatedNS);

        if (nsNodeId != -1)
            declarationAnalyzer->createReferenceRelation(nodeId, nsNodeId, "using_directive");
    }

    // Standard AST traversal
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);
}

void KuzuDump::VisitNamespaceAliasDecl(const NamespaceAliasDecl* D)
{
    if (D == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // Create namespace alias node using declaration analyzer
    declarationAnalyzer->createNamespaceAliasNode(nodeId, D);

    // Create scope relationships
    scopeManager->createScopeRelationships(nodeId);

    // Create reference relationship to the aliased namespace
    if (const auto* aliasedNS = D->getNamespace())
    {
        int64_t aliasedNodeId = nodeProcessor->getNodeId(aliasedNS);
        if (aliasedNodeId == -1)
            aliasedNodeId = nodeProcessor->createASTNode(aliasedNS);

        if (aliasedNodeId != -1)
            declarationAnalyzer->createReferenceRelation(nodeId, aliasedNodeId, "namespace_alias");
    }

    // Standard AST traversal
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);
}

void KuzuDump::VisitCXXRecordDecl(const CXXRecordDecl* D)
{
    if (D == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // Create declaration node using declaration analyzer
    declarationAnalyzer->createDeclarationNode(nodeId, D);

    // Create scope relationships
    scopeManager->createScopeRelationships(nodeId);

    // Push class as new scope for its members
    scopeManager->pushScope(nodeId);

    // Push class as parent for its members (for PARENT_OF relationships)
    scopeManager->pushParent(nodeId);

    // Standard AST traversal
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);

    // Process inheritance relationships
    if (D->hasDefinition())
    {
        for (const auto& base : D->bases())
        {
            const CXXRecordDecl* baseDecl = base.getType()->getAsCXXRecordDecl();
            if (baseDecl != nullptr)
            {
                // Create AST node for base class if it doesn't exist
                int64_t baseNodeId = nodeProcessor->createASTNode(baseDecl);
                if (baseNodeId != -1)
                {
                    // Create declaration node for base class
                    declarationAnalyzer->createDeclarationNode(baseNodeId, baseDecl);

                    // Create INHERITS_FROM relationship
                    std::string inheritanceType = "public";
                    if (base.getAccessSpecifier() == AS_private)
                        inheritanceType = "private";
                    else if (base.getAccessSpecifier() == AS_protected)
                        inheritanceType = "protected";

                    bool isVirtual = base.isVirtual();

                    std::map<std::string, std::string> properties;
                    properties["inheritance_type"] = inheritanceType;
                    properties["is_virtual"] = isVirtual ? "true" : "false";
                    properties["base_access_path"] = "";
                    database->addRelationshipToBatch(nodeId, baseNodeId, "INHERITS_FROM", properties);
                }
            }
        }
    }

    // Note: In full implementation, would also:
    // - Handle virtual methods and overrides
    // - Process class templates
    // - Analyze access specifiers
    // - Handle friend declarations

    // Continue recursive traversal for child nodes
    ASTNodeTraverser<KuzuDump, TextNodeDumper>::VisitCXXRecordDecl(D);

    // Pop class scope and parent
    scopeManager->popParent();
    scopeManager->popScope();
}

void KuzuDump::VisitClassTemplateDecl(const ClassTemplateDecl* D)
{
    if (D == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // Create declaration node using declaration analyzer
    declarationAnalyzer->createDeclarationNode(nodeId, D);

    // Process template using template analyzer
    templateAnalyzer->processTemplateDecl(nodeId, D);

    // Process comments
    commentProcessor->processComments(D, nodeId);

    // Create scope relationships
    scopeManager->createScopeRelationships(nodeId);

    // Push template as new scope
    scopeManager->pushScope(nodeId);

    // Standard AST traversal
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);

    // Pop template scope
    scopeManager->popScope();
}

void KuzuDump::VisitFunctionTemplateDecl(const FunctionTemplateDecl* D)
{
    if (D == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // Create declaration node using declaration analyzer
    declarationAnalyzer->createDeclarationNode(nodeId, D);

    // Process template using template analyzer
    templateAnalyzer->processTemplateDecl(nodeId, D);

    // Process comments
    commentProcessor->processComments(D, nodeId);

    // Create scope relationships
    scopeManager->createScopeRelationships(nodeId);

    // Standard AST traversal
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);
}

void KuzuDump::VisitClassTemplateSpecializationDecl(const ClassTemplateSpecializationDecl* D)
{
    if (D == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // Create declaration node using declaration analyzer
    declarationAnalyzer->createDeclarationNode(nodeId, D);

    // Process template specialization using template analyzer
    templateAnalyzer->processTemplateSpecialization(nodeId, D);

    // Process comments
    commentProcessor->processComments(D, nodeId);

    // Create scope relationships
    scopeManager->createScopeRelationships(nodeId);

    // Push specialization as new scope
    scopeManager->pushScope(nodeId);

    // Standard AST traversal
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);

    // Pop specialization scope
    scopeManager->popScope();
}

void KuzuDump::VisitStaticAssertDecl(const StaticAssertDecl* D)
{
    if (D == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // StaticAssertDecl is a Decl but not a NamedDecl, so we skip declaration analyzer
    // and go straight to advanced analyzer for static assertion processing

    // Process static assertion using advanced analyzer
    advancedAnalyzer->createStaticAssertionNode(nodeId, D);

    // Create scope relationships
    scopeManager->createScopeRelationships(nodeId);

    // Standard AST traversal
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);
}

void KuzuDump::VisitTranslationUnitDecl(const TranslationUnitDecl* D)
{
    if (D == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // TranslationUnitDecl is not a NamedDecl, so we skip the declaration analyzer
    // and only create basic AST node processing

    // Create scope relationships - this is the root scope
    scopeManager->createScopeRelationships(nodeId);

    // Push translation unit as global scope
    scopeManager->pushScope(nodeId);

    // Push translation unit as parent for top-level declarations
    scopeManager->pushParent(nodeId);

    // Standard AST traversal
    if (!databaseOnlyMode)
        NodeDumper.Visit(D);

    // Let the base traverser handle child node traversal

    // Pop translation unit scope and parent
    scopeManager->popParent();
    scopeManager->popScope();
}

void KuzuDump::VisitStmt(const Stmt* S)
{
    if (S == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(S);
    if (nodeId == -1)
        return;

    // Create statement node using statement analyzer
    statementAnalyzer->createStatementNode(nodeId, S);

    // Create hierarchy relationships
    scopeManager->createHierarchyRelationship(nodeId);

    // Push statement as parent for its child expressions/statements
    scopeManager->pushParent(nodeId);

    // Text output (if enabled)
    if (!databaseOnlyMode)
        NodeDumper.Visit(S);

    // Continue recursive traversal for child nodes
    ASTNodeTraverser<KuzuDump, TextNodeDumper>::VisitStmt(S);

    // Pop statement parent
    scopeManager->popParent();
}

void KuzuDump::VisitReturnStmt(const ReturnStmt* S)
{
    if (S == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(S);
    if (nodeId == -1)
        return;

    // Create statement node using statement analyzer
    statementAnalyzer->createStatementNode(nodeId, S);

    // Create hierarchy relationships
    scopeManager->createHierarchyRelationship(nodeId);

    // Push return statement as parent for its child expressions
    scopeManager->pushParent(nodeId);

    // Text output (if enabled)
    if (!databaseOnlyMode)
        NodeDumper.Visit(S);

    // Continue recursive traversal for child nodes (return value expression)
    ASTNodeTraverser<KuzuDump, TextNodeDumper>::VisitStmt(S);

    // Pop return statement parent
    scopeManager->popParent();
}

void KuzuDump::VisitExpr(const Expr* E)
{
    if (E == nullptr)
        return;

    // Create AST node using node processor
    int64_t nodeId = nodeProcessor->createASTNode(E);
    if (nodeId == -1)
        return;

    // Create expression node using statement analyzer
    statementAnalyzer->createExpressionNode(nodeId, E);

    // Analyze constant expressions using advanced analyzer
    if (statementAnalyzer->isExpressionConstexpr(E))
        advancedAnalyzer->createConstantExpressionNode(nodeId, E, false, "expression_evaluation");

    // Create hierarchy relationships
    scopeManager->createHierarchyRelationship(nodeId);

    // Push expression as parent for its child sub-expressions
    scopeManager->pushParent(nodeId);

    // Text output (if enabled)
    if (!databaseOnlyMode)
        NodeDumper.Visit(E);

    // Continue recursive traversal for child nodes
    ASTNodeTraverser<KuzuDump, TextNodeDumper>::VisitExpr(E);

    // Pop expression parent
    scopeManager->popParent();
}

void KuzuDump::processDeclaration(const Decl* D)
{
    if ((D == nullptr) || !database->isInitialized())
        return;

    // Create basic AST node
    int64_t nodeId = nodeProcessor->createASTNode(D);
    if (nodeId == -1)
        return;

    // Create hierarchy relationships
    scopeManager->createHierarchyRelationship(nodeId);

    // Process specialized declaration types
    if (const auto* namedDecl = dyn_cast<NamedDecl>(D))
    {
        declarationAnalyzer->createDeclarationNode(nodeId, namedDecl);

        // Create type relationships for typed declarations
        if (const auto* valueDecl = dyn_cast<ValueDecl>(namedDecl))
            typeAnalyzer->createTypeNodeAndRelation(nodeId, valueDecl->getType());
        else if (const auto* functionDecl = dyn_cast<FunctionDecl>(namedDecl))
            typeAnalyzer->createTypeNodeAndRelation(nodeId, functionDecl->getType());
    }

    // Note: Additional processing would be handled by specialized analyzers:
    // - Template processing by TemplateAnalyzer
    // - Comment processing by CommentProcessor
    // - Constant expression analysis by AdvancedAnalyzer
}

void KuzuDump::processStatement(const Stmt* S)
{
    if ((S == nullptr) || !database->isInitialized())
        return;

    // Create basic AST node
    int64_t nodeId = nodeProcessor->createASTNode(S);
    if (nodeId == -1)
        return;

    // Create hierarchy relationships
    scopeManager->createHierarchyRelationship(nodeId);

    // Create statement node using StatementAnalyzer
    statementAnalyzer->createStatementNode(nodeId, S);

    // For expressions, delegate to specialized expression handling
    if (const auto* expr = dyn_cast<Expr>(S))
        statementAnalyzer->createExpressionNode(nodeId, expr);
}

// Legacy compatibility methods
void KuzuDump::dumpInvalidDeclContext(const DeclContext* DC)
{
    (void)DC;  // Unused parameter - kept for interface compatibility
    // Simplified implementation for compatibility
    if (!databaseOnlyMode)
        llvm::outs() << "Invalid DeclContext\n";
}

void KuzuDump::dumpLookups(const DeclContext* DC, bool DumpDecls)
{
    (void)DC;         // Unused parameter - kept for interface compatibility
    (void)DumpDecls;  // Unused parameter - kept for interface compatibility
    // Simplified implementation for compatibility
    if (!databaseOnlyMode)
        llvm::outs() << "Lookup information (simplified)\n";
}
