# Performance Overview

This document describes the current performance characteristics of Dosatsu C++ and remaining optimization opportunities.

## Current Performance Characteristics

Based on profiling data from 2025-08-27 across all example categories (3,917 to 29,767 CPU samples):

| Component | CPU Usage Range | Average | Status |
|-----------|-----------------|---------|---------|
| **kuzu_shared.dll** | **43.3% - 44.8%** | **44.0%** | Database operations - primary CPU consumer |
| **ntdll.dll** | **30.4% - 32.3%** | **31.4%** | System calls and memory management |
| **ucrtbase.dll** | **9.8% - 10.7%** | **10.2%** | C Runtime overhead |
| **ntkrnlmp.exe** | **6.9% - 9.9%** | **8.1%** | Windows kernel operations |
| **Debug Runtime** | **15.0% - 16.2%** | **15.6%** | Combined debug libraries (vcruntime140d.dll + msvcp140d.dll) |
| **dosatsu_cpp.exe** | **0.85% - 1.77%** | **1.3%** | Application logic |

## Key Performance Insights

### 1. Database Operations (44.0% CPU Usage)
Database operations consume the largest portion of CPU time. Processing scales linearly with C++ code complexity.

### 2. System Call Overhead (31.4% CPU Usage) 
System calls through ntdll.dll represent the second-largest bottleneck, indicating memory allocation and I/O patterns that could be optimized.

### 3. Debug Runtime Overhead (15.6% CPU Usage)
Debug build configuration creates significant overhead that impacts development experience.

### 4. Application Logic Efficiency (1.3% CPU Usage)
Application code is highly efficient - optimization focus should be on external dependencies.

## Performance Infrastructure

The project has comprehensive profiling capabilities:
- **ETW profiling** with etwprof.exe for performance data collection
- **Stack analysis** with xperf butterfly view for call hierarchy analysis  
- **Automated profiling** integrated into Examples runner (`--profile` option)
- **Analysis pipeline** for generating performance reports

See PERFORMANCE_INFRASTRUCTURE.md for detailed usage instructions.

## Optimization Opportunities

### Priority 1: Database Query Optimization ✅ Investigation Complete
- **Target**: kuzu_shared.dll operations (43-44% average CPU usage)
- **Root Cause Analysis**: 
  - Memory allocation: 13-14% CPU from Kuzu operations
    - Individual query execution despite batching infrastructure
    - String-based query construction with repeated parsing
  - Synchronization overhead: 17-23% CPU from database locks
    - Individual operations causing lock contention
    - Inefficient transaction boundaries
  - Query processing inefficiencies:
    - Pseudo-batching: queries executed individually in loops
    - Disabled relationship batching (fallback to MATCH...CREATE)
    - No prepared statements or query caching
- **Optimization Strategy**: Implement true bulk operations and optimize query patterns
- **Potential Impact**: 50-70% reduction in database CPU usage through bulk operations

### Priority 2: Debug Runtime Optimization  
- **Target**: Debug runtime overhead (15.6% combined)
- **Focus**: Optimized debug configurations that preserve debugging capabilities
- **Potential Impact**: Improved development experience

## Detailed System Call Analysis

### Memory Allocation Bottlenecks
Based on ETW profiling analysis across all examples (3,917 to 29,767 CPU samples):

**Primary Allocation Functions:**
- `RtlpLowFragHeapAllocFromContext`: 12-14% of total CPU usage
- `RtlAllocateHeap`: 1% of total CPU usage
- Debug allocator (`_malloc_dbg`): Additional 15k-66k samples per example

**Allocation Patterns:**
- Frequent small allocations dominate performance profile
- Heap fragmentation contributing to allocation overhead
- String-heavy operations triggering excessive allocations

### Memory Allocation Source Analysis
Detailed call stack analysis reveals the origin of memory allocations:

**Allocation Distribution by Module:**
- **kuzu_shared.dll (43-44% CPU)**: Primary source of allocations
  - Database operations and SQL query processing
  - ANTLR parser components for SQL parsing
  - STL containers (vectors, unordered_maps, shared_ptrs)
  - ValueVectors, ExpressionEvaluators, DataChunks
- **dosatsu_cpp.exe (0.85-1.77% CPU)**: Minimal allocation activity
  - LLVM/Clang AST processing (integrated)
  - Application-level string and container operations
- **System overhead (ntdll.dll 30-32% CPU)**: Serving allocation requests

**Key Finding:** The Kuzu database library, not the application code or LLVM/Clang, is responsible for the vast majority of memory allocations. The application code itself is highly efficient.

### Database Query Pattern Analysis
Code investigation reveals critical inefficiencies in database operations:

**Query Execution Patterns:**
- **Pseudo-batching**: Despite BATCH_SIZE=150, queries execute individually in loops
- **Disabled bulk operations**: Relationship batching commented out as "disabled due to schema complexity"
- **String concatenation**: Every query built via string concatenation, no prepared statements
- **Individual CREATE statements**: Each AST node and relationship creates separate query

**Performance Bottlenecks:**
- Query parsing overhead: ANTLR parses each query individually
- Memory allocations: Each query allocates intermediate results
- Lock contention: Individual operations compete for database locks
- Transaction overhead: Suboptimal transaction boundaries

**Optimization Potential:**
- True bulk operations could reduce database CPU by 50-70%
- Prepared statements could eliminate 20-30% of parsing overhead
- Proper batching could reduce lock contention significantly

### Synchronization Analysis
Critical section contention is the primary driver of ntdll.dll overhead:

**Lock Contention Functions:**
- `RtlLeaveCriticalSection`: Up to 23% CPU (732k samples in complex examples)
- `RtlEnterCriticalSection`: Up to 17% CPU (534k samples in complex examples)
- `RtlSleepConditionVariableSRW`: 18% CPU waiting on condition variables
- `RtlpAcquireSRWLockExclusiveContended`: 18% CPU in lock contention

**Contention Sources:**
- Database operations (kuzu_shared.dll) creating lock pressure
- Heap allocation synchronization creating bottlenecks
- Multi-threaded access patterns requiring optimization

## Performance Analysis Commands

```bash
# Profile all examples
python Examples/run_examples.py --profile

# Profile specific example  
python Examples/run_examples.py --profile --example simple

# Analyze profiling results
python scripts/analyze_profile.py --directory artifacts/profile
```

---

*Last Updated: 2025-08-28*  
*Database query pattern analysis completed - specific optimization strategies identified with 50-70% potential CPU reduction*