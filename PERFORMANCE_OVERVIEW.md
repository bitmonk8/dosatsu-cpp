# Performance Overview

Dosatsu C++ performance characteristics and current optimization status.

## Current Performance Profile

Based on profiling data across all example categories:

| Component | CPU Usage | Status |
|-----------|-----------|---------|
| **kuzu_shared.dll** | **44.0%** | Database operations - optimized with bulk operations |
| **ntdll.dll** | **31.4%** | System calls - reduced through batching optimizations |
| **Debug Runtime** | **15.6%** | Debug libraries - optimization opportunity remains |
| **ucrtbase.dll** | **10.2%** | C Runtime - stable overhead |
| **ntkrnlmp.exe** | **8.1%** | Windows kernel - stable overhead |
| **dosatsu_cpp.exe** | **1.3%** | Application code - highly efficient |

## Key Performance Insights

**Database Operations (44.0% CPU)**: The Kuzu database dominates CPU usage. Now optimized with bulk operations that process multiple nodes and relationships per query instead of individual operations.

**System Call Overhead (31.4% CPU)**: Memory allocation and synchronization overhead from database operations. Significantly reduced through batching optimizations.

**Application Efficiency**: The core application code is highly efficient at 1.3% CPU usage.

## Current Optimizations

### Database Operations - Fully Optimized
- **Bulk node creation**: Multi-node CREATE statements (up to 50 nodes per query)
- **Schema-aware relationship batching**: UNWIND-based bulk relationship operations
- **Transaction optimization**: 500-operation batches with 5000-operation commit thresholds
- **Boolean property handling**: Proper type conversion for relationship properties
- **CSV bulk import**: Available for very large datasets

All database operations now use bulk processing instead of individual queries, significantly reducing parsing overhead and lock contention.

### Remaining Optimization Opportunities
- **Debug Runtime (15.6% CPU)**: Optimized debug configurations could improve development experience
- **Prepared statements**: Further query optimization potential for repeated patterns

## Performance Infrastructure

The project includes comprehensive profiling capabilities:
- **ETW profiling** with automated Examples runner integration (`--profile` option)
- **Analysis pipeline** generating performance reports and hotspot identification
- **Validation testing** across multiple example categories

## Usage Commands

```bash
# Profile all examples
python Examples/run_examples.py --profile

# Profile specific example
python Examples/run_examples.py --profile --example simple

# Analyze profiling results
python scripts/analyze_profile.py --directory artifacts/profile
```

---

*Current as of 2025-08-28 - Database optimizations implemented and validated*