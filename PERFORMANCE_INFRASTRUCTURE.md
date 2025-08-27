# Performance Infrastructure

This document describes the performance profiling tools and workflows that are available for analyzing Dosatsu performance.

## Profiling Tools

### Primary Profiling Tool: etwprof

- **Location**: `third_party\etwprof_0.3_release\etwprof.exe`
- **Description**: Lightweight, self-contained sampling profiler for native Windows applications
- **Technology**: Based on Event Tracing for Windows (ETW) framework
- **Documentation**: [etwprof GitHub Repository](https://github.com/Donpedro13/etwprof)
- **Output**: Generates `.etl` files with filtered sampling data

### Analysis Tool: xperf

- **Location**: `C:\Program Files (x86)\Windows Kits\10\Windows Performance Toolkit`
- **Purpose**: Process `.etl` files to identify application hotspots
- **Documentation**: [Xperf Command-Line Reference](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/xperf-command-line-reference)
- **Key Features**: 
  - CPU sampling analysis
  - Call stack butterfly view (`-a stack -butterfly`)
  - Module-level performance breakdown

## Automation Scripts

### Profiling Automation: `scripts/profile.py`

- **Function**: Run arbitrary commands with profiling enabled
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

- **New Options**:
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

# Generate comparison reports (planned)
python scripts/analyze_profile.py --compare before.etl after.etl
```

## Output Files Generated

### From Profiling (`artifacts/profile/`)
- `dosatsu_profile_[example]_[timestamp].etl` - Raw ETW profiling data

### From Analysis (`artifacts/profile/analysis/`)
- `*_butterfly.html` - Interactive call stack analysis with xperf butterfly view
- `*_summary.txt` - CPU sampling summary by xperf
- `*_stacks.csv` - Legacy stack data (for compatibility)
- `*_hotspots.json` - Automated hotspot analysis results
- `performance_report_*.md` - Executive summary with recommendations

## Enhanced Analysis Features

**Capabilities with `xperf -a stack -butterfly`:**

1. **Call Stack Butterfly View**: Visual representation of function call relationships
2. **Detailed HTML Reports**: Rich HTML output with interactive call stack analysis
3. **Caller-Callee Analysis**: Identify which functions are calling the hotspots
4. **Multi-Inclusive Analysis**: Functions appearing multiple times in different call paths
5. **Performance Hotspot Identification**: Automated extraction of top CPU consumers

## Workflow Integration

### Development Process
- Profiling capabilities integrated into existing Examples infrastructure
- Automated performance testing as part of validation pipeline
- Performance metrics tracked alongside functional correctness

### Test Scenarios
- **Primary Test Suite**: Current Examples dataset
- **Rationale**: Provides realistic workloads that represent actual Dosatsu usage patterns
- **Coverage**: Various C++ code patterns and complexity levels

## Technical Requirements

### System Requirements
- Windows 10/11 with Windows Performance Toolkit installed
- Administrative privileges may be required for ETW profiling
- Python 3.x for automation scripts

### Dependencies
- etwprof.exe (included in `third_party/`)
- xperf.exe (Windows Performance Toolkit)
- Python modules: subprocess, pathlib, datetime, json, csv, re

---

*Last Updated: 2025-08-26*

