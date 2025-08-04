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
- **Build System**: Choose one of:
  - **XMake**: Traditional build system (version: latest)
  - **Meson**: Modern build system with Conan package management (version: 1.8.3+)
- **C++20 compatible compiler**

### Supported Platforms and Toolchains

CppGraphIndex supports the following platform and toolchain combinations:

- **Windows** with **MSVC** (Microsoft Visual C++)
- **macOS** with **Clang** (Apple Clang or LLVM Clang)
- **Linux** with **GCC** (GNU Compiler Collection)

> **Note**: Other platform/toolchain combinations are not currently supported and will result in build errors. Please ensure you are using one of the supported combinations above.

## 🔧 Building the Project

### Build System Options

CppGraphIndex supports two build systems with complete feature parity:

- **XMake**: Traditional Lua-based build system (established)
- **Meson**: Modern Python-based build system with Conan integration (new)

Choose the build system that best fits your development environment and workflow.

### Option 1: XMake Build (Traditional)

#### Prerequisites

1. Install [XMake](https://xmake.io/#/getting_started?id=installation)
2. Install Clang
3. Install Kuzu database

#### Build Commands

```bash
# Configure and build the project
xmake

# Or explicitly build
xmake build
```

#### Build Modes

```bash
# Debug build (default)
xmake f -m debug
xmake

# Release build
xmake f -m release
xmake
```

### Option 2: Meson Build (Modern)

#### Prerequisites

1. Install [Meson](https://mesonbuild.com/Getting-meson.html) (≥1.8.3)
2. Install [Ninja](https://ninja-build.org/) (≥1.12.0)
3. Install [Conan](https://conan.io/downloads) (≥2.0) for dependency management
4. Ensure you have the correct compiler:
   - **Windows**: MSVC (required for LLVM compatibility)
   - **Linux**: GCC (recommended)
   - **macOS**: Clang (recommended)

#### Quick Start

```bash
# Windows only: Setup MSVC environment (REQUIRED first step)
conanvcvars.bat

# Setup dependencies and build (automated)
python tools/build.py full

# Or step by step:
python tools/setup-deps.py        # Install dependencies
meson setup builddir              # Configure build
ninja -C builddir                 # Build project
```

#### Build Modes

```bash
# Windows only: Setup MSVC environment (REQUIRED first step)
conanvcvars.bat

# Debug build (default)
meson setup builddir --buildtype=debug
ninja -C builddir

# Release build
meson setup builddir_release --buildtype=release
ninja -C builddir_release
```

> **📖 Detailed Documentation**: For comprehensive Meson build instructions, troubleshooting, and advanced features, see [docs/MESON_BUILD.md](docs/MESON_BUILD.md)

## 🧪 Running Tests

The project uses [doctest](https://github.com/doctest/doctest) for unit testing.

### XMake Tests

```bash
# Run all tests
xmake test -v

# Or run the executable with selftest flag
xmake run MakeIndex --selftest
```

### Meson Tests

```bash
# Run all tests
meson test -C builddir

# Or run the executable with selftest flag
./builddir/MakeIndex/makeindex_exe --selftest
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

#### XMake
```bash
xmake run format
```

#### Meson
```bash
ninja -C builddir format
# Or directly: python tools/format.py
```

### Code Linting

Run clang-tidy on the project:

#### XMake
```bash
# Lint all files
xmake run lint

# Lint specific file
xmake run lint path/to/file.cpp
```

#### Meson
```bash
ninja -C builddir lint
# Or directly: python tools/lint.py
```

### Development Workflows

Both build systems support comprehensive development workflows:

#### XMake Workflow
```bash
xmake            # Build
xmake test       # Test
xmake run format # Format
xmake run lint   # Lint
```

#### Meson Workflow
```bash
ninja -C builddir        # Build
meson test -C builddir   # Test
ninja -C builddir format # Format
ninja -C builddir lint   # Lint
```

For Meson development convenience scripts, see [docs/MESON_BUILD.md#development-scripts](docs/MESON_BUILD.md#development-scripts)

## 🏗️ Project Structure

```
CppGraphIndex/
├── MakeIndex/           # Source code for the indexing tool
├── 3rdParty/           # Third-party dependencies (XMake)
│   └── include/
│       └── doctest/    # Testing framework
├── tools/              # Build and development tools (Meson)
├── scripts/            # Development convenience scripts (Meson)
├── docs/               # Documentation
│   ├── MESON_BUILD.md  # Detailed Meson build guide
│   └── MIGRATION_GUIDE.md # XMake to Meson migration
├── conan/              # Conan profiles and configuration (Meson)
├── .github/            # GitHub Actions workflows
├── xmake.lua          # XMake build configuration
├── meson.build        # Meson build configuration
├── conanfile.py       # Conan dependencies (Meson)
├── .clang-format      # Code formatting rules
└── .clang-tidy        # Static analysis configuration
```

## 📝 Programming Language

This project is written in **C++20**.

## 🔄 Build System Comparison

Both build systems provide complete feature parity:

| Feature | XMake | Meson |
|---------|-------|-------|
| Build Speed | Fast | Fast (with Ninja) |
| Dependency Management | Built-in | Conan integration |
| Cross-platform | ✅ | ✅ |
| C++20 Support | ✅ | ✅ |
| Development Tools | ✅ | ✅ |
| IDE Integration | Good | Excellent |
| Package Management | Manual | Automated (Conan) |
| Learning Curve | Moderate | Gentle |

### When to Choose XMake
- You prefer Lua-based configuration
- You want minimal external dependencies
- You're familiar with existing XMake workflow

### When to Choose Meson
- You prefer Python-based tooling
- You want automated dependency management
- You need advanced IDE integration
- You're starting a new development setup

> **📖 Migration Guide**: To migrate from XMake to Meson, see [docs/MIGRATION_GUIDE.md](docs/MIGRATION_GUIDE.md)

## 📄 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## 🚧 Project Status

**Status**: As early as it can get - just started!

This project is in the very initial stages of development. Contributions, ideas, and feedback are welcome as we build this tool for enhancing AI-assisted C++ code navigation.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Format your code:
   - XMake: `xmake run format`
   - Meson: `ninja -C builddir format`
5. Run linting:
   - XMake: `xmake run lint`
   - Meson: `ninja -C builddir lint`
6. Run tests:
   - XMake: `xmake test`
   - Meson: `meson test -C builddir`
7. Commit your changes (`git commit -m 'Add amazing feature'`)
8. Push to the branch (`git push origin feature/amazing-feature`)
9. Open a Pull Request

> **Note**: Both build systems are supported for development. Choose the one that fits your workflow best.

## 📞 Support

If you encounter any issues or have questions, please open an issue on GitHub.

---

**Note**: This project aims to bridge the gap between large C++ codebases and AI tools by providing a graph-based representation of code structure that can be queried using natural language. 