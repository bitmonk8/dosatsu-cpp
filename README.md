# CppGraphIndex

CppGraphIndex is a tool that scans C++ projects using Clang and builds a graph database in Kuzu containing parts or all of the Abstract Syntax Tree (AST) of the project. The resulting database assists AI tools in navigating large C++ codebases by enabling natural language queries that are converted to Cypher queries, with results converted back to natural language.

## 🚀 Features

The project consists of two main components:

- **MakeIndex**: Scans C++ projects and builds the graph database
- **MCP**: Implements Natural Language to Cypher query conversion and Result to Natural Language logic, exposed as an MCP (Model Context Protocol) service

## 📋 Requirements

### Dependencies

- **Clang**: For parsing C++ code and generating AST
- **Kuzu**: Graph database for storing the AST data
- **XMake**: Build system (version: latest)
- **C++20 compatible compiler**

### Supported Platforms and Toolchains

CppGraphIndex supports the following platform and toolchain combinations:

- **Windows** with **MSVC** (Microsoft Visual C++)
- **macOS** with **Clang** (Apple Clang or LLVM Clang)
- **Linux** with **GCC** (GNU Compiler Collection)

> **Note**: Other platform/toolchain combinations are not currently supported and will result in build errors. Please ensure you are using one of the supported combinations above.

## 🔧 Building the Project

### Prerequisites

1. Install [XMake](https://xmake.io/#/getting_started?id=installation)
2. Install Clang
3. Install Kuzu database

### Build Commands

```bash
# Configure and build the project
xmake

# Or explicitly build
xmake build
```

### Build Modes

- **Debug Mode** (default): Includes debug symbols, no optimization
- **Release Mode**: Optimized build with symbols stripped

```bash
# Debug build (default)
xmake f -m debug
xmake

# Release build
xmake f -m release
xmake
```

## 🧪 Running Tests

The project uses [doctest](https://github.com/doctest/doctest) for unit testing.

```bash
# Run all tests
xmake test -v

# Or run the executable with selftest flag
xmake run MakeIndex --selftest
```

## 🎯 Usage

> **Note**: This project is in very early development stage. Usage instructions will be expanded as the project matures.

### MakeIndex

```bash
# Build and run MakeIndex
xmake run MakeIndex [options]
```

### MCP Service

*MCP component is planned and not yet implemented.*

## 🛠️ Development

### Code Formatting

Format all source code using clang-format:

```bash
xmake run format
```

### Code Linting

Run clang-tidy on the project:

```bash
# Lint all files
xmake run lint

# Lint specific file
xmake run lint path/to/file.cpp
```

## 🏗️ Project Structure

```
CppGraphIndex/
├── MakeIndex/           # Source code for the indexing tool
├── 3rdParty/           # Third-party dependencies
│   └── include/
│       └── doctest/    # Testing framework
├── .github/            # GitHub Actions workflows
├── xmake.lua          # Build configuration
├── .clang-format      # Code formatting rules
└── .clang-tidy        # Static analysis configuration
```

## 📝 Programming Language

This project is written in **C++20**.

## 📄 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## 🚧 Project Status

**Status**: As early as it can get - just started!

This project is in the very initial stages of development. Contributions, ideas, and feedback are welcome as we build this tool for enhancing AI-assisted C++ code navigation.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Format your code (`xmake run format`)
5. Run linting (`xmake run lint`)
6. Run tests (`xmake test`)
7. Commit your changes (`git commit -m 'Add amazing feature'`)
8. Push to the branch (`git push origin feature/amazing-feature`)
9. Open a Pull Request

## 📞 Support

If you encounter any issues or have questions, please open an issue on GitHub.

---

**Note**: This project aims to bridge the gap between large C++ codebases and AI tools by providing a graph-based representation of code structure that can be queried using natural language. 