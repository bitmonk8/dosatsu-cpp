//===--- KuzuDump.cpp - Dumping implementation for ASTs ------------------===//
//
// Based on LLVM Project's ASTDumper, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the AST dump methods, which dump out the
// AST in a form that exposes type details and other fields.
//
//===----------------------------------------------------------------------===//

#include "KuzuDump.h"

// clang-format off
#include "NoWarningScope_Enter.h"

#include "clang/AST/ASTConcept.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclLookups.h"
#include "clang/AST/JSONNodeDumper.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RawCommentList.h"
#include "clang/AST/Comment.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Analysis/CFG.h"
#include "llvm/Support/raw_ostream.h"

#include "kuzu.hpp"

#include "NoWarningScope_Leave.h"
// clang-format on

#include <algorithm>
#include <filesystem>
#include <ranges>
#include <sstream>
#include <stdexcept>

using namespace clang;
using namespace clang::comments;

void KuzuDump::dumpInvalidDeclContext(const DeclContext* DC)
{
    // Phase 4.3: Skip text dumping in database-only mode
    if (databaseOnlyMode)
    {
        return;
    }

    NodeDumper.AddChild(
        [=]
        {
            if (!DC)
            {
                ColorScope Color(OS, ShowColors, NullColor);
                OS << "<<<NULL>>>";
                return;
            }
            // An invalid DeclContext is one for which a dyn_cast() from a DeclContext
            // pointer to a Decl pointer would fail an assertion or otherwise fall prey
            // to undefined behavior as a result of an invalid associated DeclKind.
            // Such invalidity is not supposed to happen of course, but, when it does,
            // the information provided below is intended to provide some hints about
            // what might have gone awry.
            {
                ColorScope Color(OS, ShowColors, DeclKindNameColor);
                OS << "DeclContext";
            }
            NodeDumper.dumpPointer(DC);
            OS << " <";
            {
                ColorScope Color(OS, ShowColors, DeclNameColor);
                OS << "unrecognized Decl kind " << (unsigned)DC->getDeclKind();
            }
            OS << ">";
        });
}

void KuzuDump::dumpLookups(const DeclContext* DC, bool DumpDecls)
{
    // Phase 4.3: Skip text dumping in database-only mode
    if (databaseOnlyMode)
    {
        return;
    }

    NodeDumper.AddChild(
        [=]
        {
            OS << "StoredDeclsMap ";
            NodeDumper.dumpBareDeclRef(cast<Decl>(DC));

            const DeclContext* Primary = DC->getPrimaryContext();
            if (Primary != DC)
            {
                OS << " primary";
                NodeDumper.dumpPointer(cast<Decl>(Primary));
            }

            bool HasUndeserializedLookups = Primary->hasExternalVisibleStorage();

            auto Range =
                getDeserialize() ? Primary->lookups() : Primary->noload_lookups(/*PreserveInternalState=*/true);
            for (auto I = Range.begin(), E = Range.end(); I != E; ++I)
            {
                DeclarationName Name = I.getLookupName();
                DeclContextLookupResult R = *I;

                NodeDumper.AddChild(
                    [=]
                    {
                        OS << "DeclarationName ";
                        {
                            ColorScope Color(OS, ShowColors, DeclNameColor);
                            OS << '\'' << Name << '\'';
                        }

                        for (auto lookupResult : R)
                        {
                            NodeDumper.AddChild(
                                [=]
                                {
                                    NodeDumper.dumpBareDeclRef(lookupResult);

                                    if (!lookupResult->isUnconditionallyVisible())
                                        OS << " hidden";

                                    // If requested, dump the redecl chain for this lookup.
                                    if (DumpDecls)
                                    {
                                        // Dump earliest decl first.
                                        std::function<void(Decl*)> DumpWithPrev = [&](Decl* D)
                                        {
                                            if (Decl* Prev = D->getPreviousDecl())
                                                DumpWithPrev(Prev);
                                            Visit(D);
                                        };
                                        DumpWithPrev(lookupResult);
                                    }
                                });
                        }
                    });
            }

            if (HasUndeserializedLookups)
            {
                NodeDumper.AddChild(
                    [=]
                    {
                        ColorScope Color(OS, ShowColors, UndeserializedColor);
                        OS << "<undeserialized lookups>";
                    });
            }
        });
}

template <typename SpecializationDecl>
void KuzuDump::dumpTemplateDeclSpecialization(const SpecializationDecl* D, bool DumpExplicitInst, bool DumpRefOnly)
{
    // Phase 4.3: Skip text dumping in database-only mode but still process AST
    if (databaseOnlyMode)
    {
        for (const auto* RedeclWithBadType : D->redecls())
        {
            auto* Redecl = cast<SpecializationDecl>(RedeclWithBadType);
            switch (Redecl->getTemplateSpecializationKind())
            {
            case TSK_ExplicitInstantiationDeclaration:
            case TSK_ExplicitInstantiationDefinition:
                if (!DumpExplicitInst)
                    break;
                [[fallthrough]];
            case TSK_Undeclared:
            case TSK_ImplicitInstantiation:
                if (!DumpRefOnly)
                    Visit(Redecl);
                break;
            case TSK_ExplicitSpecialization:
                break;
            }
        }
        return;
    }

    bool DumpedAny = false;
    for (const auto* RedeclWithBadType : D->redecls())
    {
        // FIXME: The redecls() range sometimes has elements of a less-specific
        // type. (In particular, ClassTemplateSpecializationDecl::redecls() gives
        // us TagDecls, and should give CXXRecordDecls).
        auto* Redecl = cast<SpecializationDecl>(RedeclWithBadType);
        switch (Redecl->getTemplateSpecializationKind())
        {
        case TSK_ExplicitInstantiationDeclaration:
        case TSK_ExplicitInstantiationDefinition:
            if (!DumpExplicitInst)
                break;
            [[fallthrough]];
        case TSK_Undeclared:
        case TSK_ImplicitInstantiation:
            if (DumpRefOnly)
                NodeDumper.dumpDeclRef(Redecl);
            else
                Visit(Redecl);
            DumpedAny = true;
            break;
        case TSK_ExplicitSpecialization:
            break;
        }
    }

    // Ensure we dump at least one decl for each specialization.
    if (!DumpedAny)
        NodeDumper.dumpDeclRef(D);
}

template <typename TemplateDecl>
void KuzuDump::dumpTemplateDecl(const TemplateDecl* D, bool DumpExplicitInst)
{
    dumpTemplateParameters(D->getTemplateParameters());

    Visit(D->getTemplatedDecl());

    if (GetTraversalKind() == TK_AsIs)
    {
        for (const auto* Child : D->specializations())
            dumpTemplateDeclSpecialization(Child, DumpExplicitInst, !D->isCanonicalDecl());
    }
}

void KuzuDump::dumpTemplateParameters(const clang::TemplateParameterList* templateParams)
{
    if (templateParams == nullptr)
    {
        return;
    }

    for (const auto* param : *templateParams)
    {
        if (param == nullptr)
        {
            continue;
        }

        // Create database node for this template parameter
        int64_t nodeId = createASTNode(param);

        // Create the template parameter-specific node data
        createTemplateParameterNode(nodeId, param);
    }
}

void KuzuDump::VisitFunctionTemplateDecl(const FunctionTemplateDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this function template declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this template (it's in its parent scope)
    createScopeRelationships(nodeId);

    // Create template metaprogramming information for this function template
    int64_t metaNodeId = nextNodeId++;
    int64_t instantiationDepth = extractTemplateInstantiationDepth(D);
    createTemplateMetaprogrammingNode(metaNodeId, D, "function_template", instantiationDepth);
    createTemplateEvaluationRelation(nodeId, metaNodeId, D->getQualifiedNameAsString());

    // Push this template as a new scope for its contents
    pushScope(nodeId);

    // FIXME: We don't add a declaration of a function template specialization
    // to its context when it's explicitly instantiated, so dump explicit
    // instantiations when we dump the template itself.
    dumpTemplateDecl(D, true);

    // Pop this template scope after traversal
    popScope();
}

void KuzuDump::VisitClassTemplateDecl(const ClassTemplateDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this class template declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this template (it's in its parent scope)
    createScopeRelationships(nodeId);

    // Push this template as a new scope for its contents
    pushScope(nodeId);

    dumpTemplateDecl(D, false);

    // Pop this template scope after traversal
    popScope();
}

void KuzuDump::VisitVarTemplateDecl(const VarTemplateDecl* D)
{
    dumpTemplateDecl(D, false);
}

void KuzuDump::VisitClassTemplateSpecializationDecl(const ClassTemplateSpecializationDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this class template specialization
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this specialization (it's in its parent scope)
    createScopeRelationships(nodeId);

    // Handle template specialization relationships
    if (const ClassTemplateDecl* templateDecl = D->getSpecializedTemplate())
    {
        // Check if the template declaration has been processed
        auto it = nodeIdMap.find(templateDecl);
        int64_t templateNodeId;
        if (it != nodeIdMap.end())
        {
            templateNodeId = it->second;
        }
        else
        {
            // Create the template node if it doesn't exist yet
            templateNodeId = createASTNode(templateDecl);
        }

        // Create both the legacy TEMPLATE_RELATION and enhanced SPECIALIZES relation
        createTemplateRelation(nodeId, templateNodeId, "specializes");

        // Enhanced specialization tracking with template arguments
        std::string specializationKind;
        switch (D->getTemplateSpecializationKind())
        {
        case clang::TSK_ImplicitInstantiation:
            specializationKind = "implicit";
            break;
        case clang::TSK_ExplicitInstantiationDeclaration:
            specializationKind = "explicit_declaration";
            break;
        case clang::TSK_ExplicitInstantiationDefinition:
            specializationKind = "explicit_definition";
            break;
        case clang::TSK_ExplicitSpecialization:
            specializationKind = "explicit_specialization";
            break;
        default:
            specializationKind = "unknown";
            break;
        }

        std::string templateArguments = extractTemplateArguments(D->getTemplateArgs());
        std::string instantiationContext = D->getQualifiedNameAsString();

        createSpecializesRelation(nodeId, templateNodeId, specializationKind, templateArguments, instantiationContext);

        // Create template metaprogramming information for this specialization
        int64_t metaNodeId = nextNodeId++;
        int64_t instantiationDepth = extractTemplateInstantiationDepth(D);
        createTemplateMetaprogrammingNode(metaNodeId, D, "class_template_specialization", instantiationDepth);
        createTemplateEvaluationRelation(nodeId, metaNodeId, instantiationContext);
    }

    // Push this specialization as a new scope for its contents
    pushScope(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this specialization scope after traversal
    popScope();
}

void KuzuDump::VisitClassTemplatePartialSpecializationDecl(const ClassTemplateSpecializationDecl* D)
{
    if (D == nullptr)
        return;

    // For partial specializations, we use the same logic as full specializations
    // but we mark them as "partial" specializations
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this partial specialization (it's in its parent scope)
    createScopeRelationships(nodeId);

    // Handle template specialization relationships for partial specializations
    if (const ClassTemplateDecl* templateDecl = D->getSpecializedTemplate())
    {
        // Check if the template declaration has been processed
        auto it = nodeIdMap.find(templateDecl);
        int64_t templateNodeId;
        if (it != nodeIdMap.end())
        {
            templateNodeId = it->second;
        }
        else
        {
            // Create the template node if it doesn't exist yet
            templateNodeId = createASTNode(templateDecl);
        }

        // Create both the legacy TEMPLATE_RELATION and enhanced SPECIALIZES relation
        createTemplateRelation(nodeId, templateNodeId, "specializes");

        // For partial specializations, we always mark them as "partial"
        std::string specializationKind = "partial";
        std::string templateArguments = extractTemplateArguments(D->getTemplateArgs());
        std::string instantiationContext = D->getQualifiedNameAsString();

        createSpecializesRelation(nodeId, templateNodeId, specializationKind, templateArguments, instantiationContext);
    }

    // Push this partial specialization as a new scope for its contents
    pushScope(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this partial specialization scope after traversal
    popScope();
}

void KuzuDump::VisitStaticAssertDecl(const StaticAssertDecl* D)
{
    if (D == nullptr)
        return;

    int64_t nodeId = createASTNode(D);

    // Create the static assertion node with evaluation information
    createStaticAssertionNode(nodeId, D);

    // Create scope relationships for this static assertion
    createScopeRelationships(nodeId);

    // If this static assertion is within a class or function, create the containment relationship
    if (const auto* parentDecl = dyn_cast<NamedDecl>(D->getDeclContext()))
    {
        auto it = nodeIdMap.find(parentDecl);
        if (it != nodeIdMap.end())
        {
            createStaticAssertRelation(it->second, nodeId, "local_scope");
        }
    }

    // The ASTNodeTraverser will handle automatic traversal of the assertion expression
}

void KuzuDump::initializeDatabase()
{
    if (!databaseEnabled || databasePath.empty())
    {
        return;
    }

    try
    {
        // Create database directory if it doesn't exist
        std::filesystem::path dbPath(databasePath);
        if (dbPath.has_parent_path())
        {
            std::filesystem::create_directories(dbPath.parent_path());
        }

        // Initialize Kuzu database
        database = std::make_unique<kuzu::main::Database>(databasePath);
        connection = std::make_unique<kuzu::main::Connection>(database.get());

        // Create schema
        createSchema();
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to initialize Kuzu database: " + std::string(e.what()));
    }
}

void KuzuDump::executeSchemaQuery(const std::string& query, const std::string& schemaName)
{
    auto result = connection->query(query);
    if (!result->isSuccess())
    {
        throw std::runtime_error("Failed to create " + schemaName + " table: " + result->getErrorMessage());
    }
}

// Performance optimization methods (Phase 4)
void KuzuDump::beginTransaction()
{
    if (!connection || transactionActive)
    {
        return;
    }

    try
    {
        auto result = connection->query("BEGIN TRANSACTION");
        if (result->isSuccess())
        {
            transactionActive = true;
        }
        else
        {
            llvm::errs() << "Failed to begin transaction: " << result->getErrorMessage() << "\n";
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception beginning transaction: " << e.what() << "\n";
    }
}

void KuzuDump::commitTransaction()
{
    if (!connection || !transactionActive)
    {
        return;
    }

    try
    {
        auto result = connection->query("COMMIT");
        if (result->isSuccess())
        {
            transactionActive = false;
        }
        else
        {
            llvm::errs() << "Failed to commit transaction: " << result->getErrorMessage() << "\n";
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception committing transaction: " << e.what() << "\n";
    }
}

void KuzuDump::rollbackTransaction()
{
    if (!connection || !transactionActive)
    {
        return;
    }

    try
    {
        auto result = connection->query("ROLLBACK");
        if (result->isSuccess())
        {
            transactionActive = false;
        }
        else
        {
            llvm::errs() << "Failed to rollback transaction: " << result->getErrorMessage() << "\n";
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception rolling back transaction: " << e.what() << "\n";
    }
}

void KuzuDump::addToBatch(const std::string& query)
{
    if (!connection || query.empty())
    {
        return;
    }

    pendingQueries.push_back(query);
    totalOperations++;

    // Start transaction on first batched operation
    if (!transactionActive)
    {
        beginTransaction();
    }

    // Execute batch when it reaches the batch size
    if (pendingQueries.size() >= BATCH_SIZE)
    {
        executeBatch();
    }
}

void KuzuDump::executeBatch()
{
    if (!connection || pendingQueries.empty())
    {
        return;
    }

    try
    {
        // Execute all queries in the batch
        for (const auto& query : pendingQueries)
        {
            auto result = connection->query(query);
            if (!result->isSuccess())
            {
                llvm::errs() << "Batched query failed: " << result->getErrorMessage() << "\n";
                llvm::errs() << "Query: " << query << "\n";
                // Continue with other queries rather than failing completely
            }
        }

        pendingQueries.clear();
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception executing batch: " << e.what() << "\n";
        pendingQueries.clear();
    }
}

void KuzuDump::flushOperations()
{
    if (!connection)
    {
        return;
    }

    // Execute any remaining batched operations
    if (!pendingQueries.empty())
    {
        executeBatch();
    }

    // Commit any active transaction
    if (transactionActive)
    {
        commitTransaction();
    }
}

void KuzuDump::createSchema()
{
    if (!connection)
    {
        return;
    }

    try
    {
        // Create base ASTNode table
        executeSchemaQuery("CREATE NODE TABLE ASTNode("
                           "node_id INT64 PRIMARY KEY, "
                           "node_type STRING, "
                           "memory_address STRING, "
                           "source_file STRING, "
                           "start_line INT64, "
                           "start_column INT64, "
                           "end_line INT64, "
                           "end_column INT64, "
                           "is_implicit BOOLEAN, "
                           "raw_text STRING)",
                           "ASTNode");

        // Create Declaration table
        executeSchemaQuery("CREATE NODE TABLE Declaration("
                           "node_id INT64 PRIMARY KEY, "
                           "name STRING, "
                           "qualified_name STRING, "
                           "access_specifier STRING, "
                           "storage_class STRING, "
                           "is_definition BOOLEAN, "
                           "namespace_context STRING)",
                           "Declaration");

        // Create Type table
        executeSchemaQuery("CREATE NODE TABLE Type("
                           "node_id INT64 PRIMARY KEY, "
                           "type_name STRING, "
                           "canonical_type STRING, "
                           "size_bytes INT64, "
                           "is_const BOOLEAN, "
                           "is_volatile BOOLEAN, "
                           "is_builtin BOOLEAN)",
                           "Type");

        // Create enhanced Statement table
        executeSchemaQuery("CREATE NODE TABLE Statement("
                           "node_id INT64 PRIMARY KEY, "
                           "statement_kind STRING, "
                           "has_side_effects BOOLEAN, "
                           "is_compound BOOLEAN, "
                           "control_flow_type STRING, "
                           "condition_text STRING, "
                           "is_constexpr BOOLEAN)",
                           "Statement");

        // Create enhanced Expression table
        executeSchemaQuery("CREATE NODE TABLE Expression("
                           "node_id INT64 PRIMARY KEY, "
                           "expression_kind STRING, "
                           "value_category STRING, "
                           "literal_value STRING, "
                           "operator_kind STRING, "
                           "is_constexpr BOOLEAN, "
                           "evaluation_result STRING, "
                           "implicit_cast_kind STRING)",
                           "Expression");

        // Create Attribute table
        executeSchemaQuery("CREATE NODE TABLE Attribute("
                           "node_id INT64 PRIMARY KEY, "
                           "attribute_kind STRING, "
                           "attribute_value STRING)",
                           "Attribute");

        // Create TemplateParameter table for enhanced template system
        executeSchemaQuery("CREATE NODE TABLE TemplateParameter("
                           "node_id INT64 PRIMARY KEY, "
                           "parameter_kind STRING, "
                           "parameter_name STRING, "
                           "has_default_argument BOOLEAN, "
                           "default_argument_text STRING, "
                           "is_parameter_pack BOOLEAN)",
                           "TemplateParameter");

        // Create UsingDeclaration table for namespace and using declarations
        executeSchemaQuery("CREATE NODE TABLE UsingDeclaration("
                           "node_id INT64 PRIMARY KEY, "
                           "using_kind STRING, "
                           "target_name STRING, "
                           "introduces_name STRING, "
                           "scope_impact STRING)",
                           "UsingDeclaration");

        // Create relationship tables
        executeSchemaQuery("CREATE REL TABLE PARENT_OF("
                           "FROM ASTNode TO ASTNode, "
                           "child_index INT64, "
                           "relationship_kind STRING)",
                           "PARENT_OF");

        executeSchemaQuery("CREATE REL TABLE HAS_TYPE("
                           "FROM Declaration TO Type, "
                           "type_role STRING)",
                           "HAS_TYPE");

        executeSchemaQuery("CREATE REL TABLE REFERENCES("
                           "FROM ASTNode TO Declaration, "
                           "reference_kind STRING, "
                           "is_direct BOOLEAN)",
                           "REFERENCES");

        executeSchemaQuery("CREATE REL TABLE IN_SCOPE("
                           "FROM ASTNode TO Declaration, "
                           "scope_kind STRING)",
                           "IN_SCOPE");

        executeSchemaQuery("CREATE REL TABLE TEMPLATE_RELATION("
                           "FROM ASTNode TO Declaration, "
                           "relation_kind STRING, "
                           "specialization_type STRING)",
                           "TEMPLATE_RELATION");

        // Inheritance relationship tables
        executeSchemaQuery("CREATE REL TABLE INHERITS_FROM("
                           "FROM Declaration TO Declaration, "
                           "inheritance_type STRING, "
                           "is_virtual BOOLEAN, "
                           "base_access_path STRING)",
                           "INHERITS_FROM");

        executeSchemaQuery("CREATE REL TABLE OVERRIDES("
                           "FROM Declaration TO Declaration, "
                           "override_type STRING, "
                           "is_covariant_return BOOLEAN)",
                           "OVERRIDES");

        // Enhanced template relationship table for detailed specializations
        executeSchemaQuery("CREATE REL TABLE SPECIALIZES("
                           "FROM Declaration TO Declaration, "
                           "specialization_kind STRING, "
                           "template_arguments STRING, "
                           "instantiation_context STRING)",
                           "SPECIALIZES");

        // Preprocessor and Macro Information tables
        executeSchemaQuery("CREATE NODE TABLE MacroDefinition("
                           "node_id INT64 PRIMARY KEY, "
                           "macro_name STRING, "
                           "is_function_like BOOLEAN, "
                           "parameter_count INT64, "
                           "parameter_names STRING, "
                           "replacement_text STRING, "
                           "is_builtin BOOLEAN, "
                           "is_conditional BOOLEAN)",
                           "MacroDefinition");

        executeSchemaQuery("CREATE NODE TABLE IncludeDirective("
                           "node_id INT64 PRIMARY KEY, "
                           "include_path STRING, "
                           "is_system_include BOOLEAN, "
                           "is_angled BOOLEAN, "
                           "resolved_path STRING, "
                           "include_depth INT64)",
                           "IncludeDirective");

        executeSchemaQuery("CREATE NODE TABLE ConditionalDirective("
                           "node_id INT64 PRIMARY KEY, "
                           "directive_type STRING, "
                           "condition_text STRING, "
                           "is_true_branch BOOLEAN, "
                           "nesting_level INT64)",
                           "ConditionalDirective");

        executeSchemaQuery("CREATE NODE TABLE PragmaDirective("
                           "node_id INT64 PRIMARY KEY, "
                           "pragma_name STRING, "
                           "pragma_text STRING, "
                           "pragma_kind STRING)",
                           "PragmaDirective");

        // Create Comment table for documentation and comments
        executeSchemaQuery("CREATE NODE TABLE Comment("
                           "node_id INT64 PRIMARY KEY, "
                           "comment_text STRING, "
                           "comment_kind STRING, "
                           "is_documentation BOOLEAN, "
                           "brief_text STRING, "
                           "detailed_text STRING)",
                           "Comment");

        // Create ConstantExpression table for compile-time evaluation tracking
        executeSchemaQuery("CREATE NODE TABLE ConstantExpression("
                           "node_id INT64 PRIMARY KEY, "
                           "is_constexpr_function BOOLEAN, "
                           "evaluation_context STRING, "
                           "evaluation_result STRING, "
                           "result_type STRING, "
                           "is_compile_time_constant BOOLEAN, "
                           "constant_value STRING, "
                           "constant_type STRING, "
                           "evaluation_status STRING)",
                           "ConstantExpression");

        // Create TemplateMetaprogramming table for template evaluation
        executeSchemaQuery("CREATE NODE TABLE TemplateMetaprogramming("
                           "node_id INT64 PRIMARY KEY, "
                           "template_kind STRING, "
                           "instantiation_depth INT64, "
                           "template_arguments STRING, "
                           "specialized_template_id INT64, "
                           "metaprogram_result STRING, "
                           "dependent_types STRING, "
                           "substitution_failure_reason STRING)",
                           "TemplateMetaprogramming");

        // Create StaticAssertion table for static_assert tracking
        executeSchemaQuery("CREATE NODE TABLE StaticAssertion("
                           "node_id INT64 PRIMARY KEY, "
                           "assertion_expression STRING, "
                           "assertion_message STRING, "
                           "assertion_result BOOLEAN, "
                           "failure_reason STRING, "
                           "evaluation_context STRING)",
                           "StaticAssertion");

        // Create CFGBlock table for control flow graph blocks
        executeSchemaQuery("CREATE NODE TABLE CFGBlock("
                           "node_id INT64 PRIMARY KEY, "
                           "function_id INT64, "
                           "block_index INT64, "
                           "is_entry_block BOOLEAN, "
                           "is_exit_block BOOLEAN, "
                           "terminator_kind STRING, "
                           "block_content STRING, "
                           "condition_expression STRING, "
                           "has_terminator BOOLEAN, "
                           "reachable BOOLEAN)",
                           "CFGBlock");

        // Preprocessor relationships
        executeSchemaQuery("CREATE REL TABLE MACRO_EXPANSION("
                           "FROM ASTNode TO MacroDefinition, "
                           "expansion_context STRING, "
                           "expansion_arguments STRING)",
                           "MACRO_EXPANSION");

        executeSchemaQuery("CREATE REL TABLE INCLUDES("
                           "FROM ASTNode TO IncludeDirective, "
                           "include_order INT64)",
                           "INCLUDES");

        executeSchemaQuery("CREATE REL TABLE DEFINES("
                           "FROM ASTNode TO MacroDefinition, "
                           "definition_context STRING)",
                           "DEFINES");

        executeSchemaQuery("CREATE REL TABLE HAS_COMMENT("
                           "FROM Declaration TO Comment, "
                           "attachment_type STRING)",
                           "HAS_COMMENT");

        // Constant expression and compile-time evaluation relationships
        executeSchemaQuery("CREATE REL TABLE HAS_CONSTANT_VALUE("
                           "FROM Expression TO ConstantExpression, "
                           "evaluation_stage STRING)",
                           "HAS_CONSTANT_VALUE");

        executeSchemaQuery("CREATE REL TABLE TEMPLATE_EVALUATES_TO("
                           "FROM Declaration TO TemplateMetaprogramming, "
                           "instantiation_context STRING)",
                           "TEMPLATE_EVALUATES_TO");

        executeSchemaQuery("CREATE REL TABLE CONTAINS_STATIC_ASSERT("
                           "FROM Declaration TO StaticAssertion, "
                           "assertion_scope STRING)",
                           "CONTAINS_STATIC_ASSERT");

        // Control Flow Graph relationships
        executeSchemaQuery("CREATE REL TABLE CFG_EDGE("
                           "FROM CFGBlock TO CFGBlock, "
                           "edge_type STRING, "
                           "condition STRING)",
                           "CFG_EDGE");

        executeSchemaQuery("CREATE REL TABLE CONTAINS_CFG("
                           "FROM Declaration TO CFGBlock, "
                           "cfg_role STRING)",
                           "CONTAINS_CFG");

        executeSchemaQuery("CREATE REL TABLE CFG_CONTAINS_STMT("
                           "FROM CFGBlock TO Statement, "
                           "statement_index INT64)",
                           "CFG_CONTAINS_STMT");
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to create database schema: " + std::string(e.what()));
    }
}

// Basic node creation methods (Phase 1 - minimal implementation)
auto KuzuDump::createASTNode(const clang::Decl* decl) -> int64_t
{
    if (!connection || (decl == nullptr))
    {
        return -1;
    }

    // Check if already processed
    auto it = nodeIdMap.find(decl);
    if (it != nodeIdMap.end())
    {
        return it->second;
    }

    int64_t nodeId = nextNodeId++;
    nodeIdMap[decl] = nodeId;

    try
    {
        // Extract basic information
        std::string nodeType = extractNodeType(decl);

        // Format memory address as hex string
        std::stringstream addrStream;
        addrStream << std::hex << (uintptr_t)decl;
        std::string memoryAddr = addrStream.str();

        bool isImplicit = isImplicitNode(decl);

        // Extract detailed source location information
        auto [filename, startLine, startColumn] = extractSourceLocationDetailed(decl->getLocation());
        auto [endFilename, endLine, endColumn] = extractSourceLocationDetailed(decl->getSourceRange().getEnd());

        // Use start location's filename for source_file
        std::string sourceFile = filename;

        // Create base AST node query using string concatenation
        std::string query =
            "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
            "', is_implicit: " + (isImplicit ? "true" : "false") + ", start_line: " + std::to_string(startLine) +
            ", start_column: " + std::to_string(startColumn) + ", end_line: " + std::to_string(endLine) +
            ", end_column: " + std::to_string(endColumn) + ", raw_text: ''})";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);

        // Create specialized Declaration node if this is a named declaration
        if (const auto* namedDecl = dyn_cast<NamedDecl>(decl))
        {
            createDeclarationNode(nodeId, namedDecl);

            // Create type relationships for typed declarations
            if (const auto* valueDecl = dyn_cast<ValueDecl>(namedDecl))
            {
                createTypeNodeAndRelation(nodeId, valueDecl->getType());
            }
            else if (const auto* functionDecl = dyn_cast<FunctionDecl>(namedDecl))
            {
                createTypeNodeAndRelation(nodeId, functionDecl->getType());
            }
        }

        // Create specialized nodes for using declarations
        if (const auto* usingDecl = dyn_cast<UsingDecl>(decl))
        {
            createUsingDeclarationNode(nodeId, usingDecl);
        }
        else if (const auto* usingDirectiveDecl = dyn_cast<UsingDirectiveDecl>(decl))
        {
            createUsingDirectiveNode(nodeId, usingDirectiveDecl);
        }
        else if (const auto* namespaceAliasDecl = dyn_cast<NamespaceAliasDecl>(decl))
        {
            createNamespaceAliasNode(nodeId, namespaceAliasDecl);
        }

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node: " << e.what() << "\n";
        return -1;
    }
}

auto KuzuDump::createASTNode(const clang::Stmt* stmt) -> int64_t
{
    if (!connection || (stmt == nullptr))
    {
        return -1;
    }

    // Check if already processed
    auto it = nodeIdMap.find(stmt);
    if (it != nodeIdMap.end())
    {
        return it->second;
    }

    int64_t nodeId = nextNodeId++;
    nodeIdMap[stmt] = nodeId;

    try
    {
        // Extract basic information
        std::string nodeType = extractNodeType(stmt);
        // Format memory address as hex string
        std::stringstream addrStream;
        addrStream << std::hex << (uintptr_t)stmt;
        std::string memoryAddr = addrStream.str();

        // Extract detailed source location information
        auto [filename, startLine, startColumn] = extractSourceLocationDetailed(stmt->getBeginLoc());
        auto [endFilename, endLine, endColumn] = extractSourceLocationDetailed(stmt->getEndLoc());

        // Use start location's filename for source_file
        std::string sourceFile = filename;

        // Create base AST node query using string concatenation
        std::string query = "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
                            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
                            "', is_implicit: false, start_line: " + std::to_string(startLine) +
                            ", start_column: " + std::to_string(startColumn) +
                            ", end_line: " + std::to_string(endLine) + ", end_column: " + std::to_string(endColumn) +
                            ", raw_text: ''})";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node for statement: " << e.what() << "\n";
        return -1;
    }
}

auto KuzuDump::createASTNode(const clang::Type* type) -> int64_t
{
    if (!connection || (type == nullptr))
    {
        return -1;
    }

    // Check if already processed
    auto it = nodeIdMap.find(type);
    if (it != nodeIdMap.end())
    {
        return it->second;
    }

    int64_t nodeId = nextNodeId++;
    nodeIdMap[type] = nodeId;

    try
    {
        // Extract basic information
        std::string nodeType = extractNodeType(type);
        // Format memory address as hex string
        std::stringstream addrStream;
        addrStream << std::hex << (uintptr_t)type;
        std::string memoryAddr = addrStream.str();

        // Types don't have specific source locations, so use empty values
        std::string sourceFile = "";
        int64_t startLine = -1;
        int64_t startColumn = -1;
        int64_t endLine = -1;
        int64_t endColumn = -1;

        // Create base AST node query using string concatenation
        std::string query = "CREATE (n:ASTNode {node_id: " + std::to_string(nodeId) + ", node_type: '" + nodeType +
                            "', memory_address: '" + memoryAddr + "', source_file: '" + sourceFile +
                            "', is_implicit: false, start_line: " + std::to_string(startLine) +
                            ", start_column: " + std::to_string(startColumn) +
                            ", end_line: " + std::to_string(endLine) + ", end_column: " + std::to_string(endColumn) +
                            ", raw_text: ''})";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);

        return nodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating AST node for type: " << e.what() << "\n";
        return -1;
    }
}

// Relationship creation methods (Phase 2 & 3)
void KuzuDump::createParentChildRelation(int64_t parentId, int64_t childId, int index)
{
    if (!connection || parentId == -1 || childId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (parent:ASTNode {node_id: " + std::to_string(parentId) + "}), " +
                            "(child:ASTNode {node_id: " + std::to_string(childId) + "}) " +
                            "CREATE (parent)-[:PARENT_OF {child_index: " + std::to_string(index) +
                            ", relationship_kind: 'child'}]->(child)";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating PARENT_OF relationship: " << e.what() << "\n";
    }
}

void KuzuDump::createTypeRelation(int64_t declId, int64_t typeId)
{
    if (!connection || declId == -1 || typeId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (decl:Declaration {node_id: " + std::to_string(declId) + "}), " +
                            "(type:Type {node_id: " + std::to_string(typeId) + "}) " +
                            "CREATE (decl)-[:HAS_TYPE {type_role: 'primary'}]->(type)";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating HAS_TYPE relationship: " << e.what() << "\n";
    }
}

void KuzuDump::createReferenceRelation(int64_t fromId, int64_t toId, const std::string& kind)
{
    if (!connection || fromId == -1 || toId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (from:ASTNode {node_id: " + std::to_string(fromId) + "}), " +
                            "(to:Declaration {node_id: " + std::to_string(toId) + "}) " +
                            "CREATE (from)-[:REFERENCES {reference_kind: '" + kind + "', is_direct: true}]->(to)";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating REFERENCES relationship: " << e.what() << "\n";
    }
}

void KuzuDump::createScopeRelation(int64_t nodeId, int64_t scopeId, const std::string& scopeKind)
{
    if (!connection || nodeId == -1 || scopeId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (node:ASTNode {node_id: " + std::to_string(nodeId) + "}), " +
                            "(scope:Declaration {node_id: " + std::to_string(scopeId) + "}) " +
                            "CREATE (node)-[:IN_SCOPE {scope_kind: '" + scopeKind + "'}]->(scope)";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating IN_SCOPE relationship: " << e.what() << "\n";
    }
}

// Type processing methods (Phase 2)
auto KuzuDump::createTypeNodeAndRelation(int64_t declNodeId, clang::QualType qualType) -> int64_t
{
    if (!connection || qualType.isNull())
    {
        return -1;
    }

    int64_t typeNodeId = createTypeNode(qualType);
    if (typeNodeId != -1)
    {
        createTypeRelation(declNodeId, typeNodeId);
    }

    return typeNodeId;
}

auto KuzuDump::createTypeNode(clang::QualType qualType) -> int64_t
{
    if (!connection || qualType.isNull())
    {
        return -1;
    }

    try
    {
        int64_t typeNodeId = nextNodeId++;

        std::string typeName = extractTypeName(qualType);
        std::string typeCategory = extractTypeCategory(qualType);
        std::string qualifiers = extractTypeQualifiers(qualType);
        bool isBuiltIn = isBuiltInType(qualType);

        std::string query = "CREATE (t:Type {node_id: " + std::to_string(typeNodeId) + ", type_name: '" + typeName +
                            "', canonical_type: '" + typeCategory +
                            "', size_bytes: -1, is_const: " + (qualType.isConstQualified() ? "true" : "false") +
                            ", is_volatile: " + (qualType.isVolatileQualified() ? "true" : "false") +
                            ", is_builtin: " + (isBuiltIn ? "true" : "false") + "})";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);

        return typeNodeId;
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Type node: " << e.what() << "\n";
        return -1;
    }
}

// UsingDeclaration-specific node creation methods
void KuzuDump::createUsingDeclarationNode(int64_t nodeId, const clang::UsingDecl* decl)
{
    if (!connection || (decl == nullptr))
    {
        return;
    }

    try
    {
        std::string usingKind = "declaration";
        std::string targetName = "";
        std::string introducesName = "";
        std::string scopeImpact = "current";

        // Extract using declaration information
        if (const auto* nestedNameSpec = decl->getQualifier())
        {
            // Get the qualified name being used
            std::string qualifierStr;
            llvm::raw_string_ostream qualifierStream(qualifierStr);
            nestedNameSpec->print(qualifierStream, clang::PrintingPolicy(clang::LangOptions()));
            targetName = qualifierStr + decl->getNameAsString();
        }
        else
        {
            targetName = decl->getNameAsString();
        }

        introducesName = decl->getNameAsString();

        // Escape single quotes in strings for database storage
        std::ranges::replace(targetName, '\'', '_');
        std::ranges::replace(introducesName, '\'', '_');

        std::string query = "CREATE (u:UsingDeclaration {node_id: " + std::to_string(nodeId) + ", using_kind: '" +
                            usingKind + "', target_name: '" + targetName + "', introduces_name: '" + introducesName +
                            "', scope_impact: '" + scopeImpact + "'})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating UsingDeclaration node: " << e.what() << "\n";
    }
}

void KuzuDump::createUsingDirectiveNode(int64_t nodeId, const clang::UsingDirectiveDecl* decl)
{
    if (!connection || (decl == nullptr))
    {
        return;
    }

    try
    {
        std::string usingKind = "directive";
        std::string targetName = "";
        std::string introducesName = "";
        std::string scopeImpact = "current";

        // Extract using directive information
        if (const auto* nominatedNS = decl->getNominatedNamespace())
        {
            targetName = nominatedNS->getQualifiedNameAsString();
            introducesName = "*";  // Directive brings in all names from namespace
        }

        // Escape single quotes in strings for database storage
        std::ranges::replace(targetName, '\'', '_');

        std::string query = "CREATE (u:UsingDeclaration {node_id: " + std::to_string(nodeId) + ", using_kind: '" +
                            usingKind + "', target_name: '" + targetName + "', introduces_name: '" + introducesName +
                            "', scope_impact: '" + scopeImpact + "'})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating UsingDirective node: " << e.what() << "\n";
    }
}

void KuzuDump::createNamespaceAliasNode(int64_t nodeId, const clang::NamespaceAliasDecl* decl)
{
    if (!connection || (decl == nullptr))
    {
        return;
    }

    try
    {
        std::string usingKind = "alias";
        std::string targetName = "";
        std::string introducesName = "";
        std::string scopeImpact = "current";

        // Extract namespace alias information
        if (const auto* aliasedNS = decl->getNamespace())
        {
            targetName = aliasedNS->getQualifiedNameAsString();
        }
        introducesName = decl->getNameAsString();

        // Escape single quotes in strings for database storage
        std::ranges::replace(targetName, '\'', '_');
        std::ranges::replace(introducesName, '\'', '_');

        std::string query = "CREATE (u:UsingDeclaration {node_id: " + std::to_string(nodeId) + ", using_kind: '" +
                            usingKind + "', target_name: '" + targetName + "', introduces_name: '" + introducesName +
                            "', scope_impact: '" + scopeImpact + "'})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating NamespaceAlias node: " << e.what() << "\n";
    }
}

// Enhanced declaration processing methods (Phase 2)
void KuzuDump::createDeclarationNode(int64_t nodeId, const clang::NamedDecl* decl)
{
    if (!connection || (decl == nullptr))
    {
        return;
    }

    try
    {
        std::string name = decl->getNameAsString();
        std::string qualifiedName = extractQualifiedName(decl);
        std::string accessSpec = extractAccessSpecifier(decl);
        std::string storageClass = extractStorageClass(decl);
        std::string namespaceContext = extractNamespaceContext(decl);
        bool isDef = isDefinition(decl);

        // Create Declaration node with extracted properties
        std::string query = "CREATE (d:Declaration {node_id: " + std::to_string(nodeId) + ", name: '" + name +
                            "', qualified_name: '" + qualifiedName + "', access_specifier: '" + accessSpec +
                            "', storage_class: '" + storageClass + "', is_definition: " + (isDef ? "true" : "false") +
                            ", namespace_context: '" + namespaceContext + "'})";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);

        // Process comments for this declaration
        processComments(decl, nodeId);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Declaration node: " << e.what() << "\n";
    }
}

auto KuzuDump::extractQualifiedName(const clang::NamedDecl* decl) -> std::string
{
    if (decl == nullptr)
    {
        return "";
    }

    std::string qualifiedName = decl->getQualifiedNameAsString();
    // Replace any problematic characters for database storage
    std::ranges::replace(qualifiedName, '\'', '_');
    return qualifiedName;
}

auto KuzuDump::extractAccessSpecifier(const clang::Decl* decl) -> std::string
{
    if (decl == nullptr)
    {
        return "none";
    }

    switch (decl->getAccess())
    {
    case AS_public:
        return "public";
    case AS_protected:
        return "protected";
    case AS_private:
        return "private";
    case AS_none:
    default:
        return "none";
    }
}

auto KuzuDump::extractStorageClass(const clang::Decl* decl) -> std::string
{
    if (const auto* varDecl = dyn_cast<VarDecl>(decl))
    {
        switch (varDecl->getStorageClass())
        {
        case SC_None:
            return "none";
        case SC_Static:
            return "static";
        case SC_Extern:
            return "extern";
        case SC_Auto:
            return "auto";
        case SC_Register:
            return "register";
        default:
            return "unknown";
        }
    }
    else if (const auto* funcDecl = dyn_cast<FunctionDecl>(decl))
    {
        switch (funcDecl->getStorageClass())
        {
        case SC_None:
            return "none";
        case SC_Static:
            return "static";
        case SC_Extern:
            return "extern";
        default:
            return "unknown";
        }
    }

    return "none";
}

auto KuzuDump::extractNamespaceContext(const clang::Decl* decl) -> std::string
{
    if (decl == nullptr)
    {
        return "";
    }

    const DeclContext* context = decl->getDeclContext();
    std::vector<std::string> namespaces;

    while ((context != nullptr) && !context->isTranslationUnit())
    {
        if (const auto* nsDecl = dyn_cast<NamespaceDecl>(context))
        {
            if (!nsDecl->isAnonymousNamespace())
            {
                namespaces.push_back(nsDecl->getNameAsString());
            }
        }
        else if (const auto* recordDecl = dyn_cast<RecordDecl>(context))
        {
            if (recordDecl->getIdentifier() != nullptr)
            {
                namespaces.push_back(recordDecl->getNameAsString());
            }
        }
        context = context->getParent();
    }

    std::ranges::reverse(namespaces);

    std::string result;
    for (size_t i = 0; i < namespaces.size(); ++i)
    {
        if (i > 0)
            result += "::";
        result += namespaces[i];
    }

    return result;
}

auto KuzuDump::isDefinition(const clang::Decl* decl) -> bool
{
    if (decl == nullptr)
    {
        return false;
    }

    if (const auto* funcDecl = dyn_cast<FunctionDecl>(decl))
    {
        return funcDecl->isThisDeclarationADefinition();
    }
    if (const auto* varDecl = dyn_cast<VarDecl>(decl))
    {
        return varDecl->isThisDeclarationADefinition() != 0;
    }
    if (const auto* recordDecl = dyn_cast<RecordDecl>(decl))
    {
        return recordDecl->isThisDeclarationADefinition();
    }

    return false;
}

// Data extraction utility methods (enhanced for Phase 2)
auto KuzuDump::extractSourceLocation(const clang::SourceLocation& loc) -> std::string
{
    if (loc.isInvalid())
    {
        return "<invalid>";
    }

    // Get detailed location information
    auto [fileName, line, column] = extractSourceLocationDetailed(loc);

    // Return formatted location string
    if (fileName == "<invalid>" || line == -1 || column == -1)
    {
        return "<unknown_location>";
    }

    return fileName + ":" + std::to_string(line) + ":" + std::to_string(column);
}

auto KuzuDump::extractSourceLocationDetailed(const clang::SourceLocation& loc)
    -> std::tuple<std::string, int64_t, int64_t>
{
    if (loc.isInvalid() || (sourceManager == nullptr))
    {
        return std::make_tuple("<invalid>", -1, -1);
    }

    try
    {
        // Use SourceManager to get precise location information
        auto presumedLoc = sourceManager->getPresumedLoc(loc);
        if (presumedLoc.isInvalid())
        {
            return std::make_tuple("<invalid>", -1, -1);
        }

        // Extract filename, line, and column from the presumed location
        std::string filename = (presumedLoc.getFilename() != nullptr) ? presumedLoc.getFilename() : "<unknown>";
        int64_t line = presumedLoc.getLine();
        int64_t column = presumedLoc.getColumn();

        // Clean up filename for database storage (escape single quotes)
        std::ranges::replace(filename, '\'', '_');

        return std::make_tuple(filename, line, column);
    }
    catch (...)
    {
        // If any exception occurs, return invalid location
        return std::make_tuple("<exception>", -1, -1);
    }
}

auto KuzuDump::extractNodeType(const clang::Decl* decl) -> std::string
{
    if (decl == nullptr)
    {
        return "UnknownDecl";
    }
    return decl->getDeclKindName();
}

auto KuzuDump::extractNodeType(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
    {
        return "UnknownStmt";
    }
    return stmt->getStmtClassName();
}

auto KuzuDump::extractNodeType(const clang::Type* type) -> std::string
{
    if (type == nullptr)
    {
        return "UnknownType";
    }
    return type->getTypeClassName();
}

auto KuzuDump::isImplicitNode(const clang::Decl* decl) -> bool
{
    if (decl == nullptr)
    {
        return false;
    }
    return decl->isImplicit();
}

// Type extraction utility methods (Phase 2)
auto KuzuDump::extractTypeName(clang::QualType qualType) -> std::string
{
    if (qualType.isNull())
    {
        return "<invalid_type>";
    }

    // Get the unqualified type name
    return qualType.getUnqualifiedType().getAsString();
}

auto KuzuDump::extractTypeCategory(clang::QualType qualType) -> std::string
{
    if (qualType.isNull())
    {
        return "invalid";
    }

    const clang::Type* type = qualType.getTypePtr();
    if (type == nullptr)
    {
        return "invalid";
    }

    if (type->isBuiltinType())
    {
        return "builtin";
    }
    if (type->isPointerType())
    {
        return "pointer";
    }
    if (type->isReferenceType())
    {
        return "reference";
    }
    if (type->isArrayType())
    {
        return "array";
    }
    if (type->isFunctionType())
    {
        return "function";
    }
    if (type->isRecordType())
    {
        return "record";
    }
    if (type->isEnumeralType())
    {
        return "enum";
    }
    if (type->isTemplateTypeParmType())
    {
        return "template_param";
    }

    return "user_defined";
}

auto KuzuDump::extractTypeQualifiers(clang::QualType qualType) -> std::string
{
    if (qualType.isNull())
    {
        return "";
    }

    std::string qualifiers;

    if (qualType.isConstQualified())
    {
        qualifiers += "const ";
    }
    if (qualType.isVolatileQualified())
    {
        qualifiers += "volatile ";
    }
    if (qualType.isRestrictQualified())
    {
        qualifiers += "restrict ";
    }

    // Remove trailing space
    if (!qualifiers.empty() && qualifiers.back() == ' ')
    {
        qualifiers.pop_back();
    }

    return qualifiers;
}

auto KuzuDump::isBuiltInType(clang::QualType qualType) -> bool
{
    if (qualType.isNull())
    {
        return false;
    }

    const clang::Type* type = qualType.getTypePtr();
    return (type != nullptr) && type->isBuiltinType();
}

auto KuzuDump::extractTypeSourceLocation(clang::QualType /*qualType*/) -> std::string
{
    // For now, types don't have specific source locations
    // This could be enhanced in the future to show where types are defined
    return "<type_location>";
}

// Enhanced statement and expression processing methods implementation
void KuzuDump::createStatementNode(int64_t nodeId, const clang::Stmt* stmt)
{
    if (!connection || (stmt == nullptr))
    {
        return;
    }

    try
    {
        std::string statementKind = extractStatementKind(stmt);
        std::string controlFlowType = extractControlFlowType(stmt);
        std::string conditionText = extractConditionText(stmt);
        bool hasSideEffects = hasStatementSideEffects(stmt);
        bool isCompound = isCompoundStatement(stmt);
        bool isConstexpr = isStatementConstexpr(stmt);

        // Escape single quotes in text fields
        std::ranges::replace(conditionText, '\'', '_');

        std::string query = "CREATE (s:Statement {node_id: " + std::to_string(nodeId) + ", statement_kind: '" +
                            statementKind + "', has_side_effects: " + (hasSideEffects ? "true" : "false") +
                            ", is_compound: " + (isCompound ? "true" : "false") + ", control_flow_type: '" +
                            controlFlowType + "', condition_text: '" + conditionText +
                            "', is_constexpr: " + (isConstexpr ? "true" : "false") + "})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Statement node: " << e.what() << "\n";
    }
}

void KuzuDump::createExpressionNode(int64_t nodeId, const clang::Expr* expr)
{
    if (!connection || (expr == nullptr))
    {
        return;
    }

    try
    {
        std::string expressionKind = extractExpressionKind(expr);
        std::string valueCategory = extractValueCategory(expr);
        std::string literalValue = extractLiteralValue(expr);
        std::string operatorKind = extractOperatorKind(expr);
        std::string evaluationResult = extractEvaluationResult(expr);
        std::string implicitCastKind = extractImplicitCastKind(expr);
        bool isConstexpr = isExpressionConstexpr(expr);

        // Escape single quotes in text fields
        std::ranges::replace(literalValue, '\'', '_');
        std::ranges::replace(evaluationResult, '\'', '_');

        std::string query = "CREATE (e:Expression {node_id: " + std::to_string(nodeId) + ", expression_kind: '" +
                            expressionKind + "', value_category: '" + valueCategory + "', literal_value: '" +
                            literalValue + "', operator_kind: '" + operatorKind +
                            "', is_constexpr: " + (isConstexpr ? "true" : "false") + ", evaluation_result: '" +
                            evaluationResult + "', implicit_cast_kind: '" + implicitCastKind + "'})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating Expression node: " << e.what() << "\n";
    }
}

auto KuzuDump::extractStatementKind(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
    {
        return "unknown";
    }
    return stmt->getStmtClassName();
}

auto KuzuDump::extractControlFlowType(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
    {
        return "none";
    }

    if (isa<IfStmt>(stmt))
        return "if";
    if (isa<WhileStmt>(stmt))
        return "while";
    if (isa<ForStmt>(stmt))
        return "for";
    if (isa<DoStmt>(stmt))
        return "do";
    if (isa<SwitchStmt>(stmt))
        return "switch";
    if (isa<CaseStmt>(stmt))
        return "case";
    if (isa<DefaultStmt>(stmt))
        return "default";
    if (isa<BreakStmt>(stmt))
        return "break";
    if (isa<ContinueStmt>(stmt))
        return "continue";
    if (isa<ReturnStmt>(stmt))
        return "return";
    if (isa<GotoStmt>(stmt))
        return "goto";
    if (isa<LabelStmt>(stmt))
        return "label";

    return "none";
}

auto KuzuDump::extractConditionText(const clang::Stmt* stmt) -> std::string
{
    if (stmt == nullptr)
    {
        return "";
    }

    if (const auto* ifStmt = dyn_cast<IfStmt>(stmt))
    {
        if ([[maybe_unused]] const Expr* cond = ifStmt->getCond())
        {
            // For now, return a placeholder. In a full implementation, we'd need
            // a way to get the source text of the condition expression
            return "if_condition";
        }
    }
    else if (const auto* whileStmt = dyn_cast<WhileStmt>(stmt))
    {
        if ([[maybe_unused]] const Expr* cond = whileStmt->getCond())
        {
            return "while_condition";
        }
    }
    else if (const auto* forStmt = dyn_cast<ForStmt>(stmt))
    {
        if ([[maybe_unused]] const Expr* cond = forStmt->getCond())
        {
            return "for_condition";
        }
    }

    return "";
}

auto KuzuDump::hasStatementSideEffects(const clang::Stmt* stmt) -> bool
{
    if (stmt == nullptr)
    {
        return false;
    }

    // Most statements that can have side effects
    if (isa<CallExpr>(stmt) || isa<CXXOperatorCallExpr>(stmt))
        return true;
    if (isa<BinaryOperator>(stmt))
    {
        const auto* binOp = cast<BinaryOperator>(stmt);
        return binOp->isAssignmentOp() || binOp->isCompoundAssignmentOp();
    }
    if (isa<UnaryOperator>(stmt))
    {
        const auto* unaryOp = cast<UnaryOperator>(stmt);
        return unaryOp->isIncrementDecrementOp();
    }
    if (isa<CXXNewExpr>(stmt) || isa<CXXDeleteExpr>(stmt))
        return true;
    if (isa<CXXThrowExpr>(stmt))
        return true;

    return false;
}

auto KuzuDump::isCompoundStatement(const clang::Stmt* stmt) -> bool
{
    return stmt != nullptr && isa<CompoundStmt>(stmt);
}

auto KuzuDump::isStatementConstexpr(const clang::Stmt* stmt) -> bool
{
    if (stmt == nullptr)
    {
        return false;
    }

    // Check if this is a constexpr if statement (C++17 feature)
    if (const auto* ifStmt = dyn_cast<IfStmt>(stmt))
    {
        return ifStmt->isConstexpr();
    }

    // For expressions, check if they're constant expressions
    if (const auto* expr = dyn_cast<Expr>(stmt))
    {
        return isExpressionConstexpr(expr);
    }

    return false;
}

auto KuzuDump::extractExpressionKind(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "unknown";
    }
    return expr->getStmtClassName();
}

auto KuzuDump::extractValueCategory(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "unknown";
    }

    switch (expr->getValueKind())
    {
    case VK_PRValue:
        return "prvalue";
    case VK_LValue:
        return "lvalue";
    case VK_XValue:
        return "xvalue";
    default:
        return "unknown";
    }
}

auto KuzuDump::extractLiteralValue(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "";
    }

    if (const auto* intLit = dyn_cast<IntegerLiteral>(expr))
    {
        return std::to_string(intLit->getValue().getSExtValue());
    }
    if (const auto* floatLit = dyn_cast<FloatingLiteral>(expr))
    {
        return std::to_string(floatLit->getValueAsApproximateDouble());
    }
    if (const auto* stringLit = dyn_cast<StringLiteral>(expr))
    {
        return stringLit->getString().str();
    }
    if (const auto* charLit = dyn_cast<CharacterLiteral>(expr))
    {
        return std::to_string(charLit->getValue());
    }
    if (const auto* boolLit = dyn_cast<CXXBoolLiteralExpr>(expr))
    {
        return boolLit->getValue() ? "true" : "false";
    }
    if (isa<CXXNullPtrLiteralExpr>(expr))
    {
        return "nullptr";
    }

    return "";
}

auto KuzuDump::extractOperatorKind(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "";
    }

    if (const auto* binOp = dyn_cast<BinaryOperator>(expr))
    {
        return binOp->getOpcodeStr().str();
    }
    if (const auto* unaryOp = dyn_cast<UnaryOperator>(expr))
    {
        return UnaryOperator::getOpcodeStr(unaryOp->getOpcode()).str();
    }
    if (const auto* cxxOp = dyn_cast<CXXOperatorCallExpr>(expr))
    {
        return getOperatorSpelling(cxxOp->getOperator());
    }

    return "";
}

auto KuzuDump::isExpressionConstexpr(const clang::Expr* expr) -> bool
{
    if (expr == nullptr)
    {
        return false;
    }

    // Check if the expression is a constant expression
    // Simplified approach - check if it's a literal or has constant value
    return isa<IntegerLiteral>(expr) || isa<FloatingLiteral>(expr) || isa<StringLiteral>(expr) ||
           isa<CharacterLiteral>(expr) || isa<CXXBoolLiteralExpr>(expr) || isa<CXXNullPtrLiteralExpr>(expr);
}

auto KuzuDump::extractEvaluationResult(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "";
    }

    // For constant expressions, try to get their evaluated value
    // Simplified approach - just check if it's a literal
    if (isa<IntegerLiteral>(expr) || isa<FloatingLiteral>(expr) || isa<StringLiteral>(expr) ||
        isa<CharacterLiteral>(expr) || isa<CXXBoolLiteralExpr>(expr) || isa<CXXNullPtrLiteralExpr>(expr))
    {
        return extractLiteralValue(expr);
    }

    return "";
}

auto KuzuDump::extractImplicitCastKind(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "";
    }

    if (const auto* castExpr = dyn_cast<ImplicitCastExpr>(expr))
    {
        return castExpr->getCastKindName();
    }

    return "";
}

// Hierarchy processing methods (Phase 2)
void KuzuDump::pushParent(int64_t parentNodeId)
{
    parentStack.push_back(parentNodeId);
    childIndex = 0;  // Reset child index for new parent
}

void KuzuDump::popParent()
{
    if (!parentStack.empty())
    {
        parentStack.pop_back();
        childIndex = 0;  // Reset child index when popping
    }
}

void KuzuDump::createHierarchyRelationship(int64_t childNodeId)
{
    if (parentStack.empty() || childNodeId == -1)
    {
        return;
    }

    int64_t parentNodeId = getCurrentParent();
    if (parentNodeId != -1)
    {
        createParentChildRelation(parentNodeId, childNodeId, childIndex++);
    }
}

auto KuzuDump::getCurrentParent() -> int64_t
{
    if (parentStack.empty())
    {
        return -1;
    }
    return parentStack.back();
}

// Scope processing methods (Phase 3)
void KuzuDump::pushScope(int64_t scopeNodeId)
{
    if (scopeNodeId != -1)
    {
        scopeStack.push_back(scopeNodeId);
    }
}

void KuzuDump::popScope()
{
    if (!scopeStack.empty())
    {
        scopeStack.pop_back();
    }
}

void KuzuDump::createScopeRelationships(int64_t nodeId)
{
    if (nodeId == -1 || scopeStack.empty())
    {
        return;
    }

    // Create IN_SCOPE relationships for all current scopes
    for (int64_t scopeId : scopeStack)
    {
        // Determine scope kind based on scope type
        // For now, we'll use "local" as default, but this could be enhanced
        createScopeRelation(nodeId, scopeId, "local");
    }
}

auto KuzuDump::getCurrentScope() -> int64_t
{
    if (scopeStack.empty())
    {
        return -1;
    }
    return scopeStack.back();
}

void KuzuDump::createTemplateRelation(int64_t specializationId, int64_t templateId, const std::string& kind)
{
    if (!connection || specializationId == -1 || templateId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (spec:ASTNode {node_id: " + std::to_string(specializationId) + "}), " +
                            "(tmpl:Declaration {node_id: " + std::to_string(templateId) + "}) " +
                            "CREATE (spec)-[:TEMPLATE_RELATION {relation_kind: '" + kind +
                            "', specialization_type: 'explicit'}]->(tmpl)";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating TEMPLATE_RELATION relationship: " << e.what() << "\n";
    }
}

void KuzuDump::createSpecializesRelation(int64_t specializationId,
                                         int64_t templateId,
                                         const std::string& specializationKind,
                                         const std::string& templateArguments,
                                         const std::string& instantiationContext)
{
    if (!connection || specializationId == -1 || templateId == -1)
    {
        return;
    }

    try
    {
        // Escape single quotes in the template arguments and context
        std::string escapedArgs = templateArguments;
        std::string escapedContext = instantiationContext;

        // Simple escaping - replace single quotes with double quotes
        size_t pos = 0;
        while ((pos = escapedArgs.find('\'', pos)) != std::string::npos)
        {
            escapedArgs.replace(pos, 1, "''");
            pos += 2;
        }
        pos = 0;
        while ((pos = escapedContext.find('\'', pos)) != std::string::npos)
        {
            escapedContext.replace(pos, 1, "''");
            pos += 2;
        }

        std::string query = "MATCH (spec:Declaration {node_id: " + std::to_string(specializationId) + "}), " +
                            "(tmpl:Declaration {node_id: " + std::to_string(templateId) + "}) " +
                            "CREATE (spec)-[:SPECIALIZES {specialization_kind: '" + specializationKind +
                            "', template_arguments: '" + escapedArgs + "', instantiation_context: '" + escapedContext +
                            "'}]->(tmpl)";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating SPECIALIZES relationship: " << e.what() << "\n";
    }
}

void KuzuDump::createTemplateParameterNode(int64_t nodeId, const clang::NamedDecl* param)
{
    if (!connection || nodeId == -1 || param == nullptr)
    {
        return;
    }

    try
    {
        std::string parameterKind;
        std::string parameterName = param->getNameAsString();
        bool hasDefaultArgument = false;
        std::string defaultArgumentText;
        bool isParameterPack = false;

        // Determine the parameter kind and extract specific information
        if (const auto* typeParam = dyn_cast<clang::TemplateTypeParmDecl>(param))
        {
            parameterKind = "type";
            hasDefaultArgument = typeParam->hasDefaultArgument();
            if (hasDefaultArgument)
            {
                clang::TemplateArgumentLoc defaultArg = typeParam->getDefaultArgument();
                if (defaultArg.getArgument().getKind() == clang::TemplateArgument::Type)
                {
                    defaultArgumentText = defaultArg.getArgument().getAsType().getAsString();
                }
            }
            isParameterPack = typeParam->isParameterPack();
        }
        else if (const auto* nonTypeParam = dyn_cast<clang::NonTypeTemplateParmDecl>(param))
        {
            parameterKind = "non_type";
            hasDefaultArgument = nonTypeParam->hasDefaultArgument();
            if (hasDefaultArgument)
            {
                // Extract default argument text - simplified approach
                defaultArgumentText = "default_value";  // Could be enhanced to extract actual value
            }
            isParameterPack = nonTypeParam->isParameterPack();
        }
        else if (const auto* templateParam = dyn_cast<clang::TemplateTemplateParmDecl>(param))
        {
            parameterKind = "template";
            hasDefaultArgument = templateParam->hasDefaultArgument();
            if (hasDefaultArgument)
            {
                // Extract template default argument - simplified
                defaultArgumentText = "default_template";
            }
            isParameterPack = templateParam->isParameterPack();
        }
        else
        {
            parameterKind = "unknown";
        }

        // Escape single quotes in parameter name and default argument
        std::string escapedName = parameterName;
        std::string escapedDefault = defaultArgumentText;

        size_t pos = 0;
        while ((pos = escapedName.find('\'', pos)) != std::string::npos)
        {
            escapedName.replace(pos, 1, "''");
            pos += 2;
        }
        pos = 0;
        while ((pos = escapedDefault.find('\'', pos)) != std::string::npos)
        {
            escapedDefault.replace(pos, 1, "''");
            pos += 2;
        }

        std::string hasDefaultStr = hasDefaultArgument ? "true" : "false";
        std::string isPackStr = isParameterPack ? "true" : "false";

        std::string query = "CREATE (tp:TemplateParameter {";
        query += "node_id: " + std::to_string(nodeId) + ", ";
        query += "parameter_kind: '" + parameterKind + "', ";
        query += "parameter_name: '" + escapedName + "', ";
        query += "has_default_argument: " + hasDefaultStr + ", ";
        query += "default_argument_text: '" + escapedDefault + "', ";
        query += "is_parameter_pack: " + isPackStr + "})";

        // Use batched operation for performance optimization (Phase 4)
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating TemplateParameter node: " << e.what() << "\n";
    }
}

auto KuzuDump::extractTemplateArguments(const clang::TemplateArgumentList& args) -> std::string
{
    std::string result = "[";

    for (unsigned i = 0; i < args.size(); ++i)
    {
        if (i > 0)
        {
            result += ", ";
        }

        const clang::TemplateArgument& arg = args[i];

        switch (arg.getKind())
        {
        case clang::TemplateArgument::Type:
            result += arg.getAsType().getAsString();
            break;
        case clang::TemplateArgument::Integral:
            result += std::to_string(arg.getAsIntegral().getExtValue());
            break;
        case clang::TemplateArgument::Declaration:
            if (const auto* namedDecl = dyn_cast<clang::NamedDecl>(arg.getAsDecl()))
            {
                result += namedDecl->getNameAsString();
            }
            else
            {
                result += "declaration";
            }
            break;
        case clang::TemplateArgument::NullPtr:
            result += "nullptr";
            break;
        case clang::TemplateArgument::Template:
            result += arg.getAsTemplate().getAsTemplateDecl()->getNameAsString();
            break;
        case clang::TemplateArgument::Pack:
            result += "{pack of " + std::to_string(arg.pack_size()) + " args}";
            break;
        default:
            result += "unknown_arg";
            break;
        }
    }

    result += "]";
    return result;
}

// Comment processing methods implementation
void KuzuDump::createCommentNode(int64_t nodeId,
                                 const std::string& commentText,
                                 const std::string& commentKind,
                                 bool isDocumentationComment,
                                 const std::string& briefText,
                                 const std::string& detailedText)
{
    if (!connection || nodeId == -1)
    {
        return;
    }

    try
    {
        // Escape single quotes in strings for database storage
        std::string escapedText = commentText;
        std::string escapedBrief = briefText;
        std::string escapedDetailed = detailedText;
        std::string escapedKind = commentKind;

        // Replace single quotes with double quotes to avoid SQL injection
        std::ranges::replace(escapedText, '\'', '"');
        std::ranges::replace(escapedBrief, '\'', '"');
        std::ranges::replace(escapedDetailed, '\'', '"');
        std::ranges::replace(escapedKind, '\'', '"');

        std::string query = "CREATE (c:Comment {node_id: " + std::to_string(nodeId) + ", comment_text: '" +
                            escapedText + "'" + ", comment_kind: '" + escapedKind + "'" +
                            ", is_documentation: " + (isDocumentationComment ? "true" : "false") + ", brief_text: '" +
                            escapedBrief + "'" + ", detailed_text: '" + escapedDetailed + "'})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Error creating comment node: " << e.what() << "\n";
    }
}

void KuzuDump::createCommentRelation(int64_t declId, int64_t commentId)
{
    if (!connection || declId == -1 || commentId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (d:Declaration {node_id: " + std::to_string(declId) + "}), " +
                            "(c:Comment {node_id: " + std::to_string(commentId) + "}) " +
                            "CREATE (d)-[:HAS_COMMENT {attachment_type: 'documentation'}]->(c)";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Error creating comment relation: " << e.what() << "\n";
    }
}

void KuzuDump::processComments(const clang::Decl* decl, int64_t declId)
{
    if ((decl == nullptr) || (sourceManager == nullptr))
    {
        return;
    }

    // Get the AST context from the declaration
    const ASTContext& context = decl->getASTContext();

    // Try to get raw comment for this declaration
    const RawComment* rawComment = context.getRawCommentForAnyRedecl(decl);
    if (rawComment != nullptr)
    {
        // Create a comment node
        int64_t commentId = nextNodeId++;

        std::string commentText = rawComment->getRawText(*sourceManager).str();
        std::string commentKind = rawComment->getKind() == RawComment::RCK_OrdinaryBCPL ? "BCPL"
                                  : rawComment->getKind() == RawComment::RCK_OrdinaryC  ? "C"
                                                                                        : "Invalid";
        bool isDocumentation = rawComment->isDocumentation();

        std::string briefText;
        std::string detailedText;

        // Try to parse structured comment if it's documentation
        if (isDocumentation)
        {
            const comments::FullComment* fullComment = context.getCommentForDecl(decl, nullptr);
            if (fullComment != nullptr)
            {
                // Extract brief and detailed text from structured comment
                briefText = extractCommentText(fullComment);
                detailedText = briefText;  // For now, use same text for both
            }
        }

        createCommentNode(commentId, commentText, commentKind, isDocumentation, briefText, detailedText);
        createCommentRelation(declId, commentId);
    }
}

auto KuzuDump::extractCommentKind(const clang::comments::Comment* comment) -> std::string
{
    if (comment == nullptr)
    {
        return "unknown";
    }

    switch (comment->getCommentKind())
    {
    case comments::CommentKind::FullComment:
        return "FullComment";
    case comments::CommentKind::ParagraphComment:
        return "Paragraph";
    case comments::CommentKind::BlockCommandComment:
        return "BlockCommand";
    case comments::CommentKind::ParamCommandComment:
        return "ParamCommand";
    case comments::CommentKind::TParamCommandComment:
        return "TParamCommand";
    case comments::CommentKind::VerbatimBlockComment:
        return "VerbatimBlock";
    case comments::CommentKind::VerbatimBlockLineComment:
        return "VerbatimBlockLine";
    case comments::CommentKind::VerbatimLineComment:
        return "VerbatimLine";
    case comments::CommentKind::TextComment:
        return "Text";
    case comments::CommentKind::InlineCommandComment:
        return "InlineCommand";
    case comments::CommentKind::HTMLStartTagComment:
        return "HTMLStartTag";
    case comments::CommentKind::HTMLEndTagComment:
        return "HTMLEndTag";
    default:
        return "unknown";
    }
}

auto KuzuDump::extractCommentText(const clang::comments::Comment* comment) -> std::string
{
    if (comment == nullptr)
    {
        return "";
    }

    // For now, return a simple text extraction
    // In a more sophisticated implementation, we could walk the comment AST
    // and extract formatted text
    return "Comment text extraction";  // Placeholder - would need more sophisticated implementation
}

auto KuzuDump::isDocumentationComment(const clang::comments::Comment* comment) -> bool
{
    return (comment != nullptr) && comment->getCommentKind() == comments::CommentKind::FullComment;
}

// Constant expression and compile-time evaluation methods implementation
void KuzuDump::createConstantExpressionNode(int64_t nodeId,
                                            const clang::Expr* expr,
                                            bool isConstexprFunction,
                                            const std::string& evaluationContext)
{
    if (!connection || nodeId == -1 || expr == nullptr)
    {
        return;
    }

    try
    {
        std::string evaluationResult = evaluateConstantExpression(expr);
        auto [constantValue, constantType] = extractConstantValue(expr);
        std::string evaluationStatus = extractEvaluationStatus(expr);
        std::string resultType = expr->getType().getAsString();
        bool isCompileTimeConstant = expr->isConstantInitializer(const_cast<ASTContext&>(*astContext), false);

        // Escape single quotes in text fields
        std::ranges::replace(evaluationResult, '\'', '_');
        std::ranges::replace(constantValue, '\'', '_');
        std::ranges::replace(constantType, '\'', '_');
        std::ranges::replace(evaluationStatus, '\'', '_');
        std::ranges::replace(resultType, '\'', '_');

        std::string query =
            "CREATE (ce:ConstantExpression {node_id: " + std::to_string(nodeId) +
            ", is_constexpr_function: " + (isConstexprFunction ? "true" : "false") + ", evaluation_context: '" +
            evaluationContext + "', evaluation_result: '" + evaluationResult + "', result_type: '" + resultType +
            "', is_compile_time_constant: " + (isCompileTimeConstant ? "true" : "false") + ", constant_value: '" +
            constantValue + "', constant_type: '" + constantType + "', evaluation_status: '" + evaluationStatus + "'})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating ConstantExpression node: " << e.what() << "\n";
    }
}

void KuzuDump::createTemplateMetaprogrammingNode(int64_t nodeId,
                                                 const clang::Decl* templateDecl,
                                                 const std::string& templateKind,
                                                 int64_t instantiationDepth)
{
    if (!connection || nodeId == -1 || templateDecl == nullptr)
    {
        return;
    }

    try
    {
        std::string templateArguments;
        int64_t specializedTemplateId = -1;
        std::string metaprogramResult;
        std::string dependentTypes;
        std::string substitutionFailureReason;

        if (const auto* classTemplateDecl = dyn_cast<ClassTemplateDecl>(templateDecl))
        {
            templateArguments = extractTemplateArguments(classTemplateDecl);
        }
        else if (const auto* functionTemplateDecl = dyn_cast<FunctionTemplateDecl>(templateDecl))
        {
            templateArguments = extractTemplateArguments(functionTemplateDecl);
        }
        else if (const auto* classTemplateSpecDecl = dyn_cast<ClassTemplateSpecializationDecl>(templateDecl))
        {
            const auto& args = classTemplateSpecDecl->getTemplateArgs();
            templateArguments = extractTemplateArguments(args);
            if (classTemplateSpecDecl->getSpecializedTemplate() != nullptr)
            {
                auto it = nodeIdMap.find(classTemplateSpecDecl->getSpecializedTemplate());
                if (it != nodeIdMap.end())
                {
                    specializedTemplateId = it->second;
                }
            }
        }

        // Escape single quotes in text fields
        std::ranges::replace(templateArguments, '\'', '_');
        std::ranges::replace(metaprogramResult, '\'', '_');
        std::ranges::replace(dependentTypes, '\'', '_');
        std::ranges::replace(substitutionFailureReason, '\'', '_');

        std::string query =
            "CREATE (tmp:TemplateMetaprogramming {node_id: " + std::to_string(nodeId) + ", template_kind: '" +
            templateKind + "', instantiation_depth: " + std::to_string(instantiationDepth) + ", template_arguments: '" +
            templateArguments + "', specialized_template_id: " + std::to_string(specializedTemplateId) +
            ", metaprogram_result: '" + metaprogramResult + "', dependent_types: '" + dependentTypes +
            "', substitution_failure_reason: '" + substitutionFailureReason + "'})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating TemplateMetaprogramming node: " << e.what() << "\n";
    }
}

void KuzuDump::createStaticAssertionNode(int64_t nodeId, const clang::StaticAssertDecl* assertDecl)
{
    if (!connection || nodeId == -1 || assertDecl == nullptr)
    {
        return;
    }

    try
    {
        auto [assertionExpression, assertionMessage, assertionResult] = extractStaticAssertInfo(assertDecl);
        std::string failureReason;
        std::string evaluationContext = "static_assert";

        if (!assertionResult && (assertDecl->getMessage() != nullptr))
        {
            failureReason = assertionMessage;
        }

        // Escape single quotes in text fields
        std::ranges::replace(assertionExpression, '\'', '_');
        std::ranges::replace(assertionMessage, '\'', '_');
        std::ranges::replace(failureReason, '\'', '_');
        std::ranges::replace(evaluationContext, '\'', '_');

        std::string query = "CREATE (sa:StaticAssertion {node_id: " + std::to_string(nodeId) +
                            ", assertion_expression: '" + assertionExpression + "', assertion_message: '" +
                            assertionMessage + "', assertion_result: " + (assertionResult ? "true" : "false") +
                            ", failure_reason: '" + failureReason + "', evaluation_context: '" + evaluationContext +
                            "'})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating StaticAssertion node: " << e.what() << "\n";
    }
}

void KuzuDump::createConstantValueRelation(int64_t exprId, int64_t constantId, const std::string& stage)
{
    if (!connection || exprId == -1 || constantId == -1)
    {
        return;
    }

    std::string escapedStage = stage;
    std::ranges::replace(escapedStage, '\'', '_');

    std::string query = "MATCH (e:Expression {node_id: " + std::to_string(exprId) + "}), " +
                        "(c:ConstantExpression {node_id: " + std::to_string(constantId) + "}) " +
                        "CREATE (e)-[r:HAS_CONSTANT_VALUE {evaluation_stage: '" + escapedStage + "'}]->(c)";

    addToBatch(query);
}

void KuzuDump::createTemplateEvaluationRelation(int64_t templateId, int64_t metaprogramId, const std::string& context)
{
    if (!connection || templateId == -1 || metaprogramId == -1)
    {
        return;
    }

    std::string escapedContext = context;
    std::ranges::replace(escapedContext, '\'', '_');

    std::string query = "MATCH (d:Declaration {node_id: " + std::to_string(templateId) + "}), " +
                        "(tmp:TemplateMetaprogramming {node_id: " + std::to_string(metaprogramId) + "}) " +
                        "CREATE (d)-[r:TEMPLATE_EVALUATES_TO {instantiation_context: '" + escapedContext + "'}]->(tmp)";

    addToBatch(query);
}

void KuzuDump::createStaticAssertRelation(int64_t declId, int64_t assertId, const std::string& scope)
{
    if (!connection || declId == -1 || assertId == -1)
    {
        return;
    }

    std::string escapedScope = scope;
    std::ranges::replace(escapedScope, '\'', '_');

    std::string query = "MATCH (d:Declaration {node_id: " + std::to_string(declId) + "}), " +
                        "(sa:StaticAssertion {node_id: " + std::to_string(assertId) + "}) " +
                        "CREATE (d)-[r:CONTAINS_STATIC_ASSERT {assertion_scope: '" + escapedScope + "'}]->(sa)";

    addToBatch(query);
}

// Enhanced constant expression evaluation methods
auto KuzuDump::evaluateConstantExpression(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "";
    }

    // Try to evaluate the expression at compile time
    if (expr->isEvaluatable(*astContext))
    {
        clang::Expr::EvalResult result;
        if (expr->EvaluateAsRValue(result, *astContext) && !result.HasSideEffects)
        {
            const auto& value = result.Val;
            if (value.isInt())
            {
                return std::to_string(value.getInt().getSExtValue());
            }
            if (value.isFloat())
            {
                return std::to_string(value.getFloat().convertToDouble());
            }
            if (value.isLValue())
            {
                return "lvalue";
            }
            if (value.isComplexInt())
            {
                return "complex_int";
            }
            if (value.isComplexFloat())
            {
                return "complex_float";
            }
        }
    }

    return "";
}

auto KuzuDump::extractConstantValue(const clang::Expr* expr) -> std::pair<std::string, std::string>
{
    if (expr == nullptr)
    {
        return {"", ""};
    }

    std::string value;
    std::string type = expr->getType().getAsString();

    if (const auto* intLiteral = dyn_cast<IntegerLiteral>(expr))
    {
        value = std::to_string(intLiteral->getValue().getSExtValue());
    }
    else if (const auto* floatLiteral = dyn_cast<FloatingLiteral>(expr))
    {
        value = std::to_string(floatLiteral->getValue().convertToDouble());
    }
    else if (const auto* stringLiteral = dyn_cast<StringLiteral>(expr))
    {
        value = stringLiteral->getString().str();
    }
    else if (const auto* charLiteral = dyn_cast<CharacterLiteral>(expr))
    {
        value = std::to_string(charLiteral->getValue());
    }
    else if (const auto* boolLiteral = dyn_cast<CXXBoolLiteralExpr>(expr))
    {
        value = boolLiteral->getValue() ? "true" : "false";
    }
    else if (isa<CXXNullPtrLiteralExpr>(expr))
    {
        value = "nullptr";
    }
    else
    {
        value = evaluateConstantExpression(expr);
    }

    return {value, type};
}

auto KuzuDump::extractEvaluationStatus(const clang::Expr* expr) -> std::string
{
    if (expr == nullptr)
    {
        return "null_expression";
    }

    if (expr->isEvaluatable(*astContext))
    {
        return "evaluatable";
    }
    if (expr->isValueDependent())
    {
        return "value_dependent";
    }
    if (expr->isTypeDependent())
    {
        return "type_dependent";
    }
    if (expr->containsUnexpandedParameterPack())
    {
        return "contains_parameter_pack";
    }

    return "not_evaluatable";
}

auto KuzuDump::detectConstexprFunction(const clang::FunctionDecl* func) -> bool
{
    if (func == nullptr)
    {
        return false;
    }

    return func->isConstexpr() || func->isConstexprSpecified();
}

auto KuzuDump::extractTemplateInstantiationDepth(const clang::Decl* decl) -> int64_t
{
    if (decl == nullptr)
    {
        return 0;
    }

    int64_t depth = 0;
    const DeclContext* context = decl->getDeclContext();

    while (context != nullptr)
    {
        if (isa<ClassTemplateSpecializationDecl>(context))
        {
            depth++;
        }
        else if (const auto* functionDecl = dyn_cast<FunctionDecl>(context))
        {
            if (functionDecl->getTemplateSpecializationInfo() != nullptr)
            {
                depth++;
            }
        }
        context = context->getParent();
    }

    return depth;
}

auto KuzuDump::extractTemplateArguments(const clang::TemplateDecl* templateDecl) -> std::string
{
    if (templateDecl == nullptr)
    {
        return "";
    }

    if (const auto* templateParams = templateDecl->getTemplateParameters())
    {
        std::string arguments;
        for (unsigned int i = 0; i < templateParams->size(); ++i)
        {
            if (i > 0)
                arguments += ", ";
            if (const auto* namedDecl = dyn_cast<NamedDecl>(templateParams->getParam(i)))
            {
                arguments += namedDecl->getNameAsString();
            }
        }
        return arguments;
    }

    return "";
}

auto KuzuDump::extractStaticAssertInfo(const clang::StaticAssertDecl* assertDecl)
    -> std::tuple<std::string, std::string, bool>
{
    if (assertDecl == nullptr)
    {
        return {"", "", false};
    }

    std::string expression;
    std::string message;
    bool result = true;

    if (const auto* condExpr = assertDecl->getAssertExpr())
    {
        // Try to get the expression text
        if (sourceManager != nullptr)
        {
            auto range = condExpr->getSourceRange();
            if (range.isValid())
            {
                expression = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range),
                                                         *sourceManager,
                                                         astContext->getLangOpts())
                                 .str();
            }
        }

        // Check if the assertion would fail
        if (condExpr->isEvaluatable(*astContext))
        {
            clang::Expr::EvalResult evalResult;
            if (condExpr->EvaluateAsRValue(evalResult, *astContext))
            {
                if (evalResult.Val.isInt())
                {
                    result = evalResult.Val.getInt().getBoolValue();
                }
            }
        }
    }

    if (const auto* messageExpr = assertDecl->getMessage())
    {
        if (const auto* stringLiteral = dyn_cast<StringLiteral>(messageExpr))
        {
            message = stringLiteral->getString().str();
        }
    }

    return {expression, message, result};
}

// Preprocessor and Macro processing methods implementation
void KuzuDump::createMacroDefinitionNode(int64_t nodeId,
                                         const std::string& macroName,
                                         bool isFunctionLike,
                                         const std::vector<std::string>& parameters,
                                         const std::string& replacementText,
                                         bool isBuiltin,
                                         bool isConditional)
{
    if (!connection || nodeId == -1)
    {
        return;
    }

    try
    {
        // Escape single quotes in strings for database storage
        std::string escapedName = macroName;
        std::string escapedReplacement = replacementText;
        std::string parametersStr;

        // Join parameters into a single string
        for (size_t i = 0; i < parameters.size(); ++i)
        {
            if (i > 0)
                parametersStr += ", ";
            parametersStr += parameters[i];
        }

        // Escape strings
        std::ranges::replace(escapedName, '\'', '_');
        std::ranges::replace(escapedReplacement, '\'', '_');
        std::ranges::replace(parametersStr, '\'', '_');

        std::string query = "CREATE (m:MacroDefinition {" + std::string("node_id: ") + std::to_string(nodeId) + ", " +
                            "macro_name: '" + escapedName + "', " +
                            "is_function_like: " + (isFunctionLike ? "true" : "false") + ", " +
                            "parameter_count: " + std::to_string(parameters.size()) + ", " + "parameter_names: '" +
                            parametersStr + "', " + "replacement_text: '" + escapedReplacement + "', " +
                            "is_builtin: " + (isBuiltin ? "true" : "false") + ", " +
                            "is_conditional: " + (isConditional ? "true" : "false") + "})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating MacroDefinition node: " << e.what() << "\n";
    }
}

void KuzuDump::createIncludeDirectiveNode(int64_t nodeId,
                                          const std::string& includePath,
                                          bool isSystemInclude,
                                          bool isAngled,
                                          const std::string& resolvedPath,
                                          int64_t includeDepth)
{
    if (!connection || nodeId == -1)
    {
        return;
    }

    try
    {
        // Escape single quotes in strings for database storage
        std::string escapedIncludePath = includePath;
        std::string escapedResolvedPath = resolvedPath;

        std::ranges::replace(escapedIncludePath, '\'', '_');
        std::ranges::replace(escapedResolvedPath, '\'', '_');

        std::string query = "CREATE (i:IncludeDirective {" + std::string("node_id: ") + std::to_string(nodeId) + ", " +
                            "include_path: '" + escapedIncludePath + "', " +
                            "is_system_include: " + (isSystemInclude ? "true" : "false") + ", " +
                            "is_angled: " + (isAngled ? "true" : "false") + ", " + "resolved_path: '" +
                            escapedResolvedPath + "', " + "include_depth: " + std::to_string(includeDepth) + "})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating IncludeDirective node: " << e.what() << "\n";
    }
}

void KuzuDump::createConditionalDirectiveNode(int64_t nodeId,
                                              const std::string& directiveType,
                                              const std::string& conditionText,
                                              bool isTrueBranch,
                                              int64_t nestingLevel)
{
    if (!connection || nodeId == -1)
    {
        return;
    }

    try
    {
        // Escape single quotes in strings for database storage
        std::string escapedDirectiveType = directiveType;
        std::string escapedConditionText = conditionText;

        std::ranges::replace(escapedDirectiveType, '\'', '_');
        std::ranges::replace(escapedConditionText, '\'', '_');

        std::string query = "CREATE (c:ConditionalDirective {" + std::string("node_id: ") + std::to_string(nodeId) +
                            ", " + "directive_type: '" + escapedDirectiveType + "', " + "condition_text: '" +
                            escapedConditionText + "', " + "is_true_branch: " + (isTrueBranch ? "true" : "false") +
                            ", " + "nesting_level: " + std::to_string(nestingLevel) + "})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating ConditionalDirective node: " << e.what() << "\n";
    }
}

void KuzuDump::createPragmaDirectiveNode(int64_t nodeId,
                                         const std::string& pragmaName,
                                         const std::string& pragmaText,
                                         const std::string& pragmaKind)
{
    if (!connection || nodeId == -1)
    {
        return;
    }

    try
    {
        // Escape single quotes in strings for database storage
        std::string escapedPragmaName = pragmaName;
        std::string escapedPragmaText = pragmaText;
        std::string escapedPragmaKind = pragmaKind;

        std::ranges::replace(escapedPragmaName, '\'', '_');
        std::ranges::replace(escapedPragmaText, '\'', '_');
        std::ranges::replace(escapedPragmaKind, '\'', '_');

        std::string query = "CREATE (p:PragmaDirective {" + std::string("node_id: ") + std::to_string(nodeId) + ", " +
                            "pragma_name: '" + escapedPragmaName + "', " + "pragma_text: '" + escapedPragmaText +
                            "', " + "pragma_kind: '" + escapedPragmaKind + "'})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating PragmaDirective node: " << e.what() << "\n";
    }
}

void KuzuDump::createMacroExpansionRelation(int64_t fromId,
                                            int64_t macroId,
                                            const std::string& expansionContext,
                                            const std::string& expansionArguments)
{
    if (!connection || fromId == -1 || macroId == -1)
    {
        return;
    }

    try
    {
        // Escape single quotes in strings for database storage
        std::string escapedContext = expansionContext;
        std::string escapedArguments = expansionArguments;

        std::ranges::replace(escapedContext, '\'', '_');
        std::ranges::replace(escapedArguments, '\'', '_');

        std::string query = "MATCH (from:ASTNode {node_id: " + std::to_string(fromId) + "}), " +
                            "(macro:MacroDefinition {node_id: " + std::to_string(macroId) + "}) " +
                            "CREATE (from)-[:MACRO_EXPANSION {expansion_context: '" + escapedContext +
                            "', expansion_arguments: '" + escapedArguments + "'}]->(macro)";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating MACRO_EXPANSION relationship: " << e.what() << "\n";
    }
}

void KuzuDump::createIncludesRelation(int64_t fromId, int64_t includeId, int64_t includeOrder)
{
    if (!connection || fromId == -1 || includeId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (from:ASTNode {node_id: " + std::to_string(fromId) + "}), " +
                            "(include:IncludeDirective {node_id: " + std::to_string(includeId) + "}) " +
                            "CREATE (from)-[:INCLUDES {include_order: " + std::to_string(includeOrder) +
                            "}]->(include)";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating INCLUDES relationship: " << e.what() << "\n";
    }
}

void KuzuDump::createDefinesRelation(int64_t fromId, int64_t macroId, const std::string& definitionContext)
{
    if (!connection || fromId == -1 || macroId == -1)
    {
        return;
    }

    try
    {
        // Escape single quotes in strings for database storage
        std::string escapedContext = definitionContext;
        std::ranges::replace(escapedContext, '\'', '_');

        std::string query = "MATCH (from:ASTNode {node_id: " + std::to_string(fromId) + "}), " +
                            "(macro:MacroDefinition {node_id: " + std::to_string(macroId) + "}) " +
                            "CREATE (from)-[:DEFINES {definition_context: '" + escapedContext + "'}]->(macro)";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating DEFINES relationship: " << e.what() << "\n";
    }
}

void KuzuDump::createInheritanceRelation(int64_t derivedId,
                                         int64_t baseId,
                                         const std::string& inheritanceType,
                                         bool isVirtual,
                                         const std::string& accessPath)
{
    if (!connection || derivedId == -1 || baseId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (derived:Declaration {node_id: " + std::to_string(derivedId) + "}), " +
                            "(base:Declaration {node_id: " + std::to_string(baseId) + "}) " +
                            "CREATE (derived)-[:INHERITS_FROM {inheritance_type: '" + inheritanceType +
                            "', is_virtual: " + (isVirtual ? "true" : "false") + ", base_access_path: '" + accessPath +
                            "'}]->(base)";

        // Use batched operation for performance optimization
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating INHERITS_FROM relationship: " << e.what() << "\n";
    }
}

void KuzuDump::createOverrideRelation(int64_t overridingId,
                                      int64_t overriddenId,
                                      const std::string& overrideType,
                                      bool isCovariantReturn)
{
    if (!connection || overridingId == -1 || overriddenId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (overriding:Declaration {node_id: " + std::to_string(overridingId) + "}), " +
                            "(overridden:Declaration {node_id: " + std::to_string(overriddenId) + "}) " +
                            "CREATE (overriding)-[:OVERRIDES {override_type: '" + overrideType +
                            "', is_covariant_return: " + (isCovariantReturn ? "true" : "false") + "}]->(overridden)";

        // Use batched operation for performance optimization
        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating OVERRIDES relationship: " << e.what() << "\n";
    }
}

// Core Visit method implementations
void KuzuDump::VisitDecl(const Decl* D)
{
    if (D == nullptr)
    {
        return;
    }

    // Create database node for this declaration
    int64_t nodeId = createASTNode(D);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitFunctionDecl(const FunctionDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this function declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this function (it's in its parent scope)
    createScopeRelationships(nodeId);

    // Check if this is a function template specialization
    if (const FunctionTemplateSpecializationInfo* specInfo = D->getTemplateSpecializationInfo())
    {
        if (const FunctionTemplateDecl* templateDecl = specInfo->getTemplate())
        {
            // Check if the template declaration has been processed
            auto it = nodeIdMap.find(templateDecl);
            int64_t templateNodeId;
            if (it != nodeIdMap.end())
            {
                templateNodeId = it->second;
            }
            else
            {
                // Create the template node if it doesn't exist yet
                templateNodeId = createASTNode(templateDecl);
            }

            // Create both the legacy TEMPLATE_RELATION and enhanced SPECIALIZES relation
            createTemplateRelation(nodeId, templateNodeId, "specializes");

            // Enhanced specialization tracking with template arguments
            std::string specializationKind;
            switch (specInfo->getTemplateSpecializationKind())
            {
            case clang::TSK_ImplicitInstantiation:
                specializationKind = "implicit";
                break;
            case clang::TSK_ExplicitInstantiationDeclaration:
                specializationKind = "explicit_declaration";
                break;
            case clang::TSK_ExplicitInstantiationDefinition:
                specializationKind = "explicit_definition";
                break;
            case clang::TSK_ExplicitSpecialization:
                specializationKind = "explicit_specialization";
                break;
            default:
                specializationKind = "unknown";
                break;
            }

            std::string templateArguments = extractTemplateArguments(*specInfo->TemplateArguments);
            std::string instantiationContext = D->getQualifiedNameAsString();

            createSpecializesRelation(
                nodeId, templateNodeId, specializationKind, templateArguments, instantiationContext);
        }
    }

    // Analyze Control Flow Graph if the function has a body
    if (D->hasBody())
    {
        analyzeCFGForFunction(D, nodeId);
    }

    // Push this function as a new scope for its body and parameters
    pushScope(nodeId);

    // The ASTNodeTraverser will handle automatic traversal
    // Pop this function scope after traversal
    popScope();
}

void KuzuDump::VisitVarDecl(const VarDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this variable declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this variable (it's in its current scope)
    createScopeRelationships(nodeId);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitParmVarDecl(const ParmVarDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this parameter declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this parameter (it's in its parent function scope)
    createScopeRelationships(nodeId);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitNamespaceDecl(const NamespaceDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this namespace declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this namespace (it's in its parent scope)
    createScopeRelationships(nodeId);

    // Push this namespace as a new scope for its contents
    pushScope(nodeId);

    // The ASTNodeTraverser will handle automatic traversal
    // Pop this namespace scope after traversal
    popScope();
}

void KuzuDump::VisitUsingDecl(const UsingDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this using declaration
    int64_t nodeId = createASTNode(D);

    // Create specialized UsingDeclaration node
    createUsingDeclarationNode(nodeId, D);

    // Create scope relationships for this using declaration
    createScopeRelationships(nodeId);

    // Create reference relationships to the used declarations
    for (const auto* shadow : D->shadows())
    {
        if (const auto* usedDecl = shadow->getTargetDecl())
        {
            // Create or find the used declaration node
            auto it = nodeIdMap.find(usedDecl);
            int64_t usedNodeId;
            if (it != nodeIdMap.end())
            {
                usedNodeId = it->second;
            }
            else
            {
                usedNodeId = createASTNode(usedDecl);
            }

            if (usedNodeId != -1)
            {
                createReferenceRelation(nodeId, usedNodeId, "using");
            }
        }
    }

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitUsingDirectiveDecl(const UsingDirectiveDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this using directive declaration
    int64_t nodeId = createASTNode(D);

    // Create specialized UsingDeclaration node
    createUsingDirectiveNode(nodeId, D);

    // Create scope relationships for this using directive
    createScopeRelationships(nodeId);

    // Create reference relationship to the nominated namespace
    if (const auto* nominatedNS = D->getNominatedNamespace())
    {
        // Create or find the nominated namespace node
        auto it = nodeIdMap.find(nominatedNS);
        int64_t nsNodeId;
        if (it != nodeIdMap.end())
        {
            nsNodeId = it->second;
        }
        else
        {
            nsNodeId = createASTNode(nominatedNS);
        }

        if (nsNodeId != -1)
        {
            createReferenceRelation(nodeId, nsNodeId, "using_directive");
        }
    }

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitNamespaceAliasDecl(const NamespaceAliasDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this namespace alias declaration
    int64_t nodeId = createASTNode(D);

    // Create specialized UsingDeclaration node (aliases are treated as a kind of using)
    createNamespaceAliasNode(nodeId, D);

    // Create scope relationships for this namespace alias
    createScopeRelationships(nodeId);

    // Create reference relationship to the aliased namespace
    if (const auto* aliasedNS = D->getNamespace())
    {
        // Create or find the aliased namespace node
        auto it = nodeIdMap.find(aliasedNS);
        int64_t aliasedNodeId;
        if (it != nodeIdMap.end())
        {
            aliasedNodeId = it->second;
        }
        else
        {
            aliasedNodeId = createASTNode(aliasedNS);
        }

        if (aliasedNodeId != -1)
        {
            createReferenceRelation(nodeId, aliasedNodeId, "namespace_alias");
        }
    }

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitCXXRecordDecl(const CXXRecordDecl* D)
{
    if (D == nullptr)
        return;

    // Create database node for this C++ record (class/struct) declaration
    int64_t nodeId = createASTNode(D);

    // Create scope relationships for this record (it's in its parent scope)
    createScopeRelationships(nodeId);

    // Process inheritance relationships
    if (D->hasDefinition() && D->getNumBases() > 0)
    {
        // Process each base class
        for (const auto& base : D->bases())
        {
            if (const auto* baseDecl = base.getType()->getAsCXXRecordDecl())
            {
                // Create or find the base class node
                int64_t baseNodeId = -1;
                auto it = nodeIdMap.find(baseDecl);
                if (it != nodeIdMap.end())
                {
                    baseNodeId = it->second;
                }
                else
                {
                    baseNodeId = createASTNode(baseDecl);
                }

                if (baseNodeId != -1)
                {
                    // Extract inheritance information
                    std::string inheritanceType;
                    switch (base.getAccessSpecifier())
                    {
                    case AS_public:
                        inheritanceType = "public";
                        break;
                    case AS_protected:
                        inheritanceType = "protected";
                        break;
                    case AS_private:
                        inheritanceType = "private";
                        break;
                    case AS_none:
                    default:
                        inheritanceType = "none";
                        break;
                    }

                    bool isVirtual = base.isVirtual();
                    std::string accessPath = base.getType().getAsString();

                    // Create inheritance relationship
                    createInheritanceRelation(nodeId, baseNodeId, inheritanceType, isVirtual, accessPath);
                }
            }
        }
    }

    // Process virtual function overrides
    if (D->hasDefinition())
    {
        for (const auto* method : D->methods())
        {
            if (method != nullptr && method->isVirtual())
            {
                // Check for overridden methods
                CXXMethodDecl::method_iterator overriddenBegin = method->begin_overridden_methods();
                CXXMethodDecl::method_iterator overriddenEnd = method->end_overridden_methods();

                for (auto it = overriddenBegin; it != overriddenEnd; ++it)
                {
                    const CXXMethodDecl* overriddenMethod = *it;
                    if (overriddenMethod != nullptr)
                    {
                        // Create or find the overridden method node
                        int64_t overriddenNodeId = -1;
                        auto nodeIt = nodeIdMap.find(overriddenMethod);
                        if (nodeIt != nodeIdMap.end())
                        {
                            overriddenNodeId = nodeIt->second;
                        }
                        else
                        {
                            overriddenNodeId = createASTNode(overriddenMethod);
                        }

                        // Create or find the overriding method node
                        int64_t overridingNodeId = -1;
                        auto overridingIt = nodeIdMap.find(method);
                        if (overridingIt != nodeIdMap.end())
                        {
                            overridingNodeId = overridingIt->second;
                        }
                        else
                        {
                            overridingNodeId = createASTNode(method);
                        }

                        if (overridingNodeId != -1 && overriddenNodeId != -1)
                        {
                            // Determine override type
                            std::string overrideType = "virtual";
                            if (method->hasAttr<FinalAttr>())
                            {
                                overrideType = "final";
                            }
                            else if (method->isPureVirtual())
                            {
                                overrideType = "pure_virtual";
                            }
                            else if (method->hasAttr<OverrideAttr>())
                            {
                                overrideType = "override";
                            }

                            // Check for covariant return type (simplified check)
                            bool isCovariantReturn = false;
                            QualType overridingReturnType = method->getReturnType();
                            QualType overriddenReturnType = overriddenMethod->getReturnType();
                            if (!overridingReturnType->isVoidType() && !overriddenReturnType->isVoidType())
                            {
                                // This is a simplified check using canonical types comparison
                                isCovariantReturn = (overridingReturnType.getCanonicalType() !=
                                                     overriddenReturnType.getCanonicalType());
                            }

                            // Create override relationship
                            createOverrideRelation(overridingNodeId, overriddenNodeId, overrideType, isCovariantReturn);
                        }
                    }
                }
            }
        }
    }

    // Push this record as a new scope for its contents
    pushScope(nodeId);

    // The ASTNodeTraverser will handle automatic traversal
    // Pop this record scope after traversal
    popScope();
}

void KuzuDump::VisitStmt(const Stmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this statement
    int64_t nodeId = createASTNode(S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitCompoundStmt(const CompoundStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this compound statement
    int64_t nodeId = createASTNode(S);

    // Create enhanced Statement node with detailed information
    createStatementNode(nodeId, S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitDeclStmt(const DeclStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this declaration statement
    int64_t nodeId = createASTNode(S);

    // Create enhanced Statement node with detailed information
    createStatementNode(nodeId, S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Create scope relationships for this declaration statement
    createScopeRelationships(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // Process each declaration in this statement
    for (const auto* decl : S->decls())
    {
        if (const auto* varDecl = dyn_cast<VarDecl>(decl))
        {
            // Create the variable declaration node
            int64_t varNodeId = createASTNode(varDecl);

            // Create scope relationships for this local variable
            createScopeRelationships(varNodeId);
        }
    }

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitReturnStmt(const ReturnStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this return statement
    int64_t nodeId = createASTNode(S);

    // Create enhanced Statement node with detailed information
    createStatementNode(nodeId, S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitIfStmt(const IfStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this if statement
    int64_t nodeId = createASTNode(S);

    // Create enhanced Statement node with detailed information
    createStatementNode(nodeId, S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitWhileStmt(const WhileStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this while statement
    int64_t nodeId = createASTNode(S);

    // Create enhanced Statement node with detailed information
    createStatementNode(nodeId, S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitForStmt(const ForStmt* S)
{
    if (S == nullptr)
        return;

    // Create database node for this for statement
    int64_t nodeId = createASTNode(S);

    // Create enhanced Statement node with detailed information
    createStatementNode(nodeId, S);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitExpr(const Expr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this expression
    int64_t nodeId = createASTNode(E);

    // Create enhanced Expression node with detailed information
    createExpressionNode(nodeId, E);

    // Check if this is a constant expression and create additional information
    if (isExpressionConstexpr(E) || E->isEvaluatable(*astContext))
    {
        // Determine if this is within a constexpr function context
        bool isConstexprFunction = false;
        // Check current scope stack for constexpr functions
        for (auto scopeIt = scopeStack.rbegin(); scopeIt != scopeStack.rend(); ++scopeIt)
        {
            // Find the declaration corresponding to this scope
            for (const auto& [decl, scopeNodeId] : nodeIdMap)
            {
                if (scopeNodeId == *scopeIt)
                {
                    if (const auto* funcDecl = dyn_cast<FunctionDecl>(static_cast<const Decl*>(decl)))
                    {
                        isConstexprFunction = detectConstexprFunction(funcDecl);
                        if (isConstexprFunction)
                            break;
                    }
                }
            }
            if (isConstexprFunction)
                break;
        }

        // Create a separate constant expression node
        int64_t constExprNodeId = nextNodeId++;
        std::string evaluationContext = isConstexprFunction ? "constexpr_function" : "constant_expression";
        createConstantExpressionNode(constExprNodeId, E, isConstexprFunction, evaluationContext);

        // Create relationship between the expression and its constant evaluation
        createConstantValueRelation(nodeId, constExprNodeId, "compile_time");
    }

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitDeclRefExpr(const DeclRefExpr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this declaration reference expression
    int64_t nodeId = createASTNode(E);

    // Create reference relationship if we can find the referenced declaration
    if (const Decl* referencedDecl = E->getDecl())
    {
        // Check if the referenced declaration has been processed
        auto it = nodeIdMap.find(referencedDecl);
        if (it != nodeIdMap.end())
        {
            createReferenceRelation(nodeId, it->second, "uses");
        }
        else
        {
            // Create the referenced declaration node if it doesn't exist yet
            int64_t referencedNodeId = createASTNode(referencedDecl);
            if (referencedNodeId != -1)
            {
                createReferenceRelation(nodeId, referencedNodeId, "uses");
            }
        }
    }

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitIntegerLiteral(const IntegerLiteral* E)
{
    if (E == nullptr)
        return;

    // Create database node for this integer literal
    int64_t nodeId = createASTNode(E);

    // Create enhanced Expression node with detailed information
    createExpressionNode(nodeId, E);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitBinaryOperator(const BinaryOperator* E)
{
    if (E == nullptr)
        return;

    // Create database node for this binary operator
    int64_t nodeId = createASTNode(E);

    // Create enhanced Expression node with detailed information
    createExpressionNode(nodeId, E);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitUnaryOperator(const UnaryOperator* E)
{
    if (E == nullptr)
        return;

    // Create database node for this unary operator
    int64_t nodeId = createASTNode(E);

    // Create enhanced Expression node with detailed information
    createExpressionNode(nodeId, E);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}

void KuzuDump::VisitCallExpr(const CallExpr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this call expression
    int64_t nodeId = createASTNode(E);

    // Create reference relationship for function calls
    if (const FunctionDecl* calledFunc = E->getDirectCallee())
    {
        // Check if the called function has been processed
        auto it = nodeIdMap.find(calledFunc);
        if (it != nodeIdMap.end())
        {
            createReferenceRelation(nodeId, it->second, "calls");
        }
        else
        {
            // Create the called function node if it doesn't exist yet
            int64_t calledFuncNodeId = createASTNode(calledFunc);
            if (calledFuncNodeId != -1)
            {
                createReferenceRelation(nodeId, calledFuncNodeId, "calls");
            }
        }
    }

    // Handle indirect calls through function pointers or expressions
    if (const Expr* callee = E->getCallee())
    {
        if (const auto* declRef = dyn_cast<DeclRefExpr>(callee))
        {
            // This will be handled by VisitDeclRefExpr, so we don't duplicate
            static_cast<void>(declRef);  // Suppress unused variable warning
        }
        else if (const auto* memberExpr = dyn_cast<MemberExpr>(callee))
        {
            // Handle member function calls
            if (const ValueDecl* memberDecl = memberExpr->getMemberDecl())
            {
                auto it = nodeIdMap.find(memberDecl);
                if (it != nodeIdMap.end())
                {
                    createReferenceRelation(nodeId, it->second, "calls");
                }
                else
                {
                    int64_t memberNodeId = createASTNode(memberDecl);
                    if (memberNodeId != -1)
                    {
                        createReferenceRelation(nodeId, memberNodeId, "calls");
                    }
                }
            }
        }
    }

    // The ASTNodeTraverser will handle automatic traversal
}

void KuzuDump::VisitImplicitCastExpr(const ImplicitCastExpr* E)
{
    if (E == nullptr)
        return;

    // Create database node for this implicit cast expression
    int64_t nodeId = createASTNode(E);

    // Create enhanced Expression node with detailed information
    createExpressionNode(nodeId, E);

    // Create hierarchy relationship if this node has a parent
    createHierarchyRelationship(nodeId);

    // Push this node as parent for potential children
    pushParent(nodeId);

    // The ASTNodeTraverser will handle automatic traversal

    // Pop this node as parent after traversal
    popParent();
}
// Control Flow Graph (CFG) analysis implementation
void KuzuDump::analyzeCFGForFunction(const clang::FunctionDecl* func, int64_t functionNodeId)
{
    if (!connection || func == nullptr || !func->hasBody())
    {
        return;
    }

    try
    {
        // Build CFG for the function body
        clang::Stmt* body = func->getBody();
        clang::ASTContext& context = func->getASTContext();
        clang::CFG::BuildOptions options;

        // Enable additional options for comprehensive CFG
        options.AddImplicitDtors = true;
        options.AddInitializers = true;
        options.AddTemporaryDtors = true;

        std::unique_ptr<clang::CFG> cfg = clang::CFG::buildCFG(func, body, &context, options);

        if (!cfg)
        {
            llvm::errs() << "Failed to build CFG for function: " << func->getNameAsString() << "\n";
            return;
        }

        // Map to store CFG block node IDs
        std::unordered_map<const clang::CFGBlock*, int64_t> blockNodeMap;

        // First pass: Create CFG block nodes
        for (auto it = cfg->begin(); it != cfg->end(); ++it)
        {
            const clang::CFGBlock* block = *it;
            if (block == nullptr)
                continue;

            int64_t blockNodeId = nextNodeId++;
            blockNodeMap[block] = blockNodeId;

            bool isEntry = (block == &cfg->getEntry());
            bool isExit = (block == &cfg->getExit());
            int blockIndex = block->getBlockID();

            createCFGBlockNode(blockNodeId, functionNodeId, block, blockIndex, isEntry, isExit);
            createCFGContainsRelation(functionNodeId, blockNodeId);
        }

        // Second pass: Create CFG edges
        for (auto block : *cfg)
        {
            if (block == nullptr)
                continue;

            int64_t fromBlockId = blockNodeMap[block];

            // Create edges to successor blocks
            for (auto successor = block->succ_begin(); successor != block->succ_end(); ++successor)
            {
                const clang::CFGBlock* succBlock = successor->getReachableBlock();
                if ((succBlock != nullptr) && blockNodeMap.find(succBlock) != blockNodeMap.end())
                {
                    int64_t toBlockId = blockNodeMap[succBlock];
                    std::string edgeType = extractCFGEdgeType(*block);
                    std::string condition = extractCFGCondition(block);

                    createCFGEdgeRelation(fromBlockId, toBlockId, edgeType, condition);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception analyzing CFG for function " << func->getNameAsString() << ": " << e.what() << "\n";
    }
}

void KuzuDump::createCFGBlockNode(int64_t blockNodeId,
                                  int64_t functionNodeId,
                                  const clang::CFGBlock* block,
                                  int blockIndex,
                                  bool isEntry,
                                  bool isExit)
{
    if (!connection || blockNodeId == -1 || block == nullptr)
    {
        return;
    }

    try
    {
        std::string blockContent = extractCFGBlockContent(block);
        std::string terminatorKind = "none";
        bool hasTerminator = false;
        std::string conditionExpression = extractCFGCondition(block);

        // Analyze terminator
        if (block->getTerminator().isValid())
        {
            hasTerminator = true;
            const clang::Stmt* terminator = block->getTerminator().getStmt();
            if (terminator != nullptr)
            {
                terminatorKind = terminator->getStmtClassName();
            }
        }

        // Escape single quotes in strings for database storage
        std::ranges::replace(blockContent, '\'', '_');
        std::ranges::replace(terminatorKind, '\'', '_');
        std::ranges::replace(conditionExpression, '\'', '_');

        std::string query =
            "CREATE (b:CFGBlock {" + std::string("node_id: ") + std::to_string(blockNodeId) + ", " +
            "function_id: " + std::to_string(functionNodeId) + ", " + "block_index: " + std::to_string(blockIndex) +
            ", " + "is_entry_block: " + (isEntry ? "true" : "false") + ", " +
            "is_exit_block: " + (isExit ? "true" : "false") + ", " + "terminator_kind: '" + terminatorKind + "', " +
            "block_content: '" + blockContent + "', " + "condition_expression: '" + conditionExpression + "', " +
            "has_terminator: " + (hasTerminator ? "true" : "false") + ", " + "reachable: true})";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating CFGBlock node: " << e.what() << "\n";
    }
}

void KuzuDump::createCFGEdgeRelation(int64_t fromBlockId,
                                     int64_t toBlockId,
                                     const std::string& edgeType,
                                     const std::string& condition)
{
    if (!connection || fromBlockId == -1 || toBlockId == -1)
    {
        return;
    }

    try
    {
        std::string escapedEdgeType = edgeType;
        std::string escapedCondition = condition;

        std::ranges::replace(escapedEdgeType, '\'', '_');
        std::ranges::replace(escapedCondition, '\'', '_');

        std::string query = "MATCH (from:CFGBlock {node_id: " + std::to_string(fromBlockId) + "}), " +
                            "(to:CFGBlock {node_id: " + std::to_string(toBlockId) + "}) " +
                            "CREATE (from)-[:CFG_EDGE {" + "edge_type: '" + escapedEdgeType + "', " + "condition: '" +
                            escapedCondition + "'}]->(to)";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating CFG edge relation: " << e.what() << "\n";
    }
}

void KuzuDump::createCFGContainsRelation(int64_t functionId, int64_t cfgBlockId)
{
    if (!connection || functionId == -1 || cfgBlockId == -1)
    {
        return;
    }

    try
    {
        std::string query = "MATCH (func:Declaration {node_id: " + std::to_string(functionId) + "}), " +
                            "(block:CFGBlock {node_id: " + std::to_string(cfgBlockId) + "}) " +
                            "CREATE (func)-[:CONTAINS_CFG {cfg_role: 'block'}]->(block)";

        addToBatch(query);
    }
    catch (const std::exception& e)
    {
        llvm::errs() << "Exception creating CFG contains relation: " << e.what() << "\n";
    }
}

auto KuzuDump::extractCFGBlockContent(const clang::CFGBlock* block) -> std::string
{
    if ((block == nullptr) || (sourceManager == nullptr))
    {
        return "";
    }

    std::string content;
    bool first = true;

    // Collect statements in the block
    for (auto it : *block)
    {
        if (const auto stmt = it.getAs<clang::CFGStmt>())
        {
            const clang::Stmt* s = stmt->getStmt();
            if (s != nullptr)
            {
                if (!first)
                    content += "; ";

                // Try to get source text
                auto range = s->getSourceRange();
                if (range.isValid())
                {
                    std::string stmtText = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range),
                                                                       *sourceManager,
                                                                       astContext->getLangOpts())
                                               .str();

                    if (!stmtText.empty())
                    {
                        content += stmtText;
                    }
                    else
                    {
                        content += s->getStmtClassName();
                    }
                }
                else
                {
                    content += s->getStmtClassName();
                }
                first = false;
            }
        }
    }

    if (content.empty())
    {
        content = "empty_block";
    }

    return content;
}

auto KuzuDump::extractCFGEdgeType(const clang::CFGBlock& from) -> std::string
{
    // Analyze the terminator of the 'from' block to determine edge type
    if (from.getTerminator().isValid())
    {
        const clang::Stmt* terminator = from.getTerminator().getStmt();
        if (terminator != nullptr)
        {
            switch (terminator->getStmtClass())
            {
            case clang::Stmt::IfStmtClass:
                // For if statements, determine if this is the true or false branch
                // This is a simplified heuristic
                return "conditional";
            case clang::Stmt::WhileStmtClass:
            case clang::Stmt::ForStmtClass:
            case clang::Stmt::DoStmtClass:
                return "loop";
            case clang::Stmt::SwitchStmtClass:
                return "switch";
            case clang::Stmt::ReturnStmtClass:
                return "return";
            case clang::Stmt::BreakStmtClass:
                return "break";
            case clang::Stmt::ContinueStmtClass:
                return "continue";
            case clang::Stmt::GotoStmtClass:
                return "goto";
            default:
                return "sequential";
            }
        }
    }

    return "unconditional";
}

auto KuzuDump::extractCFGCondition(const clang::CFGBlock* block) -> std::string
{
    if ((block == nullptr) || (sourceManager == nullptr))
    {
        return "";
    }

    if (const clang::Stmt* termCond = block->getTerminatorCondition())
    {
        if (const auto* condition = clang::dyn_cast<clang::Expr>(termCond))
        {
            auto range = condition->getSourceRange();
            if (range.isValid())
            {
                return clang::Lexer::getSourceText(
                           clang::CharSourceRange::getTokenRange(range), *sourceManager, astContext->getLangOpts())
                    .str();
            }
        }
    }

    return "";
}
