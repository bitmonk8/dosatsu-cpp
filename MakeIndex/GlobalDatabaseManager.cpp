//===--- GlobalDatabaseManager.cpp - Global database instance management -===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#include "GlobalDatabaseManager.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

using namespace clang;

auto GlobalDatabaseManager::getInstance() -> GlobalDatabaseManager&
{
    static GlobalDatabaseManager instance;
    return instance;
}

void GlobalDatabaseManager::initializeDatabase(const std::string& databasePath)
{
    if (initialized)
    {
        llvm::errs() << "Warning: Database already initialized, ignoring duplicate initialization\n";
        return;
    }
    
    try
    {
        database = std::make_unique<KuzuDatabase>(databasePath);
        database->initialize();
        initialized = true;
        llvm::outs() << "Global database initialized at: " << databasePath << "\n";
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Failed to initialize global database: " << e.what() << "\n";
        database.reset();
        initialized = false;
        throw;
    }
}

auto GlobalDatabaseManager::getDatabase() -> KuzuDatabase*
{
    return database.get();
}

auto GlobalDatabaseManager::isInitialized() const -> bool
{
    return initialized && database != nullptr;
}

void GlobalDatabaseManager::cleanup()
{
    if (database)
    {
        database->flushOperations();
        database.reset();
    }
    initialized = false;
}

GlobalDatabaseManager::~GlobalDatabaseManager()
{
    cleanup();
}
