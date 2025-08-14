//===--- ASTDumpAction.h - AST Frontend Action for dumping ASTs ----------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#pragma once

#include "KuzuDump.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/FrontendAction.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <memory>

namespace clang
{

/// AST Consumer that uses KuzuDump to output AST information
class MakeIndexASTDumpConsumer : public ASTConsumer
{
public:
    /// Constructor
    /// \param OS Output stream for dumping
    /// \param Context AST context for the current compilation unit
    MakeIndexASTDumpConsumer(llvm::raw_ostream& OS, ASTContext& Context);

    /// Handle the translation unit once it's fully parsed
    /// \param Context The AST context for this translation unit
    void HandleTranslationUnit(ASTContext& Context) override;

private:
    std::unique_ptr<KuzuDump> Dumper;
    llvm::raw_ostream& OS;
};

/// Frontend action that creates MakeIndexASTDumpConsumer instances
class MakeIndexASTDumpAction : public ASTFrontendAction
{
public:
    /// Constructor
    /// \param OS Output stream for dumping
    MakeIndexASTDumpAction(llvm::raw_ostream& OS);

protected:
    /// Create the AST consumer for this action
    /// \param CI Compiler instance
    /// \param InFile Input file being processed
    /// \return Unique pointer to the created AST consumer
    auto CreateASTConsumer(CompilerInstance& CI, StringRef InFile) -> std::unique_ptr<ASTConsumer> override;

private:
    llvm::raw_ostream& OS;
};

}  // namespace clang
