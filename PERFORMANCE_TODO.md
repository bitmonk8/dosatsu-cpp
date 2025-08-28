# Performance TODO

This document outlines the current performance optimization opportunities for Dosatsu C++.

## Optimization Priorities

### Priority 1: System Call Optimization 🎯 HIGH IMPACT

**Target**: ntdll.dll operations (31.4% average CPU usage)  
**Impact**: Second largest performance bottleneck with high optimization potential

**Investigation Required**:
- [x] Profile memory allocation patterns to identify hotspots
- [x] Investigate file I/O efficiency and caching opportunities  
- [x] Review string operations for optimization potential
- [x] Analyze system call patterns using memory profiling tools

**Success Metrics**:
- Reduce ntdll.dll CPU usage by 25-35% (target: <23%)
- Improve memory allocation efficiency across all example types

### Priority 2: Debug Runtime Optimization 🔧 DEVELOPMENT EXPERIENCE

**Target**: Debug runtime overhead (15.6% combined debug libraries)  
**Impact**: Development experience and iteration speed improvement

**Investigation Required**:
- [ ] Research optimized debug configurations that preserve debugging capabilities
- [ ] Document recommended development build configurations
- [ ] Test performance impact of different debug optimization levels

**Success Metrics**:
- Reduce debug runtime overhead by 20-30% while maintaining debugging capabilities
- Improve development build performance

## Investigation Phase: System Resource Analysis

### Memory Allocation Investigation 📋 COMPLETED

**Objective**: Understand what's driving the 31.4% ntdll.dll usage

**Actions Completed**:
- [x] Memory allocation profiling using heap profiling tools
- [x] System call tracing to identify specific bottlenecks
- [x] Correlation analysis between system calls and processing phases
- [x] Identify memory allocation patterns that can be optimized

**Key Findings**:

#### Memory Allocation Patterns (13-14% of total CPU usage across examples):
- **RtlpLowFragHeapAllocFromContext**: Primary allocation function consuming 12-14% CPU
- **RtlAllocateHeap**: Secondary allocation function consuming 1% CPU  
- **Pattern**: Frequent small allocations dominate - typical allocation weights 144k-1.1M samples
- **Debug overhead**: Additional _malloc_dbg calls adding 15k-66k samples per example

#### Synchronization Overhead (40% of ntdll.dll usage):
- **RtlLeaveCriticalSection**: 23% of CPU in complex examples (732k samples)
- **RtlEnterCriticalSection**: 17% of CPU in complex examples (534k samples)
- **RtlSleepConditionVariableSRW**: 18% CPU waiting on condition variables
- **RtlpAcquireSRWLockExclusiveContended**: 18% CPU in lock contention

#### String Operations Impact:
- Heavy std::string usage in templates and hash operations
- String construction/destruction consuming 1% CPU (10k-11k samples)
- Hash map operations with string keys showing frequent allocation patterns

#### File I/O Patterns:
- Minimal WriteFile operations (1k samples across examples)
- I/O not a significant contributor to system call overhead
- Most system calls are memory and synchronization related

#### Critical Section Contention Analysis:
- Lock contention is primary driver of ntdll.dll overhead
- Database operations (kuzu_shared.dll) causing lock pressure
- Heap allocation synchronization creating bottlenecks

#### Memory Allocation Source Analysis ✅ COMPLETED
**Key Discovery**: Kuzu database is the primary allocation source, not application code
- **kuzu_shared.dll**: 43-44% of total CPU (primary allocation source)
  - Database operations, SQL parsing (ANTLR), STL containers
  - ValueVectors, ExpressionEvaluators, DataChunks
- **dosatsu_cpp.exe**: Only 0.85-1.77% of CPU (highly efficient)
  - Includes LLVM/Clang AST processing
- **Implication**: Optimization must focus on Kuzu database, not application code

#### Database Query Pattern Analysis ✅ COMPLETED (2025-08-28)
**Code Investigation Findings**:

**Current Implementation Issues:**
1. **Pseudo-batching, not true batching**: KuzuDatabase.cpp:199-229
   - `executeBatch()` loops through queries executing them individually
   - No actual bulk operations despite batching infrastructure
   - Each query still parsed and executed separately

2. **Relationship batching disabled**: KuzuDatabase.cpp:218
   - Comment: "Complex relationship batching disabled due to schema complexity"
   - Falls back to individual MATCH...CREATE patterns (inefficient)
   - `executeFallbackRelationships()` creates one query per relationship

3. **String-based query construction**: ASTNodeProcessor.cpp:75-82
   - Every node/relationship builds queries via string concatenation
   - No prepared statements or parameterized queries
   - Causes repeated query parsing overhead in Kuzu

4. **Individual CREATE statements**: 
   - Each AST node creates separate: `CREATE (n:ASTNode {node_id: X, ...})`
   - Could use bulk CSV import or multi-node CREATE statements
   - PARENT_OF relationships created individually (ScopeManager.cpp:81-84)

5. **Missing optimization opportunities**:
   - No query result caching for frequently accessed data
   - Connection pool created but underutilized (CONNECTION_POOL_SIZE = 4)
   - No use of Kuzu's native bulk import capabilities

**Performance Impact:**
- Kuzu spends significant CPU on:
  - Query parsing (ANTLR) for each individual query
  - Transaction management overhead
  - Memory allocation for intermediate results
  - Lock contention from individual operations

### Implementation Phase (Priority Queue)

**Database-Focused Optimizations (Based on Code Investigation)**:

**Priority 1: Implement True Bulk Operations** 🎯 HIGHEST IMPACT
- [ ] Replace pseudo-batching in `executeBatch()` with true bulk operations
- [ ] Use Kuzu's COPY FROM CSV for initial node creation (can reduce insertions by 10-100x)
- [ ] Implement multi-row INSERT: `CREATE (n1:ASTNode {...}), (n2:ASTNode {...}), ...`
- [ ] Fix disabled relationship batching (KuzuDatabase.cpp:218)
- **Expected Impact**: 50-70% reduction in database CPU usage

**Priority 2: Optimize Query Generation** 
- [ ] Implement prepared statements with parameter binding
- [ ] Cache compiled query plans for repeated patterns
- [ ] Replace string concatenation with parameterized queries
- [ ] Create query templates for common operations
- **Expected Impact**: 20-30% reduction in query parsing overhead

**Priority 3: Restructure Database Operations**
- [ ] Collect all AST nodes first, then bulk insert
- [ ] Group relationships by type for bulk creation
- [ ] Use transaction boundaries more efficiently (currently commits every 1000 ops)
- [ ] Implement two-phase processing: nodes first, then all relationships
- **Expected Impact**: 15-25% reduction in transaction overhead

**Priority 4: Leverage Kuzu Features**
- [ ] Utilize connection pool properly (currently underused)
- [ ] Configure Kuzu buffer pool size based on workload
- [ ] Enable Kuzu's query result caching
- [ ] Use Kuzu's native bulk import API instead of Cypher
- **Expected Impact**: 10-15% overall performance improvement

**Priority 5: Code-Level Optimizations**
- [ ] Pre-allocate query string buffers to reduce allocations
- [ ] Reuse query objects instead of creating new ones
- [ ] Implement query result pooling for common lookups
- [ ] Optimize escapeString() function (called for every query)

**Debug Build Optimizations**:
- [ ] Research and implement optimized debug configurations
- [ ] Document recommended development build settings
- [ ] Validate debugging capabilities are preserved

## Performance Monitoring (Future Enhancement)

### Continuous Performance Tracking
- [ ] Integrate performance testing into CI/CD pipeline
- [ ] Establish performance regression alerts and thresholds
- [ ] Document optimization guidelines for future development

## Available Tools and Commands

### Profiling Commands
```bash
# Profile all examples
python Examples/run_examples.py --profile

# Profile specific example
python Examples/run_examples.py --profile --example simple

# Analyze profiling results
python scripts/analyze_profile.py --directory artifacts/profile
```

### Current Baseline
Performance baseline established from 2025-08-27 profiling data across all example categories (3,917 to 29,767 CPU samples).

---

## Implementation Examples

### Example 1: True Bulk Node Creation
**Current (inefficient):**
```cpp
// KuzuDatabase.cpp:199-216
for (const auto& query : pendingQueries) {
    auto result = connection->query(query);  // Individual execution
}
```

**Optimized:**
```cpp
// Create multiple nodes in single query
std::string bulkQuery = "CREATE ";
for (size_t i = 0; i < pendingQueries.size(); ++i) {
    if (i > 0) bulkQuery += ", ";
    bulkQuery += "(n" + std::to_string(i) + ":ASTNode {...})";
}
auto result = connection->query(bulkQuery);  // Single execution
```

### Example 2: CSV Bulk Import
**Optimal for large datasets:**
```cpp
// Write nodes to CSV file
writeNodesToCSV(nodes, "nodes.csv");
// Single bulk import instead of thousands of queries
connection->query("COPY ASTNode FROM 'nodes.csv' WITH HEADER");
```

### Example 3: Prepared Statements
**Current (inefficient):**
```cpp
std::string query = "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ...
```

**Optimized:**
```cpp
// Prepare once, execute many times
auto prepared = connection->prepare("CREATE (n:ASTNode {node_id: $1, node_type: $2, ...})");
prepared->execute(nodeId, nodeType, ...);
```

---

*Last Updated: 2025-08-28*  
*Status: Database query pattern analysis completed - specific optimization strategies identified*