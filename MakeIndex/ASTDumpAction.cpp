//===--- ASTDumpAction.cpp - AST Frontend Action for dumping ASTs --------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#include "ASTDumpAction.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/Frontend/CompilerInstance.h"
#include "NoWarningScope_Leave.h"
// clang-format on

using namespace clang;

MakeIndexASTDumpConsumer::MakeIndexASTDumpConsumer(llvm::raw_ostream& OS, ASTContext& Context)
{
    // Create the KuzuDump instance with the provided context (no colors)
    Dumper = std::make_unique<KuzuDump>(OS, Context, false);
}

MakeIndexASTDumpConsumer::MakeIndexASTDumpConsumer(const std::string& databasePath, ASTContext& Context)
{
    // Create the KuzuDump instance for database output
    Dumper = std::make_unique<KuzuDump>(databasePath, Context, false);
}

void MakeIndexASTDumpConsumer::HandleTranslationUnit(ASTContext& Context)
{
    // Process the entire translation unit starting from the translation unit declaration
    Dumper->Visit(Context.getTranslationUnitDecl());
}

// MakeIndexASTDumpAction implementations
MakeIndexASTDumpAction::MakeIndexASTDumpAction(llvm::raw_ostream& OS) : OS(&OS)
{
}

MakeIndexASTDumpAction::MakeIndexASTDumpAction(std::string databasePath)
    : OS(nullptr), databasePath(std::move(databasePath)), usingDatabase(true)
{
}

auto MakeIndexASTDumpAction::CreateASTConsumer(CompilerInstance& CI, StringRef /*InFile*/)
    -> std::unique_ptr<ASTConsumer>
{
    if (usingDatabase)
        return std::make_unique<MakeIndexASTDumpConsumer>(databasePath, CI.getASTContext());
    return std::make_unique<MakeIndexASTDumpConsumer>(*OS, CI.getASTContext());
}