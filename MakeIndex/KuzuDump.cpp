//===--- KuzuDump.cpp - Dumping implementation for ASTs ------------------===//
//
// Based on LLVM Project's ASTDumper, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the AST dump methods, which dump out the
// AST in a form that exposes type details and other fields.
//
//===----------------------------------------------------------------------===//

// clang-format off
#include "NoWarningScope_Enter.h"
#include "KuzuDump.h"
#include "clang/AST/ASTConcept.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclLookups.h"
#include "clang/AST/JSONNodeDumper.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"

#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <stdexcept>

#include "kuzu.hpp"

using namespace clang;
using namespace clang::comments;

void KuzuDump::dumpInvalidDeclContext(const DeclContext* DC)
{
    NodeDumper.AddChild(
        [=]
        {
            if (!DC)
            {
                ColorScope Color(OS, ShowColors, NullColor);
                OS << "<<<NULL>>>";
                return;
            }
            // An invalid DeclContext is one for which a dyn_cast() from a DeclContext
            // pointer to a Decl pointer would fail an assertion or otherwise fall prey
            // to undefined behavior as a result of an invalid associated DeclKind.
            // Such invalidity is not supposed to happen of course, but, when it does,
            // the information provided below is intended to provide some hints about
            // what might have gone awry.
            {
                ColorScope Color(OS, ShowColors, DeclKindNameColor);
                OS << "DeclContext";
            }
            NodeDumper.dumpPointer(DC);
            OS << " <";
            {
                ColorScope Color(OS, ShowColors, DeclNameColor);
                OS << "unrecognized Decl kind " << (unsigned)DC->getDeclKind();
            }
            OS << ">";
        });
}

void KuzuDump::dumpLookups(const DeclContext* DC, bool DumpDecls)
{
    NodeDumper.AddChild(
        [=]
        {
            OS << "StoredDeclsMap ";
            NodeDumper.dumpBareDeclRef(cast<Decl>(DC));

            const DeclContext* Primary = DC->getPrimaryContext();
            if (Primary != DC)
            {
                OS << " primary";
                NodeDumper.dumpPointer(cast<Decl>(Primary));
            }

            bool HasUndeserializedLookups = Primary->hasExternalVisibleStorage();

            auto Range =
                getDeserialize() ? Primary->lookups() : Primary->noload_lookups(/*PreserveInternalState=*/true);
            for (auto I = Range.begin(), E = Range.end(); I != E; ++I)
            {
                DeclarationName Name = I.getLookupName();
                DeclContextLookupResult R = *I;

                NodeDumper.AddChild(
                    [=]
                    {
                        OS << "DeclarationName ";
                        {
                            ColorScope Color(OS, ShowColors, DeclNameColor);
                            OS << '\'' << Name << '\'';
                        }

                        for (auto lookupResult : R)
                        {
                            NodeDumper.AddChild(
                                [=]
                                {
                                    NodeDumper.dumpBareDeclRef(lookupResult);

                                    if (!lookupResult->isUnconditionallyVisible())
                                        OS << " hidden";

                                    // If requested, dump the redecl chain for this lookup.
                                    if (DumpDecls)
                                    {
                                        // Dump earliest decl first.
                                        std::function<void(Decl*)> DumpWithPrev = [&](Decl* D)
                                        {
                                            if (Decl* Prev = D->getPreviousDecl())
                                                DumpWithPrev(Prev);
                                            Visit(D);
                                        };
                                        DumpWithPrev(lookupResult);
                                    }
                                });
                        }
                    });
            }

            if (HasUndeserializedLookups)
            {
                NodeDumper.AddChild(
                    [=]
                    {
                        ColorScope Color(OS, ShowColors, UndeserializedColor);
                        OS << "<undeserialized lookups>";
                    });
            }
        });
}

template <typename SpecializationDecl>
void KuzuDump::dumpTemplateDeclSpecialization(const SpecializationDecl* D, bool DumpExplicitInst, bool DumpRefOnly)
{
    bool DumpedAny = false;
    for (const auto* RedeclWithBadType : D->redecls())
    {
        // FIXME: The redecls() range sometimes has elements of a less-specific
        // type. (In particular, ClassTemplateSpecializationDecl::redecls() gives
        // us TagDecls, and should give CXXRecordDecls).
        auto* Redecl = cast<SpecializationDecl>(RedeclWithBadType);
        switch (Redecl->getTemplateSpecializationKind())
        {
        case TSK_ExplicitInstantiationDeclaration:
        case TSK_ExplicitInstantiationDefinition:
            if (!DumpExplicitInst)
                break;
            [[fallthrough]];
        case TSK_Undeclared:
        case TSK_ImplicitInstantiation:
            if (DumpRefOnly)
                NodeDumper.dumpDeclRef(Redecl);
            else
                Visit(Redecl);
            DumpedAny = true;
            break;
        case TSK_ExplicitSpecialization:
            break;
        }
    }

    // Ensure we dump at least one decl for each specialization.
    if (!DumpedAny)
        NodeDumper.dumpDeclRef(D);
}

template <typename TemplateDecl>
void KuzuDump::dumpTemplateDecl(const TemplateDecl* D, bool DumpExplicitInst)
{
    dumpTemplateParameters(D->getTemplateParameters());

    Visit(D->getTemplatedDecl());

    if (GetTraversalKind() == TK_AsIs)
    {
        for (const auto* Child : D->specializations())
            dumpTemplateDeclSpecialization(Child, DumpExplicitInst, !D->isCanonicalDecl());
    }
}

void KuzuDump::VisitFunctionTemplateDecl(const FunctionTemplateDecl* D)
{
    // FIXME: We don't add a declaration of a function template specialization
    // to its context when it's explicitly instantiated, so dump explicit
    // instantiations when we dump the template itself.
    dumpTemplateDecl(D, true);
}

void KuzuDump::VisitClassTemplateDecl(const ClassTemplateDecl* D)
{
    dumpTemplateDecl(D, false);
}

void KuzuDump::VisitVarTemplateDecl(const VarTemplateDecl* D)
{
    dumpTemplateDecl(D, false);
}

void KuzuDump::initializeDatabase()
{
    if (!databaseEnabled || databasePath.empty())
    {
        return;
    }

    try
    {
        // Create database directory if it doesn't exist
        std::filesystem::path dbPath(databasePath);
        if (dbPath.has_parent_path())
        {
            std::filesystem::create_directories(dbPath.parent_path());
        }

        // Initialize Kuzu database
        database = std::make_unique<kuzu::main::Database>(databasePath);
        connection = std::make_unique<kuzu::main::Connection>(database.get());

        // Create schema
        createSchema();

        llvm::outs() << "Database initialized at: " << databasePath << "\n";
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to initialize Kuzu database: " + std::string(e.what()));
    }
}

void KuzuDump::executeSchemaQuery(const std::string& query, const std::string& schemaName)
{
    auto result = connection->query(query);
    if (!result->isSuccess())
    {
        throw std::runtime_error("Failed to create " + schemaName + " table: " + result->getErrorMessage());
    }
}

void KuzuDump::createSchema()
{
    if (!connection)
    {
        return;
    }

    try
    {
        // Create base ASTNode table
        executeSchemaQuery("CREATE NODE TABLE ASTNode("
                           "node_id INT64 PRIMARY KEY, "
                           "node_type STRING, "
                           "memory_address STRING, "
                           "source_file STRING, "
                           "start_line INT64, "
                           "start_column INT64, "
                           "end_line INT64, "
                           "end_column INT64, "
                           "is_implicit BOOLEAN, "
                           "raw_text STRING)",
                           "ASTNode");

        // Create Declaration table
        executeSchemaQuery("CREATE NODE TABLE Declaration("
                           "node_id INT64 PRIMARY KEY, "
                           "name STRING, "
                           "qualified_name STRING, "
                           "access_specifier STRING, "
                           "storage_class STRING, "
                           "is_definition BOOLEAN, "
                           "namespace_context STRING)",
                           "Declaration");

        // Create Type table
        executeSchemaQuery("CREATE NODE TABLE Type("
                           "node_id INT64 PRIMARY KEY, "
                           "type_name STRING, "
                           "canonical_type STRING, "
                           "size_bytes INT64, "
                           "is_const BOOLEAN, "
                           "is_volatile BOOLEAN, "
                           "is_builtin BOOLEAN)",
                           "Type");

        // Create Statement table
        executeSchemaQuery("CREATE NODE TABLE Statement("
                           "node_id INT64 PRIMARY KEY, "
                           "statement_kind STRING, "
                           "has_side_effects BOOLEAN)",
                           "Statement");

        // Create Expression table
        executeSchemaQuery("CREATE NODE TABLE Expression("
                           "node_id INT64 PRIMARY KEY, "
                           "expression_kind STRING, "
                           "value_category STRING, "
                           "literal_value STRING, "
                           "operator_kind STRING)",
                           "Expression");

        // Create Attribute table
        executeSchemaQuery("CREATE NODE TABLE Attribute("
                           "node_id INT64 PRIMARY KEY, "
                           "attribute_kind STRING, "
                           "attribute_value STRING)",
                           "Attribute");

        // Create relationship tables
        executeSchemaQuery("CREATE REL TABLE PARENT_OF("
                           "FROM ASTNode TO ASTNode, "
                           "child_index INT64, "
                           "relationship_kind STRING)",
                           "PARENT_OF");

        executeSchemaQuery("CREATE REL TABLE HAS_TYPE("
                           "FROM Declaration TO Type, "
                           "type_role STRING)",
                           "HAS_TYPE");

        executeSchemaQuery("CREATE REL TABLE REFERENCES("
                           "FROM ASTNode TO Declaration, "
                           "reference_kind STRING, "
                           "is_direct BOOLEAN)",
                           "REFERENCES");

        executeSchemaQuery("CREATE REL TABLE IN_SCOPE("
                           "FROM ASTNode TO Declaration, "
                           "scope_kind STRING)",
                           "IN_SCOPE");

        executeSchemaQuery("CREATE REL TABLE TEMPLATE_RELATION("
                           "FROM Declaration TO Declaration, "
                           "relation_type STRING)",
                           "TEMPLATE_RELATION");

        llvm::outs() << "Database schema created successfully\n";
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to create database schema: " + std::string(e.what()));
    }
}

// Basic node creation methods (Phase 1 - minimal implementation)
auto KuzuDump::createASTNode(const clang::Decl* decl) -> int64_t
{
    if (!connection || (decl == nullptr))
    {
        return -1;
    }

    // Check if already processed
    auto it = nodeIdMap.find(decl);
    if (it != nodeIdMap.end())
    {
        return it->second;
    }

    int64_t nodeId = nextNodeId++;
    nodeIdMap[decl] = nodeId;

    try
    {
        // Extract basic information
        std::string nodeType = extractNodeType(decl);

        // Format memory address as hex string
        std::stringstream addrStream;
        addrStream << std::hex << (uintptr_t)decl;
        std::string memoryAddr = addrStream.str();

        std::string sourceFile = extractSourceLocation(decl->getLocation());

        bool isImplicit = isImplicitNode(decl);

        // Create base AST node query using string concatenation
        std::string query = "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
                            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
                            "', is_implicit: " + (isImplicit ? "true" : "false") +
                            ", start_line: -1, start_column: -1, end_line: -1, end_column: -1, raw_text: ''})";

        auto result = connection->query(query);

        if (!result->isSuccess())
        {
            llvm::errs() << "Failed to create AST node: " << result->getErrorMessage() << "\n";
            return -1;
        }

        // Create specialized Declaration node if this is a named declaration
        if (const auto* namedDecl = dyn_cast<NamedDecl>(decl))
        {
            createDeclarationNode(nodeId, namedDecl);

            // Create type relationships for typed declarations
            if (const auto* valueDecl = dyn_cast<ValueDecl>(namedDecl))
            {
                createTypeNodeAndRelation(nodeId, valueDecl->getType());
            }
            else if (const auto* functionDecl = dyn_cast<FunctionDecl>(namedDecl))
            {
                createTypeNodeAndRelation(nodeId, functionDecl->getType());
            }
        }

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node: " << e.what() << "\n";
        return -1;
    }
}

auto KuzuDump::createASTNode(const clang::Stmt* stmt) -> int64_t
{
    if (!connection || (stmt == nullptr))
    {
        return -1;
    }

    // Check if already processed
    auto it = nodeIdMap.find(stmt);
    if (it != nodeIdMap.end())
    {
        return it->second;
    }

    int64_t nodeId = nextNodeId++;
    nodeIdMap[stmt] = nodeId;

    try
    {
        // Extract basic information
        std::string nodeType = extractNodeType(stmt);
        // Format memory address as hex string
        std::stringstream addrStream;
        addrStream << std::hex << (uintptr_t)stmt;
        std::string memoryAddr = addrStream.str();

        std::string sourceFile = extractSourceLocation(stmt->getBeginLoc());

        // Create base AST node query using string concatenation
        std::string query =
            "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
            "', is_implicit: false, start_line: -1, start_column: -1, end_line: -1, end_column: -1, raw_text: ''})";

        auto result = connection->query(query);
        if (!result->isSuccess())
        {
            llvm::errs() << "Failed to create AST node for statement: " << result->getErrorMessage() << "\n";
            return -1;
        }

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node for statement: " << e.what() << "\n";
        return -1;
    }
}

auto KuzuDump::createASTNode(const clang::Type* type) -> int64_t
{
    if (!connection || (type == nullptr))
    {
        return -1;
    }

    // Check if already processed
    auto it = nodeIdMap.find(type);
    if (it != nodeIdMap.end())
    {
        return it->second;
    }

    int64_t nodeId = nextNodeId++;
    nodeIdMap[type] = nodeId;

    try
    {
        // Extract basic information
        std::string nodeType = extractNodeType(type);
        // Format memory address as hex string
        std::stringstream addrStream;
        addrStream << std::hex << (uintptr_t)type;
        std::string memoryAddr = addrStream.str();

        // Create base AST node query using string concatenation
        std::string query = "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
                            "', memory_address: '" + memoryAddr +
                            "', source_file: '', is_implicit: false, start_line: -1, start_column: -1, end_line: -1, "
                            "end_column: -1, raw_text: ''})";

        auto result = connection->query(query);
        if (!result->isSuccess())
        {
            llvm::errs() << "Failed to create AST node for type: " << result->getErrorMessage() << "\n";
            return -1;
        }

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node for type: " << e.what() << "\n";
        return -1;
    }
}

// Placeholder relationship creation methods (Phase 1)
void KuzuDump::createParentChildRelation(int64_t /*parentId*/, int64_t /*childId*/, int /*index*/)
{
    // TODO: Implement in Phase 2
}

void KuzuDump::createTypeRelation(int64_t /*declId*/, int64_t /*typeId*/)
{
    // TODO: Implement in Phase 2
}

void KuzuDump::createReferenceRelation(int64_t /*fromId*/, int64_t /*toId*/, const std::string& /*kind*/)
{
    // TODO: Implement in Phase 3
}

// Type processing methods (Phase 2)
auto KuzuDump::createTypeNodeAndRelation(int64_t declNodeId, clang::QualType qualType) -> int64_t
{
    if (!connection || qualType.isNull())
    {
        return -1;
    }

    int64_t typeNodeId = createTypeNode(qualType);
    if (typeNodeId != -1)
    {
        createTypeRelation(declNodeId, typeNodeId);
    }

    return typeNodeId;
}

auto KuzuDump::createTypeNode(clang::QualType qualType) -> int64_t
{
    if (!connection || qualType.isNull())
    {
        return -1;
    }

    try
    {
        int64_t typeNodeId = nextNodeId++;

        std::string typeName = extractTypeName(qualType);
        std::string typeCategory = extractTypeCategory(qualType);
        std::string qualifiers = extractTypeQualifiers(qualType);
        bool isBuiltIn = isBuiltInType(qualType);

        std::string query = "CREATE (t:Type {id: " + std::to_string(typeNodeId) + ", name: '" + typeName +
                            "', category: '" + typeCategory + "', qualifiers: '" + qualifiers +
                            "', is_builtin: " + (isBuiltIn ? "true" : "false") + ", source_location: '" +
                            extractTypeSourceLocation(qualType) + "'})";

        auto result = connection->query(query);
        if (!result->isSuccess())
        {
            llvm::errs() << "Failed to create Type node: " << result->getErrorMessage() << "\n";
            return -1;
        }

        return typeNodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Type node: " << e.what() << "\n";
        return -1;
    }
}

// Enhanced declaration processing methods (Phase 2)
void KuzuDump::createDeclarationNode(int64_t nodeId, const clang::NamedDecl* decl)
{
    if (!connection || !decl)
    {
        return;
    }

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

        auto result = connection->query(query);
        if (!result->isSuccess())
        {
            llvm::errs() << "Failed to create Declaration node: " << result->getErrorMessage() << "\n";
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Declaration node: " << e.what() << "\n";
    }
}

auto KuzuDump::extractQualifiedName(const clang::NamedDecl* decl) -> std::string
{
    if (!decl)
    {
        return "";
    }

    std::string qualifiedName = decl->getQualifiedNameAsString();
    // Replace any problematic characters for database storage
    std::replace(qualifiedName.begin(), qualifiedName.end(), '\'', '_');
    return qualifiedName;
}

auto KuzuDump::extractAccessSpecifier(const clang::Decl* decl) -> std::string
{
    if (!decl)
    {
        return "none";
    }

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

auto KuzuDump::extractStorageClass(const clang::Decl* decl) -> std::string
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

auto KuzuDump::extractNamespaceContext(const clang::Decl* decl) -> std::string
{
    if (!decl)
    {
        return "";
    }

    const DeclContext* context = decl->getDeclContext();
    std::vector<std::string> namespaces;

    while (context && !context->isTranslationUnit())
    {
        if (const auto* nsDecl = dyn_cast<NamespaceDecl>(context))
        {
            if (!nsDecl->isAnonymousNamespace())
            {
                namespaces.push_back(nsDecl->getNameAsString());
            }
        }
        else if (const auto* recordDecl = dyn_cast<RecordDecl>(context))
        {
            if (recordDecl->getIdentifier())
            {
                namespaces.push_back(recordDecl->getNameAsString());
            }
        }
        context = context->getParent();
    }

    std::reverse(namespaces.begin(), namespaces.end());

    std::string result;
    for (size_t i = 0; i < namespaces.size(); ++i)
    {
        if (i > 0)
            result += "::";
        result += namespaces[i];
    }

    return result;
}

auto KuzuDump::isDefinition(const clang::Decl* decl) -> bool
{
    if (!decl)
    {
        return false;
    }

    if (const auto* funcDecl = dyn_cast<FunctionDecl>(decl))
    {
        return funcDecl->isThisDeclarationADefinition();
    }
    else if (const auto* varDecl = dyn_cast<VarDecl>(decl))
    {
        return varDecl->isThisDeclarationADefinition();
    }
    else if (const auto* recordDecl = dyn_cast<RecordDecl>(decl))
    {
        return recordDecl->isThisDeclarationADefinition();
    }

    return false;
}

// Data extraction utility methods (enhanced for Phase 2)
auto KuzuDump::extractSourceLocation(const clang::SourceLocation& loc) -> std::string
{
    if (loc.isInvalid())
    {
        return "<invalid>";
    }

    // Get detailed location information
    auto [fileName, line, column] = extractSourceLocationDetailed(loc);

    // Return formatted location string
    if (fileName == "<invalid>" || line == -1 || column == -1)
    {
        return "<unknown_location>";
    }

    return fileName + ":" + std::to_string(line) + ":" + std::to_string(column);
}

auto KuzuDump::extractSourceLocationDetailed(const clang::SourceLocation& loc)
    -> std::tuple<std::string, int64_t, int64_t>
{
    if (loc.isInvalid())
    {
        return std::make_tuple("<invalid>", -1, -1);
    }

    try
    {
        // TODO: Access SourceManager through proper context
        // For now, return simplified location information
        // Will be enhanced once we have proper ASTContext access

        // TODO: Implement actual source location extraction once we have SourceManager access
        // This functionality will be implemented in a future enhancement
    }
    catch (...)
    {
        // If any exception occurs, return invalid location
        return std::make_tuple("<exception>", -1, -1);
    }

    return std::make_tuple("<no_location>", -1, -1);
}

auto KuzuDump::extractNodeType(const clang::Decl* decl) -> std::string
{
    if (decl == nullptr)
    {
        return "UnknownDecl";
    }
    return decl->getDeclKindName();
}

auto KuzuDump::extractNodeType(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
    {
        return "UnknownStmt";
    }
    return stmt->getStmtClassName();
}

auto KuzuDump::extractNodeType(const clang::Type* type) -> std::string
{
    if (type == nullptr)
    {
        return "UnknownType";
    }
    return type->getTypeClassName();
}

auto KuzuDump::isImplicitNode(const clang::Decl* decl) -> bool
{
    if (decl == nullptr)
    {
        return false;
    }
    return decl->isImplicit();
}

// Type extraction utility methods (Phase 2)
auto KuzuDump::extractTypeName(clang::QualType qualType) -> std::string
{
    if (qualType.isNull())
    {
        return "<invalid_type>";
    }

    // Get the unqualified type name
    return qualType.getUnqualifiedType().getAsString();
}

auto KuzuDump::extractTypeCategory(clang::QualType qualType) -> std::string
{
    if (qualType.isNull())
    {
        return "invalid";
    }

    const clang::Type* type = qualType.getTypePtr();
    if (!type)
    {
        return "invalid";
    }

    if (type->isBuiltinType())
    {
        return "builtin";
    }
    else if (type->isPointerType())
    {
        return "pointer";
    }
    else if (type->isReferenceType())
    {
        return "reference";
    }
    else if (type->isArrayType())
    {
        return "array";
    }
    else if (type->isFunctionType())
    {
        return "function";
    }
    else if (type->isRecordType())
    {
        return "record";
    }
    else if (type->isEnumeralType())
    {
        return "enum";
    }
    else if (type->isTemplateTypeParmType())
    {
        return "template_param";
    }
    else
    {
        return "user_defined";
    }
}

auto KuzuDump::extractTypeQualifiers(clang::QualType qualType) -> std::string
{
    if (qualType.isNull())
    {
        return "";
    }

    std::string qualifiers;

    if (qualType.isConstQualified())
    {
        qualifiers += "const ";
    }
    if (qualType.isVolatileQualified())
    {
        qualifiers += "volatile ";
    }
    if (qualType.isRestrictQualified())
    {
        qualifiers += "restrict ";
    }

    // Remove trailing space
    if (!qualifiers.empty() && qualifiers.back() == ' ')
    {
        qualifiers.pop_back();
    }

    return qualifiers;
}

auto KuzuDump::isBuiltInType(clang::QualType qualType) -> bool
{
    if (qualType.isNull())
    {
        return false;
    }

    const clang::Type* type = qualType.getTypePtr();
    return type && type->isBuiltinType();
}

auto KuzuDump::extractTypeSourceLocation(clang::QualType qualType) -> std::string
{
    // For now, types don't have specific source locations
    // This could be enhanced in the future to show where types are defined
    return "<type_location>";
}

// Hierarchy processing methods (Phase 2)
void KuzuDump::pushParent(int64_t parentNodeId)
{
    parentStack.push_back(parentNodeId);
    childIndex = 0;  // Reset child index for new parent
}

void KuzuDump::popParent()
{
    if (!parentStack.empty())
    {
        parentStack.pop_back();
        childIndex = 0;  // Reset child index when popping
    }
}

void KuzuDump::createHierarchyRelationship(int64_t childNodeId)
{
    if (parentStack.empty() || childNodeId == -1)
    {
        return;
    }

    int64_t parentNodeId = getCurrentParent();
    if (parentNodeId != -1)
    {
        createParentChildRelation(parentNodeId, childNodeId, childIndex++);
    }
}

auto KuzuDump::getCurrentParent() -> int64_t
{
    if (parentStack.empty())
    {
        return -1;
    }
    return parentStack.back();
}

// Core Visit method implementations
void KuzuDump::VisitDecl(const Decl* D)
{
    if (D == nullptr)
    {
        return;
    }

    // Create database node for this declaration
    int64_t nodeId = createASTNode(D);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitFunctionDecl(const FunctionDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this function declaration
    [[maybe_unused]] int64_t nodeId = createASTNode(D);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitVarDecl(const VarDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this variable declaration
    [[maybe_unused]] int64_t nodeId = createASTNode(D);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitParmVarDecl(const ParmVarDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this parameter declaration
    [[maybe_unused]] int64_t nodeId = createASTNode(D);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitStmt(const Stmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this statement
    int64_t nodeId = createASTNode(S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitCompoundStmt(const CompoundStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this compound statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitDeclStmt(const DeclStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this declaration statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitReturnStmt(const ReturnStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this return statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitIfStmt(const IfStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this if statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitWhileStmt(const WhileStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this while statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitForStmt(const ForStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this for statement
    [[maybe_unused]] int64_t nodeId = createASTNode(S);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitExpr(const Expr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this expression
    int64_t nodeId = createASTNode(E);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitDeclRefExpr(const DeclRefExpr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this declaration reference expression
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitIntegerLiteral(const IntegerLiteral* E)
{
    if (E == nullptr)
        return;

    // Create database node for this integer literal
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitBinaryOperator(const BinaryOperator* E)
{
    if (E == nullptr)
        return;

    // Create database node for this binary operator
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitUnaryOperator(const UnaryOperator* E)
{
    if (E == nullptr)
        return;

    // Create database node for this unary operator
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitCallExpr(const CallExpr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this call expression
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitImplicitCastExpr(const ImplicitCastExpr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this implicit cast expression
    [[maybe_unused]] int64_t nodeId = createASTNode(E);

    // The ASTNodeTraverser will handle automatic traversal
}
