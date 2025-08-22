//===--- ASTDumpAction.cpp - AST Frontend Action for dumping ASTs --------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#include "ASTDumpAction.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

using namespace clang;

MakeIndexASTDumpConsumer::MakeIndexASTDumpConsumer(llvm::raw_ostream& OS, ASTContext& Context) : OS(&OS)
{
    // Create the KuzuDump instance with the provided context (no colors)
    Dumper = std::make_unique<KuzuDump>(OS, Context, false);
}

MakeIndexASTDumpConsumer::MakeIndexASTDumpConsumer(const std::string& databasePath, ASTContext& Context)
    : OS(nullptr), usingDatabase(true)
{
    // Create the KuzuDump instance for database output
    Dumper = std::make_unique<KuzuDump>(databasePath, Context, false);
}

void MakeIndexASTDumpConsumer::HandleTranslationUnit(ASTContext& Context)
{
    // Get the translation unit declaration and dump it
    TranslationUnitDecl* TU = Context.getTranslationUnitDecl();

    if (TU != nullptr)
    {
        if (usingDatabase)
        {
            // For database output, just process the AST
            Dumper->Visit(TU);
        }
        else
        {
            // For text output, add headers
            *OS << "=== AST Dump for Translation Unit ===\n";
            Dumper->Visit(TU);
            *OS << "\n=== End AST Dump ===\n\n";
        }
    }
    else
    {
        if (!usingDatabase && OS != nullptr)
            *OS << "Error: No translation unit found\n";
    }

    // Flush the output stream for text output
    if (!usingDatabase && OS != nullptr)
        OS->flush();
}

MakeIndexASTDumpAction::MakeIndexASTDumpAction(llvm::raw_ostream& OS) : OS(&OS)
{
}

MakeIndexASTDumpAction::MakeIndexASTDumpAction(std::string databasePath)
    : OS(nullptr), databasePath(std::move(databasePath)), usingDatabase(true)
{
}

auto MakeIndexASTDumpAction::CreateASTConsumer(CompilerInstance& CI, StringRef InFile) -> std::unique_ptr<ASTConsumer>
{
    if (usingDatabase)
    {
        // For database output, show processing info on standard output
        llvm::outs() << "Processing file: " << InFile << "\n";

        // Create and return the AST consumer for database output
        return std::make_unique<MakeIndexASTDumpConsumer>(databasePath, CI.getASTContext());
    }
    // Display which file we're processing
    *OS << "Processing file: " << InFile << "\n";

    // Create and return the AST consumer for text output
    return std::make_unique<MakeIndexASTDumpConsumer>(*OS, CI.getASTContext());
}
