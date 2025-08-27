# Performance Overview

This document describes the current performance problems we are focused on solving and what we know about them based on profiling data.

## Current Problem Focus

### Debug Build Performance

**Problem Statement**: Dosatsu exhibits significantly slow performance in debug builds, which creates bottlenecks during development of new features.

**Impact**: Poor debug build performance affects developer productivity by making it slow to:
- Test new features during development
- Debug issues in the codebase
- Iterate on code changes
- Run verification tests

**Goal**: Improve debug build performance to enable efficient feature development without compromising the ability to debug issues.

## Current Performance Characteristics

### Latest Baseline Measurements

**Test Environment**: Debug build, simple example (simple.cpp)  
**Date**: 2025-08-26  
**Total CPU Samples**: 4,645 (using enhanced xperf stack analysis)

### Performance Breakdown by Module

| Component | CPU Usage | Samples | Analysis |
|-----------|-----------|---------|----------|
| **kuzu_shared.dll** | **42.00%** | 1,951 | Database operations - major bottleneck |
| **ntdll.dll** | **29.41%** | 1,366 | System calls, memory management |
| **ucrtbase.dll** | **11.09%** | 515 | Debug C Runtime overhead |
| **ntkrnlmp.exe** | **9.43%** | 438 | Windows kernel operations |
| **nvlddmkm.sys** | **2.02%** | 94 | NVIDIA graphics driver |
| **dosatsu_cpp.exe** | **1.87%** | 87 | Our application logic |
| **vcruntime140d.dll** | **1.66%** | 77 | Visual C++ debug runtime |
| **msvcp140d.dll** | **1.64%** | 76 | Visual C++ standard library debug |

## Key Performance Insights

### 1. Database Bottleneck (Priority 1)
- **42% of CPU time** is spent in Kuzu database operations
- This is the single largest performance bottleneck
- Represents the highest-impact optimization opportunity

### 2. Debug Runtime Overhead (Priority 2)  
- **~13% combined overhead** from debug runtime libraries
- ucrtbase.dll + vcruntime140d.dll + msvcp140d.dll
- This is inherent debug build overhead but may be optimizable

### 3. System Call Heavy Usage (Priority 3)
- **29.41% in ntdll.dll** suggests significant system call overhead
- Could indicate inefficient memory allocation patterns
- May point to excessive file I/O operations

### 4. Application Logic Efficiency
- **Only 1.87%** of time spent in our actual application code
- Suggests the bottleneck is primarily in external dependencies
- Our code itself appears to be reasonably efficient

## Call Stack Analysis Capabilities

With the enhanced `xperf -a stack -butterfly` analysis, we now have:

1. **Function Call Hierarchy**: Complete call paths from entry points to leaf functions
2. **Inclusive vs Exclusive Time**: Functions show both time spent in themselves and time including callees
3. **Caller-Callee Relationships**: Clear visualization of which functions call which others
4. **Multi-Inclusive Hits**: Functions that appear multiple times in call stacks (recursive or multiple callers)

## Performance Testing Approach

### Test Scenarios
- **Primary Test Suite**: Current Examples dataset
- **Focus**: Real-world scenarios that represent actual Dosatsu usage patterns
- **Coverage**: Various C++ code patterns and complexity levels

### Measurement Strategy
- Profile individual examples to understand component-specific performance
- Compare debug vs release build performance characteristics
- Track performance changes over time to detect regressions

## Known Performance Characteristics

### Database Usage Patterns
- Heavy reliance on Kuzu database for AST storage and querying
- Database operations dominate CPU usage in current workloads
- Need to investigate query patterns and transaction efficiency

### Debug vs Release Impact
- Debug runtime libraries add significant overhead (~13%)
- Need to measure actual performance difference between debug and release builds
- Consider hybrid configurations for development scenarios

### System Resource Usage
- High system call usage suggests memory/IO intensive operations
- Graphics driver involvement (nvlddmkm.sys) indicates possible UI rendering overhead
- Kernel time (ntkrnlmp.exe) shows significant Windows system involvement

---

*Last Updated: 2025-08-26*
