//===--- CompilationDatabaseLoader.cpp - Load compile_commands.json ------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#include "CompilationDatabaseLoader.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/Tooling/JSONCompilationDatabase.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>

using namespace clang;
using namespace clang::tooling;

auto CompilationDatabaseLoader::loadFromFile(const std::string& path, std::string& errorMessage)
    -> std::unique_ptr<CompilationDatabase>
{
    // Check if file exists
    if (!llvm::sys::fs::exists(path))
    {
        errorMessage = "File does not exist: " + path;
        return nullptr;
    }

    // Check if file is readable
    if (!llvm::sys::fs::is_regular_file(path))
    {
        errorMessage = "Path is not a regular file: " + path;
        return nullptr;
    }

    // Try to load the compilation database
    std::string loadError;
    auto database = JSONCompilationDatabase::loadFromFile(path, loadError, JSONCommandLineSyntax::AutoDetect);

    if (!database)
    {
        errorMessage = "Failed to load compilation database: " + loadError;
        return nullptr;
    }

    // Validate that the database has some entries
    auto allFiles = database->getAllFiles();
    if (allFiles.empty())
    {
        errorMessage = "Compilation database is empty (no source files found)";
        return nullptr;
    }

    // Success
    errorMessage.clear();
    return database;
}

auto CompilationDatabaseLoader::filterSourceFiles(const CompilationDatabase& db, const std::string& pattern)
    -> std::vector<std::string>
{
    auto allFiles = db.getAllFiles();
    std::vector<std::string> filteredFiles;

    // If no pattern specified, return all files
    if (pattern.empty())
    {
        filteredFiles.reserve(allFiles.size());
        for (const auto& file : allFiles)
        {
            filteredFiles.push_back(file);
        }
        return filteredFiles;
    }

    // Filter files by pattern
    for (const auto& file : allFiles)
    {
        if (matchesPattern(file, pattern))
        {
            filteredFiles.push_back(file);
        }
    }

    return filteredFiles;
}

auto CompilationDatabaseLoader::matchesPattern(llvm::StringRef filePath, llvm::StringRef pattern) -> bool
{
    // Simple pattern matching - supports basic wildcards
    // For now, implement basic substring matching with * wildcards

    if (pattern.empty())
        return true;

    // Handle simple cases
    if (pattern == "*")
        return true;

    // Convert to lowercase for case-insensitive matching on Windows
    std::string lowerFilePath = filePath.lower();
    std::string lowerPattern = pattern.lower();

    // Simple wildcard matching
    if (lowerPattern.front() == '*' && lowerPattern.back() == '*')
    {
        // Pattern like "*substring*"
        std::string substring = lowerPattern.substr(1, lowerPattern.length() - 2);
        return lowerFilePath.find(substring) != std::string::npos;
    }
    if (lowerPattern.front() == '*')
    {
        // Pattern like "*suffix"
        std::string suffix = lowerPattern.substr(1);
        return lowerFilePath.ends_with(suffix);
    }
    if (lowerPattern.back() == '*')
    {
        // Pattern like "prefix*"
        std::string prefix = lowerPattern.substr(0, lowerPattern.length() - 1);
        return lowerFilePath.starts_with(prefix);
    }
    // Exact substring match
    return lowerFilePath.find(lowerPattern) != std::string::npos;
}
