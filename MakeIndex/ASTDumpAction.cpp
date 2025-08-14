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

MakeIndexASTDumpConsumer::MakeIndexASTDumpConsumer(llvm::raw_ostream& OS, ASTContext& Context) : OS(OS)
{
    // Create the KuzuDump instance with the provided context (no colors)
    Dumper = std::make_unique<KuzuDump>(OS, Context, false);
}

void MakeIndexASTDumpConsumer::HandleTranslationUnit(ASTContext& Context)
{
    // Get the translation unit declaration and dump it
    TranslationUnitDecl* TU = Context.getTranslationUnitDecl();

    if (TU != nullptr)
    {
        OS << "=== AST Dump for Translation Unit ===\n";
        Dumper->Visit(TU);
        OS << "\n=== End AST Dump ===\n\n";
    }
    else
    {
        OS << "Error: No translation unit found\n";
    }

    // Flush the output stream to ensure data is written
    OS.flush();
}

MakeIndexASTDumpAction::MakeIndexASTDumpAction(llvm::raw_ostream& OS) : OS(OS)
{
}

auto MakeIndexASTDumpAction::CreateASTConsumer(CompilerInstance& CI, StringRef InFile) -> std::unique_ptr<ASTConsumer>
{
    // Display which file we're processing
    OS << "Processing file: " << InFile << "\n";

    // Create and return the AST consumer (no colors)
    return std::make_unique<MakeIndexASTDumpConsumer>(OS, CI.getASTContext());
}
