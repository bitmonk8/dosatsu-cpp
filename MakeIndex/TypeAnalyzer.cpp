//===--- TypeAnalyzer.cpp - Type processing and analysis ----------------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#include "TypeAnalyzer.h"

#include "ASTNodeProcessor.h"
#include "GlobalDatabaseManager.h"
#include "KuzuDatabase.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/Type.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>

using namespace clang;

TypeAnalyzer::TypeAnalyzer(KuzuDatabase& database, ASTNodeProcessor& nodeProcessor, const ASTContext& astContext)
    : database(database), nodeProcessor(nodeProcessor)
{
    (void)astContext;  // Unused parameter - kept for future expansion
}

auto TypeAnalyzer::createTypeNodeAndRelation(int64_t declNodeId, clang::QualType qualType) -> int64_t
{
    if (!database.isInitialized() || qualType.isNull())
        return -1;

    int64_t typeNodeId = createTypeNode(qualType);
    if (typeNodeId != -1)
        createTypeRelation(declNodeId, typeNodeId);

    return typeNodeId;
}

auto TypeAnalyzer::createTypeNode(clang::QualType qualType) -> int64_t
{
    if (!database.isInitialized() || qualType.isNull())
        return -1;

    try
    {
        // Create node using node processor first
        const Type* typePtr = qualType.getTypePtr();
        if (typePtr == nullptr)
            return -1;
        int64_t typeNodeId = nodeProcessor.createASTNode(typePtr);
        if (typeNodeId == -1)
            return -1;

        // Check if Type node already exists for this nodeId
        auto& dbManager = GlobalDatabaseManager::getInstance();
        if (dbManager.hasTypeNode(typeNodeId))
            return typeNodeId;

        std::string typeName = extractTypeName(qualType);
        std::string typeCategory = extractTypeCategory(qualType);
        std::string qualifiers = extractTypeQualifiers(qualType);
        bool isBuiltIn = isBuiltInType(qualType);

        std::string query = "CREATE (t:Type {node_id: " + std::to_string(typeNodeId) + ", type_name: '" + typeName +
                            "', canonical_type: '" + typeCategory +
                            "', size_bytes: -1, is_const: " + (qualType.isConstQualified() ? "true" : "false") +
                            ", is_volatile: " + (qualType.isVolatileQualified() ? "true" : "false") +
                            ", is_builtin: " + (isBuiltIn ? "true" : "false") + "})";

        // Use batched operation for performance optimization
        database.addToBatch(query);

        // Register that this Type node has been created
        dbManager.registerTypeNode(typeNodeId);

        return typeNodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Type node: " << e.what() << "\n";
        return -1;
    }
}

void TypeAnalyzer::createTypeRelation(int64_t declId, int64_t typeId)
{
    if (!database.isInitialized() || declId == -1 || typeId == -1)
        return;

    try
    {
        std::string query = "MATCH (d:Declaration {node_id: " + std::to_string(declId) + "}), " +
                            "(t:Type {node_id: " + std::to_string(typeId) + "}) " +
                            "CREATE (d)-[:HAS_TYPE {type_role: 'primary'}]->(t)";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating HAS_TYPE relationship: " << e.what() << "\n";
    }
}

auto TypeAnalyzer::extractTypeName(clang::QualType qualType) -> std::string
{
    if (qualType.isNull())
        return "unknown";

    std::string typeName = qualType.getAsString();
    // Replace any problematic characters for database storage
    std::ranges::replace(typeName, '\'', '_');
    return typeName;
}

auto TypeAnalyzer::extractTypeCategory(clang::QualType qualType) -> std::string
{
    if (qualType.isNull())
        return "unknown";

    const Type* type = qualType.getTypePtr();
    if (type == nullptr)
        return "unknown";

    if (type->isBuiltinType())
        return "builtin";
    if (type->isPointerType())
        return "pointer";
    if (type->isReferenceType())
        return "reference";
    if (type->isArrayType())
        return "array";
    if (type->isFunctionType())
        return "function";
    if (type->isRecordType())
        return "record";
    if (type->isEnumeralType())
        return "enum";
    if (type->isTypedefNameType())
        return "typedef";
    if (type->isTemplateTypeParmType())
        return "template_parameter";
    if (type->isDependentType())
        return "dependent";
    return "other";
}

auto TypeAnalyzer::extractTypeQualifiers(clang::QualType qualType) -> std::string
{
    if (qualType.isNull())
        return "";

    std::string qualifiers;
    if (!qualType.isNull()) {
        if (qualType.isConstQualified())
            qualifiers += "const ";
        if (qualType.isVolatileQualified())
            qualifiers += "volatile ";
        if (qualType.isRestrictQualified())
            qualifiers += "restrict ";
    }

    // Remove trailing space
    if (!qualifiers.empty() && qualifiers.back() == ' ')
        qualifiers.pop_back();

    return qualifiers;
}

auto TypeAnalyzer::isBuiltInType(clang::QualType qualType) -> bool
{
    if (qualType.isNull())
        return false;

    const Type* type = qualType.getTypePtr();
    return (type != nullptr) && type->isBuiltinType();
}

auto TypeAnalyzer::extractTypeSourceLocation(clang::QualType /*qualType*/) -> std::string
{
    // For now, return empty string as types don't have specific source locations
    // This could be enhanced to track typedef declarations or template instantiations
    return "";
}
