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

### Priority 1: System Call Optimization
- **Target**: ntdll.dll operations (31.4% average CPU usage)
- **Focus**: Memory allocation patterns and I/O efficiency
- **Potential Impact**: Significant performance improvement opportunity

### Priority 2: Debug Runtime Optimization  
- **Target**: Debug runtime overhead (15.6% combined)
- **Focus**: Optimized debug configurations that preserve debugging capabilities
- **Potential Impact**: Improved development experience

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
*Performance baseline established from current profiling data*