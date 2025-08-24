//===--- DeclarationAnalyzer.cpp - Declaration processing and analysis --===//
//
// Part of the Dosatsu project
//
//===----------------------------------------------------------------------===//

#include "DeclarationAnalyzer.h"

#include "GlobalDatabaseManager.h"
#include "KuzuDatabase.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>
#include <vector>

using namespace clang;

DeclarationAnalyzer::DeclarationAnalyzer(KuzuDatabase& database) : database(database)
{
}

void DeclarationAnalyzer::createDeclarationNode(int64_t nodeId, const clang::NamedDecl* decl)
{
    if (!database.isInitialized() || (decl == nullptr))
        return;

    // Check if Declaration node already exists for this nodeId
    auto& dbManager = GlobalDatabaseManager::getInstance();
    if (dbManager.hasDeclarationNode(nodeId))
        return;

    try
    {
        std::string name = decl->getNameAsString();
        std::string qualifiedName = extractQualifiedName(decl);
        std::string accessSpec = extractAccessSpecifier(decl);
        std::string storageClass = extractStorageClass(decl);
        std::string namespaceContext = extractNamespaceContext(decl);
        bool isDef = isDefinition(decl);

        // Create Declaration node with extracted properties
        std::string query = "CREATE (d:Declaration {node_id: " + std::to_string(nodeId) + ", name: '" + name +
                            "', qualified_name: '" + qualifiedName + "', access_specifier: '" + accessSpec +
                            "', storage_class: '" + storageClass + "', is_definition: " + (isDef ? "true" : "false") +
                            ", namespace_context: '" + namespaceContext + "'})";

        // Use batched operation for performance optimization
        database.addToBatch(query);

        // Register that this Declaration node has been created
        dbManager.registerDeclarationNode(nodeId);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Declaration node: " << e.what() << "\n";
    }
}

void DeclarationAnalyzer::createUsingDeclarationNode(int64_t nodeId, const clang::UsingDecl* decl)
{
    if (!database.isInitialized() || (decl == nullptr))
        return;

    try
    {
        std::string usingKind = "declaration";
        std::string targetName = "";
        std::string introducesName = "";
        std::string scopeImpact = "current";

        // Extract using declaration information
        if (const auto* nestedNameSpec = decl->getQualifier())
        {
            // Get the qualified name being used
            std::string qualifierStr;
            llvm::raw_string_ostream qualifierStream(qualifierStr);
            nestedNameSpec->print(qualifierStream, clang::PrintingPolicy(clang::LangOptions()));
            targetName = qualifierStr + decl->getNameAsString();
        }
        else
        {
            targetName = decl->getNameAsString();
        }

        introducesName = decl->getNameAsString();

        // Escape single quotes in strings for database storage
        std::ranges::replace(targetName, '\'', '_');
        std::ranges::replace(introducesName, '\'', '_');

        std::string query = "CREATE (u:UsingDeclaration {node_id: " + std::to_string(nodeId) + ", using_kind: '" +
                            usingKind + "', target_name: '" + targetName + "', introduces_name: '" + introducesName +
                            "', scope_impact: '" + scopeImpact + "'})";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating UsingDeclaration node: " << e.what() << "\n";
    }
}

void DeclarationAnalyzer::createUsingDirectiveNode(int64_t nodeId, const clang::UsingDirectiveDecl* decl)
{
    if (!database.isInitialized() || (decl == nullptr))
        return;

    try
    {
        std::string usingKind = "directive";
        std::string targetName = "";
        std::string introducesName = "";
        std::string scopeImpact = "current";

        // Extract using directive information
        if (const auto* nominatedNS = decl->getNominatedNamespace())
        {
            targetName = nominatedNS->getQualifiedNameAsString();
            introducesName = "*";  // Directive brings in all names from namespace
        }

        // Escape single quotes in strings for database storage
        std::ranges::replace(targetName, '\'', '_');

        std::string query = "CREATE (u:UsingDeclaration {node_id: " + std::to_string(nodeId) + ", using_kind: '" +
                            usingKind + "', target_name: '" + targetName + "', introduces_name: '" + introducesName +
                            "', scope_impact: '" + scopeImpact + "'})";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating UsingDirective node: " << e.what() << "\n";
    }
}

void DeclarationAnalyzer::createNamespaceAliasNode(int64_t nodeId, const clang::NamespaceAliasDecl* decl)
{
    if (!database.isInitialized() || (decl == nullptr))
        return;

    try
    {
        std::string usingKind = "namespace_alias";
        std::string targetName = "";
        std::string introducesName = decl->getNameAsString();
        std::string scopeImpact = "current";

        // Extract namespace alias information
        if (const auto* aliasedNS = decl->getNamespace())
            targetName = aliasedNS->getQualifiedNameAsString();

        // Escape single quotes in strings for database storage
        std::ranges::replace(targetName, '\'', '_');
        std::ranges::replace(introducesName, '\'', '_');

        std::string query = "CREATE (u:UsingDeclaration {node_id: " + std::to_string(nodeId) + ", using_kind: '" +
                            usingKind + "', target_name: '" + targetName + "', introduces_name: '" + introducesName +
                            "', scope_impact: '" + scopeImpact + "'})";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating NamespaceAlias node: " << e.what() << "\n";
    }
}

void DeclarationAnalyzer::createReferenceRelation(int64_t fromId, int64_t toId, const std::string& kind)
{
    if (!database.isInitialized() || fromId == -1 || toId == -1)
        return;

    try
    {
        std::string query = "MATCH (from:ASTNode {node_id: " + std::to_string(fromId) + "}), " +
                            "(to:Declaration {node_id: " + std::to_string(toId) + "}) " +
                            "CREATE (from)-[:REFERENCES {reference_kind: '" + kind + "', is_direct: true}]->(to)";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating REFERENCES relationship: " << e.what() << "\n";
    }
}

auto DeclarationAnalyzer::extractQualifiedName(const clang::NamedDecl* decl) -> std::string
{
    if (decl == nullptr)
        return "";

    std::string qualifiedName = decl->getQualifiedNameAsString();
    // Replace any problematic characters for database storage
    std::ranges::replace(qualifiedName, '\'', '_');
    return qualifiedName;
}

auto DeclarationAnalyzer::extractAccessSpecifier(const clang::Decl* decl) -> std::string
{
    if (decl == nullptr)
        return "none";

    switch (decl->getAccess())
    {
    case AS_public:
        return "public";
    case AS_protected:
        return "protected";
    case AS_private:
        return "private";
    case AS_none:
    default:
        return "none";
    }
}

auto DeclarationAnalyzer::extractStorageClass(const clang::Decl* decl) -> std::string
{
    if (const auto* varDecl = dyn_cast<VarDecl>(decl))
    {
        switch (varDecl->getStorageClass())
        {
        case SC_None:
            return "none";
        case SC_Static:
            return "static";
        case SC_Extern:
            return "extern";
        case SC_Auto:
            return "auto";
        case SC_Register:
            return "register";
        default:
            return "unknown";
        }
    }
    else if (const auto* funcDecl = dyn_cast<FunctionDecl>(decl))
    {
        switch (funcDecl->getStorageClass())
        {
        case SC_None:
            return "none";
        case SC_Static:
            return "static";
        case SC_Extern:
            return "extern";
        default:
            return "unknown";
        }
    }

    return "none";
}

auto DeclarationAnalyzer::extractNamespaceContext(const clang::Decl* decl) -> std::string
{
    if (decl == nullptr)
        return "";

    const DeclContext* context = decl->getDeclContext();
    std::vector<std::string> namespaces;

    while ((context != nullptr) && !context->isTranslationUnit())
    {
        if (const auto* nsDecl = dyn_cast<NamespaceDecl>(context))
        {
            if (!nsDecl->isAnonymousNamespace())
                namespaces.push_back(nsDecl->getNameAsString());
        }
        else if (const auto* recordDecl = dyn_cast<RecordDecl>(context))
        {
            if (recordDecl->getIdentifier() != nullptr)
                namespaces.push_back(recordDecl->getNameAsString());
        }
        context = context->getParent();
    }

    std::ranges::reverse(namespaces);

    std::string result;
    for (size_t i = 0; i < namespaces.size(); ++i)
    {
        if (i > 0)
            result += "::";
        result += namespaces[i];
    }

    return result;
}

auto DeclarationAnalyzer::isDefinition(const clang::Decl* decl) -> bool
{
    if (decl == nullptr)
        return false;

    if (const auto* funcDecl = dyn_cast<FunctionDecl>(decl))
        return funcDecl->isThisDeclarationADefinition();
    if (const auto* varDecl = dyn_cast<VarDecl>(decl))
        return varDecl->isThisDeclarationADefinition() != 0;
    if (const auto* tagDecl = dyn_cast<TagDecl>(decl))
        return tagDecl->isThisDeclarationADefinition();

    return false;
}
