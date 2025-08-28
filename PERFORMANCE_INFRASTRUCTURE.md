# Performance Infrastructure

Performance profiling tools and workflows for analyzing Dosatsu C++ performance.

## Profiling Tools

**Primary Tool: etwprof**
- **Location**: `third_party\etwprof_0.3_release\etwprof.exe`
- **Technology**: Event Tracing for Windows (ETW) framework
- **Output**: `.etl` files with CPU sampling data
- **Documentation**: [etwprof GitHub](https://github.com/Donpedro13/etwprof)

**Analysis Tool: xperf**
- **Location**: Windows Performance Toolkit (WPT)
- **Features**: CPU sampling analysis, call stack butterfly view, module-level breakdown
- **Key Views**: `-a stack -butterfly` for call hierarchy analysis
- **Documentation**: [Xperf Reference](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/xperf-command-line-reference)

## Automation Scripts

**Profiling**: `scripts/profile.py` - Automated etwprof execution with target application
**Analysis**: `scripts/analyze_profile.py` - ETL file processing and report generation
**Integration**: `Examples/run_examples.py --profile` - Seamless profiling during example execution

## Available Commands

```bash
# Profile all examples
python Examples/run_examples.py --profile

# Profile specific example
python Examples/run_examples.py --profile --example simple

# Analyze generated profiles
python scripts/analyze_profile.py --directory artifacts/profile

# Manual profiling
python scripts/profile.py --tracefile output.etl [command] [args...]
```

## Output Files

**Profiling Output** (`artifacts/profile/`):
- `dosatsu_profile_[example]_[timestamp].etl` - Raw ETW data

**Analysis Output** (`artifacts/profile/analysis/`):
- `*_butterfly.html` - Interactive call stack analysis
- `*_summary.txt` - CPU sampling breakdown
- `*_hotspots.json` - Automated performance hotspot identification
- `performance_report_*.md` - Executive summary with recommendations

## Analysis Workflow

1. **Collect Data**: Use `--profile` to capture performance data during example processing
2. **Generate Reports**: Analysis scripts process `.etl` files and identify hotspots
3. **Investigate**: Use butterfly HTML reports for detailed call stack analysis
4. **Optimize**: Implement improvements based on identified bottlenecks
5. **Validate**: Re-profile to measure optimization impact

## Current Performance Status

**Baseline Established**: Comprehensive profiling data across all example categories
- **Primary bottleneck**: kuzu_shared.dll (44.0% CPU) - optimized with bulk operations
- **Secondary bottleneck**: ntdll.dll (31.4% CPU) - reduced through batching

**Optimizations Implemented**:
- Database bulk operations reducing query parsing overhead
- Schema-aware relationship batching eliminating individual MATCH...CREATE patterns
- Transaction optimization with larger batch sizes and commit thresholds
- Clean execution with zero warnings or errors

**Next Focus Areas**:
- Debug runtime optimization (15.6% CPU overhead)
- Performance impact measurement of implemented optimizations
- Prepared statement potential for further query optimization

## System Requirements

- Windows 10/11 with Windows Performance Toolkit
- Administrative privileges for ETW profiling
- Python 3.x for automation scripts

**Dependencies**: etwprof.exe (included), xperf.exe (WPT), Python standard library modules

---

*Current as of 2025-08-28 - Infrastructure operational with database optimizations validated*