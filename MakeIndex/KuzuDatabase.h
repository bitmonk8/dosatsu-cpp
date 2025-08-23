//===--- KuzuDatabase.h - Database management for AST storage ------------===//
//
// Part of the MakeIndex project
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

    /// Execute all queries in the current batch
    void executeBatch();

    /// Flush any pending operations
    void flushOperations();

    /// Get the database connection for direct access
    [[nodiscard]] auto getConnection() const -> kuzu::main::Connection* { return connection.get(); }

    /// Check if database is properly initialized
    [[nodiscard]] auto isInitialized() const -> bool { return connection != nullptr; }

    /// Get the next available node ID
    /// \return A unique node ID for this database instance
    auto getNextNodeId() -> int64_t { return nextNodeId++; }

private:
    /// Create the complete database schema
    void createSchema();

    std::string databasePath;
    std::unique_ptr<kuzu::main::Database> database;
    std::unique_ptr<kuzu::main::Connection> connection;

    // Performance optimization - batching
    static constexpr size_t BATCH_SIZE = 100;
    std::vector<std::string> pendingQueries;
    bool transactionActive = false;
    size_t totalOperations = 0;

    // Global node ID counter for uniqueness across all files
    int64_t nextNodeId = 1;
};

}  // namespace clang
