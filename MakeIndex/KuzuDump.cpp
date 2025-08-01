#include "KuzuDump.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <memory>
#include <sstream>
#include <utility>

// Bring base class's Visit methods into scope
using BaseTraverser = clang::ASTNodeTraverser<KuzuDump, KuzuNodeDumper>;

// Forward declarations for Kuzu types (will be implemented when we add Kuzu)
namespace kuzu
{
class Database
{
public:
    Database(const std::string& /*path*/) {}
};

class Connection
{
public:
    Connection(Database* /*db*/) {}
};

class Table
{
public:
    Table() = default;
};
}  // namespace kuzu

KuzuDump::KuzuDump(const clang::ASTContext& Context, std::string DatabasePath)
    : Context(Context), DatabasePath(std::move(DatabasePath))
{
    // TODO: Initialize Kuzu database when available
    // initializeDatabase();
}

KuzuDump::KuzuDump(const clang::ASTContext& Context) : Context(Context), DatabasePath("")
{
    // Test constructor - no database initialization
}

KuzuDump::~KuzuDump() = default;

void KuzuDump::dumpTranslationUnit(const clang::TranslationUnitDecl* TU)
{
    if (TU == nullptr)
        return;

    // TODO: Create translation unit node in database
    // insertASTNode("TranslationUnit", getNodeId(TU), "TranslationUnit", getSourceLocation(TU->getLocation()));

    // Traverse all declarations in the translation unit
    for (const auto* Decl : TU->decls())
    {
        BaseTraverser::Visit(Decl);
    }
}

// Declaration visitors
void KuzuDump::VisitDecl(const clang::Decl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert generic declaration node
    // insertASTNode("Decl", getNodeId(D), D->getDeclKindName(), getSourceLocation(D->getLocation()));

    // Call the base class implementation to continue traversal
    // Base::VisitDecl(D);
}

void KuzuDump::VisitFunctionDecl(const clang::FunctionDecl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert function declaration node with function-specific attributes
    std::string functionName = D->getNameAsString();
    std::string qualifiedName = getQualifiedName(D);

    // insertASTNode("FunctionDecl", getNodeId(D), functionName, getSourceLocation(D->getLocation()));

    // TODO: Add function-specific properties like return type, parameters, etc.
    // createTypeRelationship(D, D->getReturnType().getTypePtr());

    // Visit parameters
    for (const auto* Param : D->parameters())
    {
        BaseTraverser::Visit(Param);
        // createParentChildRelationship(D, Param);
    }

    // Visit function body if available
    if (const auto* Body = D->getBody())
    {
        BaseTraverser::Visit(Body);
        // createParentChildRelationship(D, Body);
    }

    VisitDecl(D);
}

void KuzuDump::VisitVarDecl(const clang::VarDecl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert variable declaration node
    std::string varName = D->getNameAsString();
    std::string typeStr = getTypeString(D->getType());

    // insertASTNode("VarDecl", getNodeId(D), varName, getSourceLocation(D->getLocation()));
    // createTypeRelationship(D, D->getType().getTypePtr());

    // Visit initializer if present
    if (const auto* Init = D->getInit())
    {
        BaseTraverser::Visit(Init);
        // createParentChildRelationship(D, Init);
    }

    VisitDecl(D);
}

void KuzuDump::VisitRecordDecl(const clang::RecordDecl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert record (struct/class/union) declaration node
    std::string recordName = D->getNameAsString();
    std::string recordKind = D->getKindName().str();  // struct, class, union

    // insertASTNode("RecordDecl", getNodeId(D), recordName, getSourceLocation(D->getLocation()));

    // Visit all fields
    for (const auto* Field : D->fields())
    {
        BaseTraverser::Visit(Field);
        // createParentChildRelationship(D, Field);
    }

    VisitDecl(D);
}

void KuzuDump::VisitCXXRecordDecl(const clang::CXXRecordDecl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert C++ class declaration with inheritance information
    std::string className = D->getNameAsString();

    // insertASTNode("CXXRecordDecl", getNodeId(D), className, getSourceLocation(D->getLocation()));

    // Visit base classes
    for (const auto& Base : D->bases())
    {
        const auto* BaseDecl = Base.getType()->getAsCXXRecordDecl();
        if (BaseDecl != nullptr)
        {
            // TODO: Create inheritance relationship
            // insertRelationship(getNodeId(D), getNodeId(BaseDecl), "inherits");
        }
    }

    // Visit methods
    for (const auto* Method : D->methods())
    {
        BaseTraverser::Visit(Method);
        // createParentChildRelationship(D, Method);
    }

    VisitRecordDecl(D);
}

void KuzuDump::VisitFieldDecl(const clang::FieldDecl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert field declaration node
    std::string fieldName = D->getNameAsString();
    std::string typeStr = getTypeString(D->getType());

    // insertASTNode("FieldDecl", getNodeId(D), fieldName, getSourceLocation(D->getLocation()));
    // createTypeRelationship(D, D->getType().getTypePtr());

    VisitDecl(D);
}

void KuzuDump::VisitParmVarDecl(const clang::ParmVarDecl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert parameter declaration node
    std::string paramName = D->getNameAsString();
    std::string typeStr = getTypeString(D->getType());

    // insertASTNode("ParmVarDecl", getNodeId(D), paramName, getSourceLocation(D->getLocation()));
    // createTypeRelationship(D, D->getType().getTypePtr());

    VisitVarDecl(D);
}

void KuzuDump::VisitTypedefDecl(const clang::TypedefDecl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert typedef declaration node
    std::string typedefName = D->getNameAsString();
    std::string underlyingType = getTypeString(D->getUnderlyingType());

    // insertASTNode("TypedefDecl", getNodeId(D), typedefName, getSourceLocation(D->getLocation()));
    // createTypeRelationship(D, D->getUnderlyingType().getTypePtr());

    VisitDecl(D);
}

void KuzuDump::VisitNamespaceDecl(const clang::NamespaceDecl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert namespace declaration node
    std::string namespaceName = D->getNameAsString();

    // insertASTNode("NamespaceDecl", getNodeId(D), namespaceName, getSourceLocation(D->getLocation()));

    // Visit all declarations in the namespace
    for (const auto* Decl : D->decls())
    {
        BaseTraverser::Visit(Decl);
        // createParentChildRelationship(D, Decl);
    }

    VisitDecl(D);
}

void KuzuDump::VisitUsingDecl(const clang::UsingDecl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert using declaration node
    std::string usingName = D->getNameAsString();

    // insertASTNode("UsingDecl", getNodeId(D), usingName, getSourceLocation(D->getLocation()));

    VisitDecl(D);
}

void KuzuDump::VisitCXXMethodDecl(const clang::CXXMethodDecl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert C++ method declaration with method-specific attributes
    std::string methodName = D->getNameAsString();
    bool isVirtual = D->isVirtual();
    bool isStatic = D->isStatic();

    // insertASTNode("CXXMethodDecl", getNodeId(D), methodName, getSourceLocation(D->getLocation()));

    VisitFunctionDecl(D);
}

void KuzuDump::VisitCXXConstructorDecl(const clang::CXXConstructorDecl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert constructor declaration node
    std::string constructorName = D->getNameAsString();

    // insertASTNode("CXXConstructorDecl", getNodeId(D), constructorName, getSourceLocation(D->getLocation()));

    VisitCXXMethodDecl(D);
}

void KuzuDump::VisitCXXDestructorDecl(const clang::CXXDestructorDecl* D)
{
    if (D == nullptr)
        return;

    // TODO: Insert destructor declaration node
    std::string destructorName = D->getNameAsString();

    // insertASTNode("CXXDestructorDecl", getNodeId(D), destructorName, getSourceLocation(D->getLocation()));

    VisitCXXMethodDecl(D);
}

// Statement visitors
void KuzuDump::VisitStmt(const clang::Stmt* S)
{
    if (S == nullptr)
        return;

    // TODO: Insert generic statement node
    // insertASTNode("Stmt", getNodeId(S), S->getStmtClassName(), getSourceLocation(S->getBeginLoc()));

    // Base::VisitStmt(S);
}

void KuzuDump::VisitExpr(const clang::Expr* E)
{
    if (E == nullptr)
        return;

    // TODO: Insert expression node with type information
    std::string typeStr = getTypeString(E->getType());

    // insertASTNode("Expr", getNodeId(E), E->getStmtClassName(), getSourceLocation(E->getBeginLoc()));
    // createTypeRelationship(E, E->getType().getTypePtr());

    VisitStmt(E);
}

void KuzuDump::VisitCallExpr(const clang::CallExpr* E)
{
    if (E == nullptr)
        return;

    // TODO: Insert function call expression node
    // insertASTNode("CallExpr", getNodeId(E), "CallExpr", getSourceLocation(E->getBeginLoc()));

    // Visit callee
    if (const auto* Callee = E->getCallee())
    {
        BaseTraverser::Visit(Callee);
        // createParentChildRelationship(E, Callee);
    }

    // Visit arguments
    for (const auto* Arg : E->arguments())
    {
        BaseTraverser::Visit(Arg);
        // createParentChildRelationship(E, Arg);
    }

    VisitExpr(E);
}

void KuzuDump::VisitDeclRefExpr(const clang::DeclRefExpr* E)
{
    if (E == nullptr)
        return;

    // TODO: Insert declaration reference expression node
    std::string declName = E->getDecl()->getNameAsString();

    // insertASTNode("DeclRefExpr", getNodeId(E), declName, getSourceLocation(E->getBeginLoc()));
    // createUsageRelationship(E, E->getDecl());

    VisitExpr(E);
}

void KuzuDump::VisitMemberExpr(const clang::MemberExpr* E)
{
    if (E == nullptr)
        return;

    // TODO: Insert member access expression node
    std::string memberName = E->getMemberDecl()->getNameAsString();

    // insertASTNode("MemberExpr", getNodeId(E), memberName, getSourceLocation(E->getBeginLoc()));
    // createUsageRelationship(E, E->getMemberDecl());

    // Visit base expression
    if (const auto* Base = E->getBase())
    {
        BaseTraverser::Visit(Base);
        // createParentChildRelationship(E, Base);
    }

    VisitExpr(E);
}

void KuzuDump::VisitCompoundStmt(const clang::CompoundStmt* S)
{
    if (S == nullptr)
        return;

    // TODO: Insert compound statement node
    // insertASTNode("CompoundStmt", getNodeId(S), "CompoundStmt", getSourceLocation(S->getBeginLoc()));

    // Visit all child statements
    for (const auto* Child : S->children())
    {
        BaseTraverser::Visit(Child);
        // createParentChildRelationship(S, Child);
    }

    VisitStmt(S);
}

void KuzuDump::VisitIfStmt(const clang::IfStmt* S)
{
    if (S == nullptr)
        return;

    // TODO: Insert if statement node
    // insertASTNode("IfStmt", getNodeId(S), "IfStmt", getSourceLocation(S->getBeginLoc()));

    // Visit condition
    if (const auto* Cond = S->getCond())
    {
        BaseTraverser::Visit(Cond);
        // createParentChildRelationship(S, Cond);
    }

    // Visit then statement
    if (const auto* Then = S->getThen())
    {
        BaseTraverser::Visit(Then);
        // createParentChildRelationship(S, Then);
    }

    // Visit else statement if present
    if (const auto* Else = S->getElse())
    {
        BaseTraverser::Visit(Else);
        // createParentChildRelationship(S, Else);
    }

    VisitStmt(S);
}

void KuzuDump::VisitForStmt(const clang::ForStmt* S)
{
    if (S == nullptr)
        return;

    // TODO: Insert for statement node
    // insertASTNode("ForStmt", getNodeId(S), "ForStmt", getSourceLocation(S->getBeginLoc()));

    // Visit init, condition, increment, and body
    if (const auto* Init = S->getInit())
    {
        BaseTraverser::Visit(Init);
        // createParentChildRelationship(S, Init);
    }

    if (const auto* Cond = S->getCond())
    {
        BaseTraverser::Visit(Cond);
        // createParentChildRelationship(S, Cond);
    }

    if (const auto* Inc = S->getInc())
    {
        BaseTraverser::Visit(Inc);
        // createParentChildRelationship(S, Inc);
    }

    if (const auto* Body = S->getBody())
    {
        BaseTraverser::Visit(Body);
        // createParentChildRelationship(S, Body);
    }

    VisitStmt(S);
}

void KuzuDump::VisitWhileStmt(const clang::WhileStmt* S)
{
    if (S == nullptr)
        return;

    // TODO: Insert while statement node
    // insertASTNode("WhileStmt", getNodeId(S), "WhileStmt", getSourceLocation(S->getBeginLoc()));

    // Visit condition and body
    if (const auto* Cond = S->getCond())
    {
        BaseTraverser::Visit(Cond);
        // createParentChildRelationship(S, Cond);
    }

    if (const auto* Body = S->getBody())
    {
        BaseTraverser::Visit(Body);
        // createParentChildRelationship(S, Body);
    }

    VisitStmt(S);
}

void KuzuDump::VisitReturnStmt(const clang::ReturnStmt* S)
{
    if (S == nullptr)
        return;

    // TODO: Insert return statement node
    // insertASTNode("ReturnStmt", getNodeId(S), "ReturnStmt", getSourceLocation(S->getBeginLoc()));

    // Visit return value if present
    if (const auto* RetValue = S->getRetValue())
    {
        BaseTraverser::Visit(RetValue);
        // createParentChildRelationship(S, RetValue);
    }

    VisitStmt(S);
}

// Type visitors
void KuzuDump::VisitType(const clang::Type* T)
{
    if (T == nullptr)
        return;

    // TODO: Insert type node
    std::string typeName = T->getTypeClassName();

    // insertASTNode("Type", getNodeId(T), typeName, "");
}

void KuzuDump::VisitQualType(clang::QualType T)
{
    if (T.isNull())
        return;

    // TODO: Insert qualified type node
    std::string typeStr = getTypeString(T);

    // insertASTNode("QualType", getNodeId(T.getAsOpaquePtr()), typeStr, "");

    // Visit the underlying type
    BaseTraverser::Visit(T.getTypePtr());
}

// Helper methods (placeholder implementations)
void KuzuDump::initializeDatabase()
{
    // TODO: Initialize Kuzu database
    // Database = std::make_unique<kuzu::Database>(DatabasePath);
    // Connection = std::make_unique<kuzu::Connection>(Database.get());
    // createDatabaseSchema();
}

void KuzuDump::createDatabaseSchema()
{
    // TODO: Create tables/node types and relationships for AST elements
    // This would create the graph schema for storing AST nodes and relationships
}

void KuzuDump::insertASTNode(const std::string& nodeType,
                             const std::string& nodeId,
                             const std::string& name,
                             const std::string& location)
{
    // TODO: Insert AST node into Kuzu database
    // Example: INSERT INTO ASTNode VALUES (nodeId, nodeType, name, location)
}

void KuzuDump::insertRelationship(const std::string& fromId,
                                  const std::string& toId,
                                  const std::string& relationshipType)
{
    // TODO: Insert relationship into Kuzu database
    // Example: INSERT INTO Relationship VALUES (fromId, toId, relationshipType)
}

auto KuzuDump::getNodeId(const void* ptr) -> std::string
{
    // Generate a unique ID for the AST node based on its pointer address
    std::ostringstream oss;
    oss << "node_" << std::hex << reinterpret_cast<uintptr_t>(ptr);
    return oss.str();
}

auto KuzuDump::getSourceLocation(clang::SourceLocation loc) -> std::string
{
    if (loc.isInvalid())
    {
        return "invalid";
    }

    const clang::SourceManager& SM = Context.getSourceManager();
    clang::PresumedLoc PLoc = SM.getPresumedLoc(loc);

    if (PLoc.isInvalid())
    {
        return "invalid";
    }

    std::ostringstream oss;
    oss << PLoc.getFilename() << ":" << PLoc.getLine() << ":" << PLoc.getColumn();
    return oss.str();
}

auto KuzuDump::getQualifiedName(const clang::NamedDecl* decl) -> std::string
{
    if (decl == nullptr)
        return "";

    std::string qualifiedName;
    llvm::raw_string_ostream OS(qualifiedName);
    decl->printQualifiedName(OS);
    OS.flush();
    return qualifiedName;
}

auto KuzuDump::getTypeString(clang::QualType type) -> std::string
{
    if (type.isNull())
        return "";

    return type.getAsString(Context.getPrintingPolicy());
}

void KuzuDump::createParentChildRelationship(const void* parent, const void* child)
{
    // TODO: Create parent-child relationship in database
    // insertRelationship(getNodeId(parent), getNodeId(child), "parent_of");
}

void KuzuDump::createTypeRelationship(const void* decl, const clang::Type* type)
{
    // TODO: Create type relationship in database
    // insertRelationship(getNodeId(decl), getNodeId(type), "has_type");
}

void KuzuDump::createUsageRelationship(const void* user, const void* used)
{
    // TODO: Create usage relationship in database
    // insertRelationship(getNodeId(user), getNodeId(used), "uses");
}