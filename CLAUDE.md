# Claude Code Configuration for Dosatsu C++

## Project Overview
Dosatsu C++ is a tool that scans C++ projects using Clang and builds a graph database in Kuzu containing the Abstract Syntax Tree (AST). This helps AI tools navigate large C++ codebases.

## Build Commands
- **Setup**: `python please.py setup`
- **Configure Debug**: `python please.py configure --debug`
- **Build Debug**: `python please.py build --debug`
- **Full Rebuild**: `python please.py rebuild --debug`
- **Run Tests**: `python please.py test`
- **Format Code**: `python please.py format`
- **Lint Code**: `python please.py lint`

## Key Files
- `please.py` - Main build orchestrator
- `source/` - Main C++ source code
- `CMakeLists.txt` - Root CMake configuration
- `Examples/` - C++ examples demonstrating features
- `scripts/` - Build helper scripts

## Development Workflow
1. Run `python please.py format` before making changes
2. Use `python please.py lint` for code quality checks
3. Run `python please.py rebuild` for full build + test
4. Check `python please.py info` for environment details

## Dependencies
- Python 3.8+
- CMake 3.24+
- Ninja build system
- C++20 compatible compiler
- LLVM/Clang 19.1.7 (auto-downloaded)

## Common Tasks
- Generate compile_commands.json: `python please.py compile-db --copy-to-root`
- Clean build: `python please.py clean`
- Build stats: `python please.py build-stats`

## Code Style Guidelines

### Comments
- **Focus on WHY, not WHAT**: Comments should explain the reasoning behind code decisions, not describe what the code does
- **Present-focused**: Avoid comments that reference how code "used to work" or optimization history
- **Avoid historical references**: Comments like "Optimized: increased from X to Y" or "Keep original pattern but..." are not helpful for future developers
- **Good examples**:
  ```cpp
  static constexpr size_t BATCH_SIZE = 150;  // Process this many operations per batch
  // Use connection pooling to reduce connection overhead
  ```
- **Bad examples**:
  ```cpp
  static constexpr size_t BATCH_SIZE = 150;  // Optimized: increased from 25 to 150
  // Keep original query pattern but benefit from increased batch size
  ```
- **Exception**: Temporary comments explaining disabled features are acceptable if they include the reason:
  ```cpp
  // Note: Complex relationship batching disabled due to schema complexity
  ```