//===--- DeclarationAnalyzer.h - Declaration processing and analysis ----===//
//
// Part of the Dosatsu project
//
//===----------------------------------------------------------------------===//

#pragma once

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <cstdint>
#include <string>

namespace clang
{

class KuzuDatabase;

/// Handles declaration analysis and processing for AST storage
class DeclarationAnalyzer
{
public:
    /// Constructor
    /// \param database Database instance for storage
    explicit DeclarationAnalyzer(KuzuDatabase& database);

    /// Create a declaration node
    /// \param nodeId Node ID for the declaration
    /// \param decl The named declaration to process
    void createDeclarationNode(int64_t nodeId, const clang::NamedDecl* decl);

    /// Create using declaration node
    /// \param nodeId Node ID for the declaration
    /// \param decl The using declaration to process
    void createUsingDeclarationNode(int64_t nodeId, const clang::UsingDecl* decl);

    /// Create using directive node
    /// \param nodeId Node ID for the declaration
    /// \param decl The using directive declaration to process
    void createUsingDirectiveNode(int64_t nodeId, const clang::UsingDirectiveDecl* decl);

    /// Create namespace alias node
    /// \param nodeId Node ID for the declaration
    /// \param decl The namespace alias declaration to process
    void createNamespaceAliasNode(int64_t nodeId, const clang::NamespaceAliasDecl* decl);

    /// Create reference relationship
    /// \param fromId Source node ID
    /// \param toId Target node ID
    /// \param kind Kind of reference relationship
    void createReferenceRelation(int64_t fromId, int64_t toId, const std::string& kind);

    /// Extract qualified name from declaration
    /// \param decl The named declaration
    /// \return Qualified name string
    auto extractQualifiedName(const clang::NamedDecl* decl) -> std::string;

    /// Extract access specifier from declaration
    /// \param decl The declaration
    /// \return Access specifier string
    auto extractAccessSpecifier(const clang::Decl* decl) -> std::string;

    /// Extract storage class from declaration
    /// \param decl The declaration
    /// \return Storage class string
    auto extractStorageClass(const clang::Decl* decl) -> std::string;

    /// Extract namespace context from declaration
    /// \param decl The declaration
    /// \return Namespace context string
    auto extractNamespaceContext(const clang::Decl* decl) -> std::string;

    /// Check if declaration is a definition
    /// \param decl The declaration
    /// \return True if declaration is a definition
    auto isDefinition(const clang::Decl* decl) -> bool;

private:
    KuzuDatabase& database;
};

}  // namespace clang
