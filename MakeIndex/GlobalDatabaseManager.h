//===--- GlobalDatabaseManager.h - Global database instance management ---===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#pragma once

#include "KuzuDatabase.h"

#include <memory>
#include <string>

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
    auto isInitialized() const -> bool;

    /// Cleanup (optional - called automatically on destruction)
    void cleanup();

private:
    GlobalDatabaseManager() = default;
    ~GlobalDatabaseManager();

    // Prevent copy/move
    GlobalDatabaseManager(const GlobalDatabaseManager&) = delete;
    GlobalDatabaseManager& operator=(const GlobalDatabaseManager&) = delete;
    GlobalDatabaseManager(GlobalDatabaseManager&&) = delete;
    GlobalDatabaseManager& operator=(GlobalDatabaseManager&&) = delete;

    std::unique_ptr<KuzuDatabase> database;
    bool initialized = false;
};

}  // namespace clang
