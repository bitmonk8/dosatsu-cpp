//===--- GlobalDatabaseManager.cpp - Global database instance management -===//
//
// Part of the Dosatsu project
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

auto GlobalDatabaseManager::getGlobalNodeId(const void* ptr) -> int64_t
{
    auto it = globalNodeIdMap.find(ptr);
    return (it != globalNodeIdMap.end()) ? it->second : -1;
}

auto GlobalDatabaseManager::hasGlobalNode(const void* ptr) const -> bool
{
    return globalNodeIdMap.find(ptr) != globalNodeIdMap.end();
}

void GlobalDatabaseManager::registerGlobalNode(const void* ptr, int64_t nodeId)
{
    globalNodeIdMap[ptr] = nodeId;
}

auto GlobalDatabaseManager::hasDeclarationNode(int64_t nodeId) const -> bool
{
    return createdDeclarationNodes.find(nodeId) != createdDeclarationNodes.end();
}

void GlobalDatabaseManager::registerDeclarationNode(int64_t nodeId)
{
    createdDeclarationNodes.insert(nodeId);
}

auto GlobalDatabaseManager::hasTypeNode(int64_t nodeId) const -> bool
{
    return createdTypeNodes.find(nodeId) != createdTypeNodes.end();
}

void GlobalDatabaseManager::registerTypeNode(int64_t nodeId)
{
    createdTypeNodes.insert(nodeId);
}

auto GlobalDatabaseManager::hasStatementNode(int64_t nodeId) const -> bool
{
    return createdStatementNodes.find(nodeId) != createdStatementNodes.end();
}

void GlobalDatabaseManager::registerStatementNode(int64_t nodeId)
{
    createdStatementNodes.insert(nodeId);
}

auto GlobalDatabaseManager::hasExpressionNode(int64_t nodeId) const -> bool
{
    return createdExpressionNodes.find(nodeId) != createdExpressionNodes.end();
}

void GlobalDatabaseManager::registerExpressionNode(int64_t nodeId)
{
    createdExpressionNodes.insert(nodeId);
}

void GlobalDatabaseManager::cleanup()
{
    if (database)
    {
        database->flushOperations();
        database.reset();
    }
    globalNodeIdMap.clear();
    createdDeclarationNodes.clear();
    createdTypeNodes.clear();
    createdStatementNodes.clear();
    createdExpressionNodes.clear();
    initialized = false;
}

GlobalDatabaseManager::~GlobalDatabaseManager()
{
    cleanup();
}
