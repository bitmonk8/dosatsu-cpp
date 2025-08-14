#define DOCTEST_CONFIG_IMPLEMENT
#include "ASTDumpAction.h"
#include "CompilationDatabaseLoader.h"

#include <doctest/doctest.h>

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// Command line options
static llvm::cl::OptionCategory MakeIndexCategory("MakeIndex Options");

static llvm::cl::opt<std::string> CompileCommandsPath(llvm::cl::Positional,
                                                      llvm::cl::desc("<compile_commands.json>"),
                                                      llvm::cl::Required,
                                                      llvm::cl::cat(MakeIndexCategory));

static llvm::cl::opt<std::string> OutputFile("output",
                                             llvm::cl::desc("Output file (required)"),
                                             llvm::cl::value_desc("filename"),
                                             llvm::cl::Required,
                                             llvm::cl::cat(MakeIndexCategory));

static llvm::cl::opt<std::string>
    FilterPattern("filter",
                  llvm::cl::desc("Filter files by pattern (e.g., \"*MakeIndex*\", default: process all files)"),
                  llvm::cl::value_desc("pattern"),
                  llvm::cl::cat(MakeIndexCategory));

auto RealMain(int argc, char** argv) -> int
{
    // Parse command line arguments
    llvm::cl::SetVersionPrinter(
        [](llvm::raw_ostream& OS)
        {
            OS << "MakeIndex 1.0.0 - AST Dump Tool\n";
            OS << "Built with LLVM/Clang support\n";
        });

    llvm::cl::ParseCommandLineOptions(argc,
                                      argv,
                                      "MakeIndex - C++ AST Dump Tool\n\n"
                                      "This tool reads compile_commands.json files and generates AST dumps\n"
                                      "for the specified source files using Clang's AST parsing capabilities.\n");

    // Validate required arguments
    if (CompileCommandsPath.empty())
    {
        llvm::errs() << "Error: compile_commands.json file path is required\n";
        return 1;
    }

    if (OutputFile.empty())
    {
        llvm::errs() << "Error: output file is required\n";
        return 1;
    }

    // Display parsed options for debugging
    llvm::outs() << "MakeIndex starting with options:\n";
    llvm::outs() << "  Compile commands: " << CompileCommandsPath << "\n";
    llvm::outs() << "  Output file: " << OutputFile << "\n";
    if (!FilterPattern.empty())
    {
        llvm::outs() << "  Filter pattern: " << FilterPattern << "\n";
    }
    else
    {
        llvm::outs() << "  Filter: none (processing all files)\n";
    }
    llvm::outs() << "\n";

    // Load compilation database
    std::string errorMessage;
    auto database = clang::CompilationDatabaseLoader::loadFromFile(CompileCommandsPath, errorMessage);

    if (!database)
    {
        llvm::errs() << "Error loading compilation database: " << errorMessage << "\n";
        return 1;
    }

    llvm::outs() << "Successfully loaded compilation database from: " << CompileCommandsPath << "\n";

    // Filter source files based on command line option
    std::string filterPattern = FilterPattern.empty() ? "*" : FilterPattern.getValue();
    auto sourceFiles = clang::CompilationDatabaseLoader::filterSourceFiles(*database, filterPattern);

    llvm::outs() << "Found " << sourceFiles.size() << " source files";
    if (!FilterPattern.empty())
    {
        llvm::outs() << " matching pattern '" << FilterPattern << "'";
    }
    llvm::outs() << ":\n";

    // Display first few files for verification
    size_t displayCount = std::min(sourceFiles.size(), size_t(10));
    for (size_t i = 0; i < displayCount; ++i)
    {
        llvm::outs() << "  " << (i + 1) << ". " << sourceFiles[i] << "\n";
    }

    if (sourceFiles.size() > displayCount)
    {
        llvm::outs() << "  ... and " << (sourceFiles.size() - displayCount) << " more files\n";
    }
    llvm::outs() << "\n";

    // Check if we have any files to process
    if (sourceFiles.empty())
    {
        llvm::errs() << "Error: No source files found";
        if (!FilterPattern.empty())
        {
            llvm::errs() << " matching pattern '" << FilterPattern << "'";
        }
        llvm::errs() << " in compilation database\n";
        return 1;
    }

    // Setup output file stream (required)
    std::error_code EC;
    auto OutputFileStream = std::make_unique<llvm::raw_fd_ostream>(OutputFile, EC);
    if (EC)
    {
        llvm::errs() << "Error opening output file '" << OutputFile << "': " << EC.message() << "\n";
        return 1;
    }
    llvm::outs() << "Writing AST dump to: " << OutputFile << "\n";

    // Create ClangTool and run AST dumping
    llvm::outs() << "Starting AST processing...\n";

    try
    {
        clang::tooling::ClangTool Tool(*database, sourceFiles);

        // Create a custom factory for our AST dump action
        class MakeIndexASTDumpActionFactory : public clang::tooling::FrontendActionFactory
        {
        private:
            llvm::raw_ostream& OS;

        public:
            MakeIndexASTDumpActionFactory(llvm::raw_ostream& OS) : OS(OS) {}

            auto create() -> std::unique_ptr<clang::FrontendAction> override
            {
                return std::make_unique<clang::MakeIndexASTDumpAction>(OS);
            }
        };

        auto ActionFactory = std::make_unique<MakeIndexASTDumpActionFactory>(*OutputFileStream);

        // Run the tool
        int Result = Tool.run(ActionFactory.get());

        if (Result == 0)
        {
            llvm::outs() << "AST processing completed successfully!\n";
        }
        else
        {
            llvm::errs() << "AST processing completed with errors (exit code: " << Result << ")\n";
        }

        return Result;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception during AST processing: " << e.what() << "\n";
        return 1;
    }
    catch (...)
    {
        llvm::errs() << "Unknown exception during AST processing\n";
        return 1;
    }
}

auto main(int argc, char** argv) -> int
{
    try
    {
        std::vector<std::string> args(argv + 1, argv + argc);

        auto it = std::ranges::find(args, "--selftest");
        bool runTests = it != args.end();

        if (runTests)
        {
            args.erase(it);

            std::vector<const char*> docTestArgv;
            docTestArgv.push_back(argv[0]);
            for (auto& s : args)
                docTestArgv.push_back(s.c_str());

            doctest::Context ctx;
            ctx.applyCommandLine(static_cast<int>(docTestArgv.size()), const_cast<char**>(docTestArgv.data()));
            return ctx.run();
        }

        return RealMain(argc, argv);
    }
    catch (...)
    {
        std::cerr << "An unexpected error occurred in main function" << std::endl;
        return 1;
    }
}

// This is just a small dummy test to make sure we actualy have a test and can verify that the testing system works
// We can remove this test once we start having actual tests
TEST_CASE("Empty dummy test")
{
}
