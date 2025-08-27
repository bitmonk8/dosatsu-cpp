# Performance Infrastructure

This document describes the performance profiling tools and workflows available for analyzing Dosatsu C++ performance.

## Profiling Tools

### Primary Profiling Tool: etwprof
- **Location**: `third_party\etwprof_0.3_release\etwprof.exe`
- **Technology**: Event Tracing for Windows (ETW) framework
- **Output**: Generates `.etl` files with sampling data
- **Documentation**: [etwprof GitHub Repository](https://github.com/Donpedro13/etwprof)

### Analysis Tool: xperf
- **Location**: `C:\Program Files (x86)\Windows Kits\10\Windows Performance Toolkit`
- **Purpose**: Process `.etl` files to identify performance hotspots
- **Key Features**: 
  - CPU sampling analysis
  - Call stack butterfly view (`-a stack -butterfly`)
  - Module-level performance breakdown
- **Documentation**: [Xperf Command-Line Reference](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/xperf-command-line-reference)

## Automation Scripts

### Profiling Automation: `scripts/profile.py`
- **Function**: Run commands with profiling enabled
- **Usage**: Automatically starts etwprof, executes target application, and collects ETL data
- **Integration**: Used by Examples runner for seamless profiling

### Analysis Automation: `scripts/analyze_profile.py`
- **Function**: Process .etl files using xperf and generate performance reports
- **Features**:
  - Automated xperf command execution
  - HTML butterfly report generation
  - Performance hotspot extraction
  - Executive summary report generation

### Examples Integration: `Examples/run_examples.py`
- **Options**:
  - `--profile`: Enable performance profiling
  - `--profile-output DIR`: Custom output directory for profiling files
  - `--example EXAMPLE`: Profile specific example only

## Available Commands

### Profiling Commands
```bash
# Profile a single example
python Examples/run_examples.py --profile --example simple

# Profile all examples
python Examples/run_examples.py --profile

# Profile with custom output directory
python Examples/run_examples.py --profile --profile-output my_analysis

# Manual profiling of any command
python scripts/profile.py --tracefile output.etl [command] [args...]
```

### Analysis Commands
```bash
# Analyze specific .etl file
python scripts/analyze_profile.py --file artifacts/profile/dosatsu_profile_*.etl

# Analyze all .etl files in directory
python scripts/analyze_profile.py --directory artifacts/profile
```

## Output Files Generated

### From Profiling (`artifacts/profile/`)
- `dosatsu_profile_[example]_[timestamp].etl` - Raw ETW profiling data

### From Analysis (`artifacts/profile/analysis/`)
- `*_butterfly.html` - Interactive call stack analysis with xperf butterfly view
- `*_summary.txt` - CPU sampling summary by xperf
- `*_stacks.csv` - Stack data for compatibility
- `*_hotspots.json` - Automated hotspot analysis results
- `performance_report_*.md` - Executive summary with recommendations

## Analysis Capabilities

### ETW Profiling Features
1. **Call Stack Butterfly View**: Visual representation of function call relationships
2. **HTML Reports**: Rich HTML output with interactive call stack analysis
3. **Caller-Callee Analysis**: Identify which functions are calling the hotspots
4. **Multi-Inclusive Analysis**: Functions appearing multiple times in different call paths
5. **Performance Hotspot Identification**: Automated extraction of top CPU consumers

## System Requirements

### Requirements
- Windows 10/11 with Windows Performance Toolkit installed
- Administrative privileges may be required for ETW profiling
- Python 3.x for automation scripts

### Dependencies
- etwprof.exe (included in `third_party/`)
- xperf.exe (Windows Performance Toolkit)
- Python modules: subprocess, pathlib, datetime, json, csv, re

## Usage Examples

### Quick Performance Analysis
```bash
# Profile all examples and generate analysis report
python Examples/run_examples.py --profile

# Profile specific example  
python Examples/run_examples.py --profile --example simple

# Analyze generated .etl files
python scripts/analyze_profile.py --directory artifacts/profile
```

### Investigation Workflow
1. **Collect Data**: Use `--profile` option to capture performance data
2. **Analyze Results**: Run analysis scripts to generate reports and identify hotspots
3. **Investigate Bottlenecks**: Use butterfly HTML reports for detailed call stack analysis
4. **Implement Optimizations**: Based on identified performance bottlenecks
5. **Validate Results**: Re-profile to measure improvement

## Current Performance Baseline

Performance baseline established from 2025-08-27 profiling data:
- **Sample Range**: 3,917 to 29,767 CPU samples across example categories
- **Primary Bottleneck**: kuzu_shared.dll (44.0% average CPU usage)
- **Secondary Bottleneck**: ntdll.dll (31.4% average CPU usage)

The infrastructure is ready for system call optimization targeting ntdll.dll usage patterns.

---

*Last Updated: 2025-08-27*  
*Status: Operational profiling infrastructure with established performance baseline*