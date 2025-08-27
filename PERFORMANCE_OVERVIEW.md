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

### Comprehensive Baseline Measurements

**Test Environment**: Debug build, all examples profiled  
**Date**: 2025-08-27  
**Analysis Method**: ETW profiling with xperf stack analysis across 13 example categories

### Performance Patterns Across All Examples

**Consistent Performance Profile** - All examples show remarkably similar CPU usage patterns:

| Component | CPU Usage Range | Average | Analysis |
|-----------|----------------|---------|----------|
| **kuzu_shared.dll** | **43.13% - 44.26%** | **43.7%** | Database operations - primary bottleneck |
| **ntdll.dll** | **29.16% - 32.64%** | **31.0%** | System calls, memory management |
| **ucrtbase.dll** | **9.67% - 10.95%** | **10.2%** | Debug C Runtime overhead |
| **ntkrnlmp.exe** | **7.19% - 9.17%** | **8.1%** | Windows kernel operations |
| **dosatsu_cpp.exe** | **0.97% - 1.87%** | **1.3%** | Our application logic |
| **Debug Runtime** | **4.93% - 5.84%** | **5.4%** | Combined vcruntime140d.dll + msvcp140d.dll |

### Scale and Complexity Correlation

Examples show performance scales with C++ code complexity:

| Example Category | Total Samples | Relative Complexity |
|------------------|---------------|-------------------|
| Simple | 4,223 | 1.0x (baseline) |
| Preprocessor Advanced | 5,692 | 1.3x |
| Templates | 7,011 | 1.7x |
| Inheritance | 8,700 | 2.1x |
| Namespaces | 10,570 | 2.5x |
| Expressions | 11,320 | 2.7x |
| Complete | 12,468 | 3.0x |
| Modern C++ Features | 16,228 | 3.8x |
| Advanced Features | 18,698 | 4.4x |
| Clean Code | 18,988 | 4.5x |
| Standard | 21,442 | 5.1x |
| Schema Coverage Complete | 24,414 | 5.8x |

## Key Performance Insights

### 1. Database Bottleneck (Priority 1)
- **43.7% of CPU time** consistently spent in Kuzu database operations across all examples
- This is the single largest performance bottleneck with universal impact
- Pattern is consistent regardless of C++ code complexity, suggesting database overhead is the limiting factor

### 2. System Call Heavy Usage (Priority 2)
- **31.0% in ntdll.dll** suggests significant system call overhead across all workloads
- Likely indicates inefficient memory allocation patterns or excessive file I/O
- Second largest bottleneck with consistent impact across all examples

### 3. Debug Runtime Overhead (Priority 3)  
- **~15.6% combined overhead** from debug runtime libraries (ucrtbase.dll + vcruntime140d.dll + msvcp140d.dll)
- This is inherent debug build overhead but represents significant CPU usage
- Consistent across all examples, suggesting optimization potential

### 4. Application Logic Efficiency
- **Only 1.3% average** time spent in our actual application code (dosatsu_cpp.exe)
- Remarkable consistency (0.97%-1.87%) regardless of C++ code complexity
- Suggests our code itself is highly efficient; bottlenecks are in external dependencies

### 5. Scaling Characteristics
- **Performance scales linearly** with C++ code complexity (5.8x samples for most complex vs simplest)
- **Bottleneck ratios remain constant** - indicates database and system calls don't become more efficient with larger workloads
- **No parallelization benefits observed** - CPU core utilization patterns show processing concentrated on subset of cores

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

### Debug Runtime Impact
- Debug runtime libraries add significant overhead (~15.6% combined)
- Pattern is consistent across all examples, suggesting inherent debug build characteristics
- Release build profiling is a separate project - current focus is on debug build optimization

### System Resource Usage
- High system call usage suggests memory/IO intensive operations
- Graphics driver involvement (nvlddmkm.sys) indicates possible UI rendering overhead
- Kernel time (ntkrnlmp.exe) shows significant Windows system involvement

---

*Last Updated: 2025-08-27 - Comprehensive profiling of all 13 examples completed*

