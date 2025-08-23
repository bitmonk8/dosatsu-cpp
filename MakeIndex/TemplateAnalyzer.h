//===--- TemplateAnalyzer.h - Template processing and analysis ----------===//
//
// Part of the MakeIndex project
//
//===----------------------------------------------------------------------===//

#pragma once

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/TemplateBase.h"
#include "clang/AST/ASTContext.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <cstdint>
#include <string>

namespace clang
{

class KuzuDatabase;
class ASTNodeProcessor;

/// Handles template analysis and processing for AST storage
class TemplateAnalyzer
{
public:
    /// Constructor
    /// \param database Database instance for storage
    /// \param nodeProcessor Node processor for creating basic nodes
    /// \param astContext AST context for template analysis
    TemplateAnalyzer(KuzuDatabase& database, ASTNodeProcessor& nodeProcessor, ASTContext& astContext);

    /// Process template declaration
    /// \param nodeId Node ID for the template
    /// \param templateDecl The template declaration
    void processTemplateDecl(int64_t nodeId, const clang::TemplateDecl* templateDecl);

    /// Process template specialization
    /// \param nodeId Node ID for the specialization
    /// \param specDecl The specialization declaration
    void processTemplateSpecialization(int64_t nodeId, const clang::Decl* specDecl);

    /// Create template parameter node
    /// \param nodeId Node ID for the parameter
    /// \param param Template parameter declaration
    void createTemplateParameterNode(int64_t nodeId, const clang::NamedDecl* param);

    /// Create template relation
    /// \param specializationId Specialization node ID
    /// \param templateId Template node ID
    /// \param kind Relation kind
    void createTemplateRelation(int64_t specializationId, int64_t templateId, const std::string& kind);

    /// Create specializes relation
    /// \param specializationId Specialization node ID
    /// \param templateId Template node ID
    /// \param specializationKind Kind of specialization
    /// \param templateArguments Template arguments
    /// \param instantiationContext Context of instantiation
    void createSpecializesRelation(int64_t specializationId,
                                   int64_t templateId,
                                   const std::string& specializationKind,
                                   const std::string& templateArguments,
                                   const std::string& instantiationContext);

    /// Create template metaprogramming node
    /// \param nodeId Node ID for metaprogramming info
    /// \param templateDecl Template declaration
    /// \param templateKind Kind of template
    /// \param instantiationDepth Instantiation depth
    void createTemplateMetaprogrammingNode(int64_t nodeId,
                                           const clang::Decl* templateDecl,
                                           const std::string& templateKind,
                                           int64_t instantiationDepth);

    /// Extract template arguments
    /// \param args Template argument list
    /// \return String representation of arguments
    auto extractTemplateArguments(const clang::TemplateArgumentList& args) -> std::string;

    /// Extract template arguments from template declaration
    /// \param templateDecl Template declaration
    /// \return String representation of arguments
    auto extractTemplateArguments(const clang::TemplateDecl* templateDecl) -> std::string;

    /// Extract template instantiation depth
    /// \param decl Declaration to analyze
    /// \return Instantiation depth
    auto extractTemplateInstantiationDepth(const clang::Decl* decl) -> int64_t;

    /// Process template parameters
    /// \param templateParams Template parameter list
    void processTemplateParameters(const clang::TemplateParameterList* templateParams);

private:
    KuzuDatabase& database;
    ASTNodeProcessor& nodeProcessor;
};

}  // namespace clang
