//===--- CompilationDatabaseLoader.h - Load compile_commands.json --------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#pragma once

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "llvm/ADT/StringRef.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <memory>
#include <string>
#include <vector>

namespace clang
{

/// Utility class for loading and validating compilation databases
class CompilationDatabaseLoader
{
public:
    /// Load a compilation database from a file
    /// \param path Path to the compile_commands.json file
    /// \param errorMessage Output parameter for error messages
    /// \return Unique pointer to the loaded database, or nullptr on failure
    static auto loadFromFile(const std::string& path, std::string& errorMessage)
        -> std::unique_ptr<clang::tooling::CompilationDatabase>;

    /// Filter source files from the compilation database
    /// \param db The compilation database to filter
    /// \param pattern Optional pattern to filter files (empty means all files)
    /// \return Vector of source file paths that match the pattern
    static auto filterSourceFiles(const clang::tooling::CompilationDatabase& db, const std::string& pattern = "")
        -> std::vector<std::string>;

private:
    /// Check if a file path matches the given pattern
    /// \param filePath The file path to check
    /// \param pattern The pattern to match against (supports basic wildcards)
    /// \return True if the file matches the pattern
    static auto matchesPattern(llvm::StringRef filePath, llvm::StringRef pattern) -> bool;
};

}  // namespace clang
