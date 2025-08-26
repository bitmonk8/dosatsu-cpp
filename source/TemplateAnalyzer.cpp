//===--- TemplateAnalyzer.cpp - Template processing and analysis --------===//
//
// Part of the Dosatsu project
//
//===----------------------------------------------------------------------===//

#include "TemplateAnalyzer.h"

#include "ASTNodeProcessor.h"
#include "KuzuDatabase.h"

// clang-format off
#include "NoWarningScope_Enter.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>
#include <sstream>

using namespace clang;

TemplateAnalyzer::TemplateAnalyzer(KuzuDatabase& database, ASTNodeProcessor& nodeProcessor, ASTContext& astContext)
    : database(database), nodeProcessor(nodeProcessor)
{
    (void)astContext;  // Unused parameter - kept for future expansion
}

void TemplateAnalyzer::processTemplateDecl(int64_t /*nodeId*/, const clang::TemplateDecl* templateDecl)
{
    if (!database.isInitialized() || (templateDecl == nullptr))
        return;

    // Process template parameters
    if (const auto* templateParams = templateDecl->getTemplateParameters())
        processTemplateParameters(templateParams);

    // Create template metaprogramming information
    int64_t instantiationDepth = extractTemplateInstantiationDepth(templateDecl);
    std::string templateKind;

    if (isa<ClassTemplateDecl>(templateDecl))
        templateKind = "class_template";
    else if (isa<FunctionTemplateDecl>(templateDecl))
        templateKind = "function_template";
    else if (isa<VarTemplateDecl>(templateDecl))
        templateKind = "variable_template";
    else if (isa<TypeAliasTemplateDecl>(templateDecl))
        templateKind = "alias_template";
    else
        templateKind = "unknown_template";

    int64_t metaNodeId = nodeProcessor.getNodeId(templateDecl);
    if (metaNodeId == -1)
        metaNodeId = nodeProcessor.createASTNode(templateDecl);

    if (metaNodeId != -1)
        createTemplateMetaprogrammingNode(metaNodeId, templateDecl, templateKind, instantiationDepth);
}

void TemplateAnalyzer::processTemplateSpecialization(int64_t nodeId, const clang::Decl* specDecl)
{
    if (!database.isInitialized() || (specDecl == nullptr))
        return;

    // Handle class template specializations
    if (const auto* classSpec = dyn_cast<ClassTemplateSpecializationDecl>(specDecl))
    {
        if (const auto* templateDecl = classSpec->getSpecializedTemplate())
        {
            int64_t templateNodeId = nodeProcessor.getNodeId(templateDecl);
            if (templateNodeId == -1)
                templateNodeId = nodeProcessor.createASTNode(templateDecl);

            if (templateNodeId != -1)
            {
                std::string templateArgs = extractTemplateArguments(classSpec->getTemplateArgs());
                std::string instantiationContext = "explicit_specialization";

                if (classSpec->isExplicitSpecialization())
                    instantiationContext = "explicit_specialization";
                else if (classSpec->isExplicitInstantiationOrSpecialization())
                    instantiationContext = "explicit_instantiation";
                else
                    instantiationContext = "implicit_instantiation";

                createSpecializesRelation(
                    nodeId, templateNodeId, "class_specialization", templateArgs, instantiationContext);
            }
        }
    }
    // Handle function template specializations
    else if (const auto* funcDecl = dyn_cast<FunctionDecl>(specDecl))
    {
        if (funcDecl->isFunctionTemplateSpecialization())
        {
            if (const auto* templateDecl = funcDecl->getPrimaryTemplate())
            {
                int64_t templateNodeId = nodeProcessor.getNodeId(templateDecl);
                if (templateNodeId == -1)
                    templateNodeId = nodeProcessor.createASTNode(templateDecl);

                if (templateNodeId != -1)
                {
                    std::string templateArgs = extractTemplateArguments(templateDecl);
                    std::string instantiationContext = "implicit_instantiation";

                    createSpecializesRelation(
                        nodeId, templateNodeId, "function_specialization", templateArgs, instantiationContext);
                }
            }
        }
    }
}

void TemplateAnalyzer::createTemplateParameterNode(int64_t nodeId, const clang::NamedDecl* param)
{
    if (!database.isInitialized() || (param == nullptr))
        return;

    try
    {
        std::string parameterKind;
        std::string parameterName = param->getNameAsString();
        bool hasDefaultArgument = false;
        std::string defaultArgumentText;
        bool isParameterPack = false;

        if (const auto* typeParam = dyn_cast<TemplateTypeParmDecl>(param))
        {
            parameterKind = "type";
            hasDefaultArgument = typeParam->hasDefaultArgument();
            isParameterPack = typeParam->isParameterPack();
            if (hasDefaultArgument && typeParam->hasDefaultArgument())
                defaultArgumentText = "default_type_arg";
        }
        else if (const auto* nonTypeParam = dyn_cast<NonTypeTemplateParmDecl>(param))
        {
            parameterKind = "non_type";
            hasDefaultArgument = nonTypeParam->hasDefaultArgument();
            isParameterPack = nonTypeParam->isParameterPack();
            if (hasDefaultArgument && nonTypeParam->hasDefaultArgument())
                defaultArgumentText = "default_nontype_arg";
        }
        else if (const auto* templateParam = dyn_cast<TemplateTemplateParmDecl>(param))
        {
            parameterKind = "template";
            hasDefaultArgument = templateParam->hasDefaultArgument();
            isParameterPack = templateParam->isParameterPack();
        }
        else
        {
            parameterKind = "unknown";
        }

        // Escape strings for safe database storage
        parameterName = KuzuDatabase::escapeString(parameterName);
        defaultArgumentText = KuzuDatabase::escapeString(defaultArgumentText);

        // Use the provided node ID for the TemplateParameter table
        std::string escapedParameterKind = KuzuDatabase::escapeString(parameterKind);
        std::string query = "CREATE (tp:TemplateParameter {node_id: " + std::to_string(nodeId) + ", parameter_kind: '" +
                            escapedParameterKind + "', parameter_name: '" + parameterName +
                            "', has_default_argument: " + (hasDefaultArgument ? "true" : "false") +
                            ", default_argument_text: '" + defaultArgumentText +
                            "', is_parameter_pack: " + (isParameterPack ? "true" : "false") + "})";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating TemplateParameter node: " << e.what() << "\n";
    }
}

void TemplateAnalyzer::createTemplateRelation(int64_t specializationId, int64_t templateId, const std::string& kind)
{
    if (!database.isInitialized() || specializationId == -1 || templateId == -1)
        return;

    try
    {
        std::string escapedKind = KuzuDatabase::escapeString(kind);
        std::string query = "MATCH (spec:ASTNode {node_id: " + std::to_string(specializationId) + "}), " +
                            "(tmpl:Declaration {node_id: " + std::to_string(templateId) + "}) " +
                            "CREATE (spec)-[:TEMPLATE_RELATION {relation_kind: '" + escapedKind +
                            "', specialization_type: 'explicit'}]->(tmpl)";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating TEMPLATE_RELATION relationship: " << e.what() << "\n";
    }
}

void TemplateAnalyzer::createSpecializesRelation(int64_t specializationId,
                                                 int64_t templateId,
                                                 const std::string& specializationKind,
                                                 const std::string& templateArguments,
                                                 const std::string& instantiationContext)
{
    if (!database.isInitialized() || specializationId == -1 || templateId == -1)
        return;

    try
    {
        // Use proper escaping for database storage
        std::string escapedSpecializationKind = KuzuDatabase::escapeString(specializationKind);
        std::string escapedArgs = KuzuDatabase::escapeString(templateArguments);
        std::string escapedContext = KuzuDatabase::escapeString(instantiationContext);

        std::string query = "MATCH (spec:Declaration {node_id: " + std::to_string(specializationId) + "}), " +
                            "(tmpl:Declaration {node_id: " + std::to_string(templateId) + "}) " +
                            "CREATE (spec)-[:SPECIALIZES {specialization_kind: '" + escapedSpecializationKind +
                            "', template_arguments: '" + escapedArgs + "', instantiation_context: '" + escapedContext +
                            "'}]->(tmpl)";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating SPECIALIZES relationship: " << e.what() << "\n";
    }
}

void TemplateAnalyzer::createTemplateMetaprogrammingNode(int64_t nodeId,
                                                         const clang::Decl* templateDecl,
                                                         const std::string& templateKind,
                                                         int64_t instantiationDepth)
{
    if (!database.isInitialized() || (templateDecl == nullptr))
        return;

    try
    {
        std::string templateArguments = extractTemplateArguments(dyn_cast<TemplateDecl>(templateDecl));
        int64_t specializedTemplateId = -1;
        std::string metaprogramResult = "pending";
        std::string dependentTypes = "";
        std::string substitutionFailureReason = "";

        // Extract more information for specializations
        if (const auto* classTemplateSpecDecl = dyn_cast<ClassTemplateSpecializationDecl>(templateDecl))
        {
            const auto& args = classTemplateSpecDecl->getTemplateArgs();
            templateArguments = extractTemplateArguments(args);
            if (classTemplateSpecDecl->getSpecializedTemplate() != nullptr)
                specializedTemplateId = nodeProcessor.getNodeId(classTemplateSpecDecl->getSpecializedTemplate());
        }

        // Use proper escaping for database storage
        std::string escapedTemplateKind = KuzuDatabase::escapeString(templateKind);
        templateArguments = KuzuDatabase::escapeString(templateArguments);
        std::string escapedMetaprogramResult = KuzuDatabase::escapeString(metaprogramResult);
        dependentTypes = KuzuDatabase::escapeString(dependentTypes);
        substitutionFailureReason = KuzuDatabase::escapeString(substitutionFailureReason);

        std::string query = "CREATE (tm:TemplateMetaprogramming {node_id: " + std::to_string(nodeId) +
                            ", template_kind: '" + escapedTemplateKind +
                            "', instantiation_depth: " + std::to_string(instantiationDepth) +
                            ", template_arguments: '" + templateArguments +
                            "', specialized_template_id: " + std::to_string(specializedTemplateId) +
                            ", metaprogram_result: '" + escapedMetaprogramResult + "', dependent_types: '" +
                            dependentTypes + "', substitution_failure_reason: '" + substitutionFailureReason + "'})";

        database.addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating TemplateMetaprogramming node: " << e.what() << "\n";
    }
}

auto TemplateAnalyzer::extractTemplateArguments(const clang::TemplateArgumentList& args) -> std::string
{
    std::string result;
    for (unsigned i = 0; i < args.size(); ++i)
    {
        if (i > 0)
            result += ", ";

        const TemplateArgument& arg = args[i];
        switch (arg.getKind())
        {
        case TemplateArgument::Type:
        {
            QualType type = arg.getAsType();
            if (type.getTypePtrOrNull() != nullptr && !type.isNull())
                result += type.getAsString();
            else
                result += "?";
        }
        break;
        case TemplateArgument::Integral:
            result += std::to_string(arg.getAsIntegral().getLimitedValue());
            break;
        case TemplateArgument::Declaration:
            if (const auto* decl = arg.getAsDecl())
                if (const auto* namedDecl = dyn_cast<NamedDecl>(decl))
                    result += namedDecl->getNameAsString();
                else
                    result += "unnamed_decl";
            else
                result += "null_decl";
            break;
        case TemplateArgument::Template:
            result += "template";
            break;
        case TemplateArgument::TemplateExpansion:
            result += "template_expansion";
            break;
        case TemplateArgument::Expression:
            result += "expression";
            break;
        case TemplateArgument::Pack:
            result += "parameter_pack";
            break;
        default:
            result += "unknown";
            break;
        }
    }
    return result;
}

auto TemplateAnalyzer::extractTemplateArguments(const clang::TemplateDecl* templateDecl) -> std::string
{
    if (templateDecl == nullptr)
        return "";

    std::string result;
    if (const auto* templateParams = templateDecl->getTemplateParameters())
    {
        for (unsigned i = 0; i < templateParams->size(); ++i)
        {
            if (i > 0)
                result += ", ";

            const NamedDecl* param = templateParams->getParam(i);
            if (const auto* typeParam = dyn_cast<TemplateTypeParmDecl>(param))
                result += "typename " + typeParam->getNameAsString();
            else if (const auto* nonTypeParam = dyn_cast<NonTypeTemplateParmDecl>(param))
                result += (nonTypeParam->getType().isNull() ? "unknown" : nonTypeParam->getType().getAsString()) + " " +
                          nonTypeParam->getNameAsString();
            else if (const auto* templateParam = dyn_cast<TemplateTemplateParmDecl>(param))
                result += "template " + templateParam->getNameAsString();
            else
                result += "unknown_param";
        }
    }
    return result;
}

auto TemplateAnalyzer::extractTemplateInstantiationDepth(const clang::Decl* decl) -> int64_t
{
    if (decl == nullptr)
        return 0;

    int64_t depth = 0;
    const DeclContext* context = decl->getDeclContext();

    while (context != nullptr)
    {
        if (const auto* classTemplateSpec = dyn_cast<ClassTemplateSpecializationDecl>(context))
        {
            depth++;
            // Check if this specialization is nested within another template
            context = classTemplateSpec->getDeclContext();
        }
        else if (const auto* functionDecl = dyn_cast<FunctionDecl>(context))
        {
            if (functionDecl->isFunctionTemplateSpecialization())
                depth++;
            context = functionDecl->getDeclContext();
        }
        else
        {
            context = context->getParent();
        }
    }

    return depth;
}

void TemplateAnalyzer::processTemplateParameters(const clang::TemplateParameterList* templateParams)
{
    if (templateParams == nullptr)
        return;

    for (unsigned i = 0; i < templateParams->size(); ++i)
    {
        const NamedDecl* param = templateParams->getParam(i);
        int64_t paramNodeId = nodeProcessor.getNodeId(param);
        if (paramNodeId == -1)
            paramNodeId = nodeProcessor.createASTNode(param);

        if (paramNodeId != -1)
            createTemplateParameterNode(paramNodeId, param);
    }
}
