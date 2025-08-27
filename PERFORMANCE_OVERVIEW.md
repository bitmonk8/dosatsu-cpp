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

### Priority 1: System Call Optimization ✅ Investigation Complete
- **Target**: ntdll.dll operations (31.4% average CPU usage)
- **Root Cause Analysis**: 
  - Memory allocation: 13-14% CPU (RtlpLowFragHeapAllocFromContext dominant)
  - Synchronization overhead: 17-23% CPU (Critical sections, condition variables)
  - String operations: 1% CPU (Hash map operations with string keys)
  - File I/O: Minimal impact (<0.1% CPU)
- **Optimization Strategy**: Focus on memory pools and lock contention reduction
- **Potential Impact**: 25-35% reduction achievable through targeted optimizations

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

*Last Updated: 2025-08-27*  
*Performance baseline established and system call optimization investigation completed*