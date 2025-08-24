//===--- GlobalDatabaseManager.h - Global database instance management ---===//
//
// Part of the Dosatsu project
//
//===----------------------------------------------------------------------===//

#pragma once

#include "KuzuDatabase.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace clang
{

/// Global singleton for managing database instances across multiple files
class GlobalDatabaseManager
{
public:
    /// Get the singleton instance
    static auto getInstance() -> GlobalDatabaseManager&;

    /// Initialize the global database (call once)
    void initializeDatabase(const std::string& databasePath);

    /// Get the global database instance
    auto getDatabase() -> KuzuDatabase*;

    /// Check if database is initialized
    [[nodiscard]] auto isInitialized() const -> bool;

    /// Get the node ID for a previously processed pointer (global across all files)
    /// \param ptr Pointer to the AST node
    /// \return Node ID if found, -1 otherwise
    auto getGlobalNodeId(const void* ptr) -> int64_t;

    /// Check if a node has already been processed globally
    /// \param ptr Pointer to the AST node
    /// \return True if the node has been processed
    auto hasGlobalNode(const void* ptr) const -> bool;

    /// Register a node ID for a pointer globally
    /// \param ptr Pointer to the AST node
    /// \param nodeId The node ID to associate with the pointer
    void registerGlobalNode(const void* ptr, int64_t nodeId);

    /// Check if a Declaration node has been created for this node ID
    /// \param nodeId The node ID to check
    /// \return True if Declaration node already exists
    [[nodiscard]] auto hasDeclarationNode(int64_t nodeId) const -> bool;

    /// Register that a Declaration node has been created for this node ID
    /// \param nodeId The node ID to register
    void registerDeclarationNode(int64_t nodeId);

    /// Check if a Type node has been created for this node ID
    /// \param nodeId The node ID to check
    /// \return True if Type node already exists
    [[nodiscard]] auto hasTypeNode(int64_t nodeId) const -> bool;

    /// Register that a Type node has been created for this node ID
    /// \param nodeId The node ID to register
    void registerTypeNode(int64_t nodeId);

    /// Check if a Statement node has been created for this node ID
    /// \param nodeId The node ID to check
    /// \return True if Statement node already exists
    [[nodiscard]] auto hasStatementNode(int64_t nodeId) const -> bool;

    /// Register that a Statement node has been created for this node ID
    /// \param nodeId The node ID to register
    void registerStatementNode(int64_t nodeId);

    /// Check if an Expression node has been created for this node ID
    /// \param nodeId The node ID to check
    /// \return True if Expression node already exists
    [[nodiscard]] auto hasExpressionNode(int64_t nodeId) const -> bool;

    /// Register that an Expression node has been created for this node ID
    /// \param nodeId The node ID to register
    void registerExpressionNode(int64_t nodeId);

    /// Cleanup (optional - called automatically on destruction)
    void cleanup();

    // Prevent copy/move - deleted functions should be public for better error messages
    GlobalDatabaseManager(const GlobalDatabaseManager&) = delete;
    auto operator=(const GlobalDatabaseManager&) -> GlobalDatabaseManager& = delete;
    GlobalDatabaseManager(GlobalDatabaseManager&&) = delete;
    auto operator=(GlobalDatabaseManager&&) -> GlobalDatabaseManager& = delete;

private:
    GlobalDatabaseManager() = default;
    ~GlobalDatabaseManager();

    std::unique_ptr<KuzuDatabase> database;
    bool initialized = false;

    // Global node ID map to prevent duplicate processing across files
    std::unordered_map<const void*, int64_t> globalNodeIdMap;

    // Track which specialized nodes have been created to prevent duplicates
    std::unordered_set<int64_t> createdDeclarationNodes;
    std::unordered_set<int64_t> createdTypeNodes;
    std::unordered_set<int64_t> createdStatementNodes;
    std::unordered_set<int64_t> createdExpressionNodes;
};

}  // namespace clang
