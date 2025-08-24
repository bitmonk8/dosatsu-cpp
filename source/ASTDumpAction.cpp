//===--- ASTDumpAction.cpp - AST Frontend Action for dumping ASTs --------===//
//
// Part of the Dosatsu project
//
//===----------------------------------------------------------------------===//

#include "ASTDumpAction.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/Frontend/CompilerInstance.h"
#include "NoWarningScope_Leave.h"
// clang-format on

using namespace clang;

DosatsuASTDumpConsumer::DosatsuASTDumpConsumer(llvm::raw_ostream& OS, ASTContext& Context)
{
    // Create the KuzuDump instance with the provided context (no colors)
    Dumper = std::make_unique<KuzuDump>(OS, Context, false);
}

DosatsuASTDumpConsumer::DosatsuASTDumpConsumer(const std::string& databasePath, ASTContext& Context)
{
    // Create the KuzuDump instance for database output
    Dumper = std::make_unique<KuzuDump>(databasePath, Context, false);
}

void DosatsuASTDumpConsumer::HandleTranslationUnit(ASTContext& Context)
{
    // Process the entire translation unit starting from the translation unit declaration
    Dumper->Visit(Context.getTranslationUnitDecl());
}

// DosatsuASTDumpAction implementations
DosatsuASTDumpAction::DosatsuASTDumpAction(llvm::raw_ostream& OS) : OS(&OS)
{
}

DosatsuASTDumpAction::DosatsuASTDumpAction(std::string databasePath)
    : OS(nullptr), databasePath(std::move(databasePath)), usingDatabase(true)
{
}

auto DosatsuASTDumpAction::CreateASTConsumer(CompilerInstance& CI, StringRef /*InFile*/) -> std::unique_ptr<ASTConsumer>
{
    if (usingDatabase)
        return std::make_unique<DosatsuASTDumpConsumer>(databasePath, CI.getASTContext());
    return std::make_unique<DosatsuASTDumpConsumer>(*OS, CI.getASTContext());
}