//===--- ScopeManager.h - Hierarchy and scope management ----------------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace clang
{

class KuzuDatabase;

/// Manages parent-child relationships and scope tracking during AST traversal
class ScopeManager
{
public:
    /// Constructor
    /// \param database Database instance for creating relationships
    explicit ScopeManager(KuzuDatabase& database);

    /// Push a new parent node onto the stack
    /// \param parentNodeId Node ID of the parent
    void pushParent(int64_t parentNodeId);

    /// Pop the current parent from the stack
    void popParent();

    /// Get the current parent node ID
    /// \return Current parent node ID, or -1 if no parent
    auto getCurrentParent() -> int64_t;

    /// Create hierarchy relationship for a child node
    /// \param childNodeId Node ID of the child
    void createHierarchyRelationship(int64_t childNodeId);

    /// Push a new scope node onto the stack
    /// \param scopeNodeId Node ID of the scope
    void pushScope(int64_t scopeNodeId);

    /// Pop the current scope from the stack
    void popScope();

    /// Get the current scope node ID
    /// \return Current scope node ID, or -1 if no scope
    auto getCurrentScope() -> int64_t;

    /// Create scope relationships for a node
    /// \param nodeId Node ID to create relationships for
    void createScopeRelationships(int64_t nodeId);

    /// Create parent-child relationship
    /// \param parentId Parent node ID
    /// \param childId Child node ID
    /// \param index Index of child within parent
    void createParentChildRelation(int64_t parentId, int64_t childId, int index);

    /// Create scope relationship
    /// \param nodeId Node ID
    /// \param scopeId Scope node ID
    /// \param scopeKind Kind of scope relationship
    void createScopeRelation(int64_t nodeId, int64_t scopeId, const std::string& scopeKind);

private:
    KuzuDatabase& database;

    // Hierarchy tracking
    std::vector<int64_t> parentStack;  // Stack of parent node IDs during traversal
    int childIndex = 0;                // Index of current child within parent

    // Scope tracking
    std::vector<int64_t> scopeStack;  // Stack of scope node IDs (namespaces, functions, etc.)
};

}  // namespace clang
