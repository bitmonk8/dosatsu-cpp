//===--- ASTNodeProcessor.h - Core AST node processing ------------------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#pragma once

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceManager.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>

namespace clang
{

class KuzuDatabase;

/// Handles core AST node creation and basic processing
class ASTNodeProcessor
{
public:
    /// Constructor
    /// \param database Database instance for storage
    /// \param astContext AST context for source location information
    explicit ASTNodeProcessor(KuzuDatabase& database, const ASTContext& astContext);

    /// Create a new AST node for a declaration
    /// \param decl The declaration to create a node for
    /// \return The unique node ID assigned
    auto createASTNode(const clang::Decl* decl) -> int64_t;

    /// Create a new AST node for a statement
    /// \param stmt The statement to create a node for
    /// \return The unique node ID assigned
    auto createASTNode(const clang::Stmt* stmt) -> int64_t;

    /// Create a new AST node for a type
    /// \param type The type to create a node for
    /// \return The unique node ID assigned
    auto createASTNode(const clang::Type* type) -> int64_t;

    /// Get the node ID for a previously processed pointer
    /// \param ptr Pointer to the AST node
    /// \return Node ID if found, -1 otherwise
    auto getNodeId(const void* ptr) -> int64_t;

    /// Check if a node has already been processed
    /// \param ptr Pointer to the AST node
    /// \return True if the node has been processed
    auto hasNode(const void* ptr) const -> bool;

    /// Extract source location as string
    /// \param loc Source location to extract
    /// \return String representation of the location
    auto extractSourceLocation(const clang::SourceLocation& loc) -> std::string;

    /// Extract detailed source location information
    /// \param loc Source location to extract
    /// \return Tuple of (filename, line, column)
    auto extractSourceLocationDetailed(const clang::SourceLocation& loc) -> std::tuple<std::string, int64_t, int64_t>;

    /// Extract node type string for a declaration
    /// \param decl The declaration
    /// \return String representation of the node type
    auto extractNodeType(const clang::Decl* decl) -> std::string;

    /// Extract node type string for a statement
    /// \param stmt The statement
    /// \return String representation of the node type
    auto extractNodeType(const clang::Stmt* stmt) -> std::string;

    /// Extract node type string for a type
    /// \param type The type
    /// \return String representation of the node type
    auto extractNodeType(const clang::Type* type) -> std::string;

    /// Check if a declaration is implicit
    /// \param decl The declaration to check
    /// \return True if the declaration is implicit
    auto isImplicitNode(const clang::Decl* decl) -> bool;

private:
    KuzuDatabase& database;
    const SourceManager* sourceManager;

    // Node tracking
    std::unordered_map<const void*, int64_t> nodeIdMap;  // Pointer -> node_id mapping

    /// Get the next available node ID from the database
    auto getNextNodeId() -> int64_t;
};

}  // namespace clang
