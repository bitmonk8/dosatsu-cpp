//===--- TypeAnalyzer.h - Type processing and analysis ------------------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#pragma once

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/Type.h"
#include "clang/AST/ASTContext.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <cstdint>
#include <string>

namespace clang
{

class KuzuDatabase;
class ASTNodeProcessor;

/// Handles type analysis and processing for AST storage
class TypeAnalyzer
{
public:
    /// Constructor
    /// \param database Database instance for storage
    /// \param nodeProcessor Node processor for creating basic nodes
    /// \param astContext AST context for type information
    TypeAnalyzer(KuzuDatabase& database, ASTNodeProcessor& nodeProcessor, ASTContext& astContext);

    /// Create type node and relationship for a declaration
    /// \param declNodeId Node ID of the declaration
    /// \param qualType The qualified type to process
    /// \return Node ID of the created type
    auto createTypeNodeAndRelation(int64_t declNodeId, clang::QualType qualType) -> int64_t;

    /// Create a type node
    /// \param qualType The qualified type to process
    /// \return Node ID of the created type
    auto createTypeNode(clang::QualType qualType) -> int64_t;

    /// Create type relationship
    /// \param declId Declaration node ID
    /// \param typeId Type node ID
    void createTypeRelation(int64_t declId, int64_t typeId);

    /// Extract type name from qualified type
    /// \param qualType The qualified type
    /// \return String representation of the type name
    auto extractTypeName(clang::QualType qualType) -> std::string;

    /// Extract type category
    /// \param qualType The qualified type
    /// \return String representation of the type category
    auto extractTypeCategory(clang::QualType qualType) -> std::string;

    /// Extract type qualifiers
    /// \param qualType The qualified type
    /// \return String representation of type qualifiers
    auto extractTypeQualifiers(clang::QualType qualType) -> std::string;

    /// Check if type is built-in
    /// \param qualType The qualified type
    /// \return True if the type is built-in
    auto isBuiltInType(clang::QualType qualType) -> bool;

    /// Extract type source location (placeholder for now)
    /// \param qualType The qualified type
    /// \return String representation of type source location
    auto extractTypeSourceLocation(clang::QualType qualType) -> std::string;

private:
    KuzuDatabase& database;
    ASTNodeProcessor& nodeProcessor;
};

}  // namespace clang
