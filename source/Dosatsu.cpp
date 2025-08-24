#include "ASTDumpAction.h"
#include "CompilationDatabaseLoader.h"

// clang-format off
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "NoWarningScope_Enter.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <dbghelp.h>
#include <windows.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// Command line options
static llvm::cl::OptionCategory DosatsuCategory("Dosatsu Options");

static llvm::cl::opt<std::string> CompileCommandsPath(llvm::cl::Positional,
                                                      llvm::cl::desc("<compile_commands.json>"),
                                                      llvm::cl::Required,
                                                      llvm::cl::cat(DosatsuCategory));

static llvm::cl::opt<std::string> OutputFile("output",
                                             llvm::cl::desc("Output file"),
                                             llvm::cl::value_desc("filename"),
                                             llvm::cl::cat(DosatsuCategory));

static llvm::cl::opt<std::string>
    FilterPattern("filter",
                  llvm::cl::desc("Filter files by pattern (e.g., \"*Dosatsu*\", default: process all files)"),
                  llvm::cl::value_desc("pattern"),
                  llvm::cl::cat(DosatsuCategory));

static llvm::cl::opt<std::string> DatabasePath("output-db",
                                               llvm::cl::desc("Output to Kuzu graph database instead of text file"),
                                               llvm::cl::value_desc("database_path"),
                                               llvm::cl::cat(DosatsuCategory));

auto RealMain(int argc, char** argv) -> int
{
    // Parse command line arguments
    llvm::cl::SetVersionPrinter(
        [](llvm::raw_ostream& OS)
        {
            OS << "Dosatsu 1.0.0 - C++ Code Analysis Tool\n";
            OS << "Built with LLVM/Clang support\n";
        });

    llvm::cl::ParseCommandLineOptions(argc,
                                      argv,
                                      "Dosatsu - C++ Code Analysis Tool\n\n"
                                      "This tool reads compile_commands.json files and generates AST dumps\n"
                                      "for the specified source files using Clang's AST parsing capabilities.\n");

    // Validate required arguments
    if (CompileCommandsPath.empty())
    {
        llvm::errs() << "Error: compile_commands.json file path is required\n";
        return 1;
    }

    // Check if database output is specified
    bool useDatabaseOutput = !DatabasePath.empty();
    if (!useDatabaseOutput && OutputFile.empty())
    {
        llvm::errs() << "Error: either --output or --output-db is required\n";
        return 1;
    }
    if (useDatabaseOutput && !OutputFile.empty())
    {
        llvm::errs() << "Error: cannot specify both --output and --output-db\n";
        return 1;
    }

    // Display parsed options for debugging
    llvm::outs() << "Dosatsu starting with options:\n";
    llvm::outs() << "  Compile commands: " << CompileCommandsPath << "\n";
    if (useDatabaseOutput)
        llvm::outs() << "  Database output: " << DatabasePath << "\n";
    else
        llvm::outs() << "  Text output: " << OutputFile << "\n";
    if (!FilterPattern.empty())
        llvm::outs() << "  Filter pattern: " << FilterPattern << "\n";
    else
        llvm::outs() << "  Filter: none (processing all files)\n";
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
    std::string filterPattern;
    if (FilterPattern.empty())
        filterPattern = "*";
    else
        filterPattern = FilterPattern.getValue();
    auto sourceFiles = clang::CompilationDatabaseLoader::filterSourceFiles(*database, filterPattern);

    llvm::outs() << "Found " << sourceFiles.size() << " source files";
    if (!FilterPattern.empty())
        llvm::outs() << " matching pattern '" << FilterPattern << "'";
    llvm::outs() << ":\n";

    // Display first few files for verification
    size_t displayCount = std::min(sourceFiles.size(), size_t(10));
    for (size_t i = 0; i < displayCount; ++i)
        llvm::outs() << "  " << (i + 1) << ". " << sourceFiles[i] << "\n";

    if (sourceFiles.size() > displayCount)
        llvm::outs() << "  ... and " << (sourceFiles.size() - displayCount) << " more files\n";
    llvm::outs() << "\n";

    // Check if we have any files to process
    if (sourceFiles.empty())
    {
        llvm::errs() << "Error: No source files found";
        if (!FilterPattern.empty())
            llvm::errs() << " matching pattern '" << FilterPattern << "'";
        llvm::errs() << " in compilation database\n";
        return 1;
    }

    // Setup output stream (for text output only)
    std::unique_ptr<llvm::raw_fd_ostream> OutputFileStream;
    if (!useDatabaseOutput)
    {
        std::error_code EC;
        OutputFileStream = std::make_unique<llvm::raw_fd_ostream>(OutputFile, EC);
        if (EC)
        {
            llvm::errs() << "Error opening output file '" << OutputFile << "': " << EC.message() << "\n";
            return 1;
        }
        llvm::outs() << "Writing AST dump to: " << OutputFile << "\n";
    }
    else
    {
        llvm::outs() << "Writing AST data to database: " << DatabasePath << "\n";
    }

    // Create ClangTool and run AST dumping
    llvm::outs() << "Starting AST processing...\n";
    try
    {
        clang::tooling::ClangTool Tool(*database, sourceFiles);

        // Create a custom factory for our AST dump action
        class DosatsuASTDumpActionFactory : public clang::tooling::FrontendActionFactory
        {
        private:
            llvm::raw_ostream* OS;
            std::string databasePath;
            bool usingDatabase;

        public:
            // Text output constructor
            DosatsuASTDumpActionFactory(llvm::raw_ostream& OS) : OS(&OS), usingDatabase(false) {}

            // Database output constructor
            DosatsuASTDumpActionFactory(std::string databasePath)
                : OS(nullptr), databasePath(std::move(databasePath)), usingDatabase(true)
            {
            }

            auto create() -> std::unique_ptr<clang::FrontendAction> override
            {
                if (usingDatabase)
                    return std::make_unique<clang::DosatsuASTDumpAction>(databasePath);
                return std::make_unique<clang::DosatsuASTDumpAction>(*OS);
            }
        };

        std::unique_ptr<DosatsuASTDumpActionFactory> ActionFactory;
        if (useDatabaseOutput)
            ActionFactory = std::make_unique<DosatsuASTDumpActionFactory>(DatabasePath);
        else
            ActionFactory = std::make_unique<DosatsuASTDumpActionFactory>(*OutputFileStream);

        // Run the tool
        int Result = Tool.run(ActionFactory.get());

        if (Result == 0)
            llvm::outs() << "AST processing completed successfully!\n";
        else
            llvm::errs() << "AST processing completed with errors (exit code: " << Result << ")\n";

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

#pragma comment(lib, "dbghelp.lib")

void PrintStackTrace()
{
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    SymInitialize(process, nullptr, TRUE);

    CONTEXT context;
    RtlCaptureContext(&context);

    STACKFRAME64 frame;
    memset(&frame, 0, sizeof(frame));

#ifdef _M_X64
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = context.Rip;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrStack.Offset = context.Rsp;
#else
    DWORD machineType = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = context.Eip;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
#endif

    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;

    auto* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + (256 * sizeof(char)), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    IMAGEHLP_LINE64 line;
    DWORD displacement;

    std::cout << "Stack trace:" << std::endl;

    while (StackWalk64(machineType,
                       process,
                       thread,
                       &frame,
                       &context,
                       nullptr,
                       SymFunctionTableAccess64,
                       SymGetModuleBase64,
                       nullptr) != 0)
    {
        if (SymFromAddr(process, frame.AddrPC.Offset, nullptr, symbol) != 0)
            if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &displacement, &line) != 0)
                std::cout << symbol->Name << " - " << line.FileName << ":" << line.LineNumber << std::endl;
            else
                std::cout << symbol->Name << " - address: 0x" << std::hex << symbol->Address << std::dec << std::endl;
        else
            std::cout << "<unknown function> - address: 0x" << std::hex << frame.AddrPC.Offset << std::dec << std::endl;
    }

    free(symbol);

    SymCleanup(process);
}

// Custom assert handler
auto CustomAssertHandler(int reportType, char* message, int* /*returnValue*/) -> int
{
    if (reportType == _CRT_ASSERT)
    {
        // Print the assertion message to console
        std::cerr << "ASSERTION FAILED: " << message << std::endl;

        PrintStackTrace();

        // Terminate the application
        std::abort();
    }

    // Return TRUE to indicate we handled the assertion
    return TRUE;
}

auto main(int argc, char** argv) -> int
{
    _set_error_mode(_OUT_TO_STDERR);

    // Set custom report mode for assertions
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

    // Install custom assert handler
    _CrtSetReportHook(CustomAssertHandler);

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
            ctx.applyCommandLine(static_cast<int>(docTestArgv.size()), docTestArgv.data());
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
