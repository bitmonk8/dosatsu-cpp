//===--- ScopeManager.cpp - Hierarchy and scope management --------------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#include "ScopeManager.h"

#include "KuzuDatabase.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

using namespace clang;

ScopeManager::ScopeManager(KuzuDatabase& database) : database(database)
{
}

void ScopeManager::pushParent(int64_t parentNodeId)
{
    parentStack.push_back(parentNodeId);
    childIndex = 0;  // Reset child index for new parent
}

void ScopeManager::popParent()
{
    if (!parentStack.empty())
        parentStack.pop_back();
}

auto ScopeManager::getCurrentParent() -> int64_t
{
    if (parentStack.empty())
        return -1;
    return parentStack.back();
}

void ScopeManager::createHierarchyRelationship(int64_t childNodeId)
{
    int64_t parentId = getCurrentParent();
    if (parentId != -1)
        createParentChildRelation(parentId, childNodeId, childIndex++);
}

void ScopeManager::pushScope(int64_t scopeNodeId)
{
    scopeStack.push_back(scopeNodeId);
}

void ScopeManager::popScope()
{
    if (!scopeStack.empty())
        scopeStack.pop_back();
}

auto ScopeManager::getCurrentScope() -> int64_t
{
    if (scopeStack.empty())
        return -1;
    return scopeStack.back();
}

void ScopeManager::createScopeRelationships(int64_t nodeId)
{
    int64_t currentScope = getCurrentScope();
    if (currentScope != -1)
        createScopeRelation(nodeId, currentScope, "lexical_scope");
}

void ScopeManager::createParentChildRelation(int64_t parentId, int64_t childId, int index)
{
    if (!database.isInitialized() || parentId == -1 || childId == -1)
        return;

    try
    {
        std::string query = "MATCH (p:ASTNode {node_id: " + std::to_string(parentId) +
                            "}), (c:ASTNode {node_id: " + std::to_string(childId) +
                            "}) CREATE (p)-[:PARENT_OF {child_index: " + std::to_string(index) +
                            ", relationship_kind: 'child'}]->(c)";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating parent-child relation: " << e.what() << "\n";
    }
}

void ScopeManager::createScopeRelation(int64_t nodeId, int64_t scopeId, const std::string& scopeKind)
{
    if (!database.isInitialized() || nodeId == -1 || scopeId == -1)
        return;

    try
    {
        std::string query = "MATCH (n:ASTNode {node_id: " + std::to_string(nodeId) +
                            "}), (s:Declaration {node_id: " + std::to_string(scopeId) +
                            "}) CREATE (n)-[:IN_SCOPE {scope_kind: '" + scopeKind + "'}]->(s)";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating scope relation: " << e.what() << "\n";
    }
}
