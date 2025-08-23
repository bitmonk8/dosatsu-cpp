//===--- ASTNodeProcessor.cpp - Core AST node processing ----------------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#include "ASTNodeProcessor.h"

#include "GlobalDatabaseManager.h"
#include "KuzuDatabase.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/DeclCXX.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>
#include <sstream>

using namespace clang;

ASTNodeProcessor::ASTNodeProcessor(KuzuDatabase& database, ASTContext& astContext)
    : database(database), sourceManager(&astContext.getSourceManager())
{
}

auto ASTNodeProcessor::createASTNode(const clang::Decl* decl) -> int64_t
{
    if (!database.isInitialized() || (decl == nullptr))
        return -1;

    // Check if already processed globally first
    auto& dbManager = GlobalDatabaseManager::getInstance();
    int64_t existingNodeId = dbManager.getGlobalNodeId(decl);
    if (existingNodeId != -1)
    {
        // Also store in local map for faster lookup
        nodeIdMap[decl] = existingNodeId;
        return existingNodeId;
    }

    // Check if already processed locally
    auto it = nodeIdMap.find(decl);
    if (it != nodeIdMap.end())
        return it->second;

    int64_t nodeId = getNextNodeId();
    nodeIdMap[decl] = nodeId;

    // Register globally to prevent duplicates across files
    dbManager.registerGlobalNode(decl, nodeId);

    try
    {
        // Extract basic information
        std::string nodeType = extractNodeType(decl);

        // Format memory address as hex string
        std::stringstream addrStream;
        addrStream << std::hex << (uintptr_t)decl;
        std::string memoryAddr = addrStream.str();

        bool isImplicit = isImplicitNode(decl);

        // Extract detailed source location information
        auto [filename, startLine, startColumn] = extractSourceLocationDetailed(decl->getLocation());
        auto [endFilename, endLine, endColumn] = extractSourceLocationDetailed(decl->getSourceRange().getEnd());

        // Use start location's filename for source_file
        std::string sourceFile = filename;

        // Create base AST node query using string concatenation
        std::string query =
            "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
            "', is_implicit: " + (isImplicit ? "true" : "false") + ", start_line: " + std::to_string(startLine) +
            ", start_column: " + std::to_string(startColumn) + ", end_line: " + std::to_string(endLine) +
            ", end_column: " + std::to_string(endColumn) + ", raw_text: ''})";

        // Use batched operation for performance optimization
        database.addToBatch(query);

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node: " << e.what() << "\n";
        return -1;
    }
}

auto ASTNodeProcessor::createASTNode(const clang::Stmt* stmt) -> int64_t
{
    if (!database.isInitialized() || (stmt == nullptr))
        return -1;

    // Check if already processed globally first
    auto& dbManager = GlobalDatabaseManager::getInstance();
    int64_t existingNodeId = dbManager.getGlobalNodeId(stmt);
    if (existingNodeId != -1)
    {
        // Also store in local map for faster lookup
        nodeIdMap[stmt] = existingNodeId;
        return existingNodeId;
    }

    // Check if already processed locally
    auto it = nodeIdMap.find(stmt);
    if (it != nodeIdMap.end())
        return it->second;

    int64_t nodeId = getNextNodeId();
    nodeIdMap[stmt] = nodeId;

    // Register globally to prevent duplicates across files
    dbManager.registerGlobalNode(stmt, nodeId);

    try
    {
        // Extract basic information
        std::string nodeType = extractNodeType(stmt);
        // Format memory address as hex string
        std::stringstream addrStream;
        addrStream << std::hex << (uintptr_t)stmt;
        std::string memoryAddr = addrStream.str();

        // Extract detailed source location information
        auto [filename, startLine, startColumn] = extractSourceLocationDetailed(stmt->getBeginLoc());
        auto [endFilename, endLine, endColumn] = extractSourceLocationDetailed(stmt->getEndLoc());

        // Use start location's filename for source_file
        std::string sourceFile = filename;

        // Create base AST node query using string concatenation
        std::string query = "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
                            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
                            "', is_implicit: false, start_line: " + std::to_string(startLine) +
                            ", start_column: " + std::to_string(startColumn) +
                            ", end_line: " + std::to_string(endLine) + ", end_column: " + std::to_string(endColumn) +
                            ", raw_text: ''})";

        // Use batched operation for performance optimization
        database.addToBatch(query);

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node for statement: " << e.what() << "\n";
        return -1;
    }
}

auto ASTNodeProcessor::createASTNode(const clang::Type* type) -> int64_t
{
    if (!database.isInitialized() || (type == nullptr))
        return -1;

    // Check if already processed globally first
    auto& dbManager = GlobalDatabaseManager::getInstance();
    int64_t existingNodeId = dbManager.getGlobalNodeId(type);
    if (existingNodeId != -1)
    {
        // Also store in local map for faster lookup
        nodeIdMap[type] = existingNodeId;
        return existingNodeId;
    }

    // Check if already processed locally
    auto it = nodeIdMap.find(type);
    if (it != nodeIdMap.end())
        return it->second;

    int64_t nodeId = getNextNodeId();
    nodeIdMap[type] = nodeId;

    // Register globally to prevent duplicates across files
    dbManager.registerGlobalNode(type, nodeId);

    try
    {
        // Extract basic information
        std::string nodeType = extractNodeType(type);
        // Format memory address as hex string
        std::stringstream addrStream;
        addrStream << std::hex << (uintptr_t)type;
        std::string memoryAddr = addrStream.str();

        // Types don't have specific source locations, so use empty values
        std::string sourceFile = "";
        int64_t startLine = -1;
        int64_t startColumn = -1;
        int64_t endLine = -1;
        int64_t endColumn = -1;

        // Create base AST node query using string concatenation
        std::string query = "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
                            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
                            "', is_implicit: false, start_line: " + std::to_string(startLine) +
                            ", start_column: " + std::to_string(startColumn) +
                            ", end_line: " + std::to_string(endLine) + ", end_column: " + std::to_string(endColumn) +
                            ", raw_text: ''})";

        // Use batched operation for performance optimization
        database.addToBatch(query);

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node for type: " << e.what() << "\n";
        return -1;
    }
}

auto ASTNodeProcessor::getNodeId(const void* ptr) -> int64_t
{
    // Check local map first for performance
    auto it = nodeIdMap.find(ptr);
    if (it != nodeIdMap.end())
        return it->second;

    // Check global map
    auto& dbManager = GlobalDatabaseManager::getInstance();
    int64_t globalNodeId = dbManager.getGlobalNodeId(ptr);
    if (globalNodeId != -1)
    {
        // Cache in local map for faster future lookups
        nodeIdMap[ptr] = globalNodeId;
        return globalNodeId;
    }

    return -1;
}

auto ASTNodeProcessor::getNextNodeId() -> int64_t
{
    return database.getNextNodeId();
}

auto ASTNodeProcessor::hasNode(const void* ptr) const -> bool
{
    // Check local map first
    if (nodeIdMap.find(ptr) != nodeIdMap.end())
        return true;

    // Check global map
    auto& dbManager = GlobalDatabaseManager::getInstance();
    return dbManager.hasGlobalNode(ptr);
}

auto ASTNodeProcessor::extractSourceLocation(const clang::SourceLocation& loc) -> std::string
{
    if (loc.isInvalid())
        return "<invalid>";

    // Get detailed location information
    auto [fileName, line, column] = extractSourceLocationDetailed(loc);

    // Return formatted location string
    if (fileName == "<invalid>" || line == -1 || column == -1)
        return "<unknown_location>";

    return fileName + ":" + std::to_string(line) + ":" + std::to_string(column);
}

auto ASTNodeProcessor::extractSourceLocationDetailed(const clang::SourceLocation& loc)
    -> std::tuple<std::string, int64_t, int64_t>
{
    if (loc.isInvalid() || (sourceManager == nullptr))
        return std::make_tuple("<invalid>", -1, -1);

    try
    {
        // Use SourceManager to get precise location information
        auto presumedLoc = sourceManager->getPresumedLoc(loc);
        if (presumedLoc.isInvalid())
            return std::make_tuple("<invalid>", -1, -1);

        // Extract filename, line, and column from the presumed location
        std::string filename = (presumedLoc.getFilename() != nullptr) ? presumedLoc.getFilename() : "<unknown>";
        int64_t line = presumedLoc.getLine();
        int64_t column = presumedLoc.getColumn();

        // Clean up filename for database storage (escape single quotes)
        std::ranges::replace(filename, '\'', '_');

        return std::make_tuple(filename, line, column);
    }
    catch (...)
    {
        // If any exception occurs, return invalid location
        return std::make_tuple("<exception>", -1, -1);
    }
}

auto ASTNodeProcessor::extractNodeType(const clang::Decl* decl) -> std::string
{
    if (decl == nullptr)
        return "UnknownDecl";
    return std::string(decl->getDeclKindName()) + "Decl";
}

auto ASTNodeProcessor::extractNodeType(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
        return "UnknownStmt";
    return stmt->getStmtClassName();
}

auto ASTNodeProcessor::extractNodeType(const clang::Type* type) -> std::string
{
    if (type == nullptr)
        return "UnknownType";
    return type->getTypeClassName();
}

auto ASTNodeProcessor::isImplicitNode(const clang::Decl* decl) -> bool
{
    if (decl == nullptr)
        return false;
    return decl->isImplicit();
}
