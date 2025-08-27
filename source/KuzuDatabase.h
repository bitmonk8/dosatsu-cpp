//===--- KuzuDatabase.h - Database management for AST storage ------------===//
//
// Part of the Dosatsu project
//
//===----------------------------------------------------------------------===//

#pragma once

// clang-format off
#include "NoWarningScope_Enter.h"
#include "kuzu.hpp"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <queue>
#include <mutex>

namespace clang
{

/// Manages Kuzu database operations for AST storage
class KuzuDatabase
{
public:
    /// Constructor - initializes database at given path
    /// \param databasePath Path to the Kuzu database
    explicit KuzuDatabase(std::string databasePath);

    /// Destructor - ensures proper cleanup
    ~KuzuDatabase();

    /// Initialize database connection and create schema
    void initialize();

    /// Execute a schema creation query
    /// \param query The SQL query to execute
    /// \param schemaName Name of the schema element being created (for error reporting)
    void executeSchemaQuery(const std::string& query, const std::string& schemaName);

    /// Begin a database transaction
    void beginTransaction();

    /// Commit the current transaction
    void commitTransaction();

    /// Rollback the current transaction
    void rollbackTransaction();

    /// Add query to batch for performance optimization
    /// \param query The query to add to the batch
    void addToBatch(const std::string& query);

    /// Add optimized relationship creation to batch
    /// \param fromNodeId Source node ID
    /// \param toNodeId Target node ID  
    /// \param relationshipType Type of relationship (e.g., "HAS_TYPE", "PARENT_OF")
    /// \param properties Additional relationship properties as key-value pairs
    void addRelationshipToBatch(int64_t fromNodeId, int64_t toNodeId, 
                               const std::string& relationshipType,
                               const std::map<std::string, std::string>& properties = {});

    /// Add bulk relationship creation to batch (most efficient for multiple relationships)
    /// \param relationships Vector of relationship data
    void addBulkRelationshipsToBatch(const std::vector<std::tuple<int64_t, int64_t, std::string, std::map<std::string, std::string>>>& relationships);

    /// Execute all queries in the current batch
    void executeBatch();

    /// Flush any pending operations
    void flushOperations();

    /// Optimize transaction boundaries based on operation count
    void optimizeTransactionBoundaries();

    /// Get the database connection for direct access
    [[nodiscard]] auto getConnection() const -> kuzu::main::Connection* { return connection.get(); }

    /// Check if database is properly initialized
    [[nodiscard]] auto isInitialized() const -> bool { return connection != nullptr; }

    /// Get a connection from the pool (for advanced usage)
    [[nodiscard]] auto getPooledConnection() -> kuzu::main::Connection*;

    /// Get the next available node ID
    /// \return A unique node ID for this database instance
    auto getNextNodeId() -> int64_t { return nextNodeId++; }

    /// Escape string for safe use in Kuzu queries
    /// \param str The string to escape
    /// \return Escaped string safe for Kuzu query usage
    static auto escapeString(const std::string& str) -> std::string;

private:
    /// Create the complete database schema
    void createSchema();

    /// Execute optimized relationship queries in bulk
    void executeOptimizedRelationships();

    /// Execute bulk relationship creation for a specific relationship type
    void executeBulkRelationshipType(const std::string& relationshipType, 
        const std::vector<std::tuple<int64_t, int64_t, std::map<std::string, std::string>>>& relationships);

    /// Fallback method for individual relationship creation if bulk fails
    void executeFallbackRelationships(const std::string& relationshipType,
        const std::vector<std::tuple<int64_t, int64_t, std::map<std::string, std::string>>>& relationships);

    /// Initialize connection pool for better performance
    void initializeConnectionPool();

    std::string databasePath;
    std::unique_ptr<kuzu::main::Database> database;
    std::unique_ptr<kuzu::main::Connection> connection;

    // Connection pooling for performance
    static constexpr size_t CONNECTION_POOL_SIZE = 4;  // Number of pooled database connections
    std::queue<std::unique_ptr<kuzu::main::Connection>> connectionPool;
    std::mutex connectionPoolMutex;

    // Performance optimization - batching
    static constexpr size_t BATCH_SIZE = 150;  // Process this many operations per batch
    static constexpr size_t TRANSACTION_COMMIT_THRESHOLD = 1000;  // Auto-commit after this many operations
    std::vector<std::string> pendingQueries;
    
    // Relationship batching support (currently unused)  
    std::vector<std::tuple<int64_t, int64_t, std::string, std::map<std::string, std::string>>> pendingRelationships;
    
    bool transactionActive = false;
    size_t totalOperations = 0;
    size_t operationsSinceLastCommit = 0;

    // Global node ID counter for uniqueness across all files
    int64_t nextNodeId = 1;
};

}  // namespace clang
