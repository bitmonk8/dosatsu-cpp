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

// clang-format off
#include "NoWarningScope_Enter.h"
#include "KuzuDump.h"
#include "clang/AST/ASTConcept.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclLookups.h"
#include "clang/AST/JSONNodeDumper.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"
#include "NoWarningScope_Leave.h"
// clang-format on

using namespace clang;
using namespace clang::comments;

void KuzuDump::dumpInvalidDeclContext(const DeclContext* DC)
{
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

                        for (DeclContextLookupResult::iterator RI = R.begin(), RE = R.end(); RI != RE; ++RI)
                        {
                            NodeDumper.AddChild(
                                [=]
                                {
                                    NodeDumper.dumpBareDeclRef(*RI);

                                    if (!(*RI)->isUnconditionallyVisible())
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
                                        DumpWithPrev(*RI);
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

void KuzuDump::VisitFunctionTemplateDecl(const FunctionTemplateDecl* D)
{
    // FIXME: We don't add a declaration of a function template specialization
    // to its context when it's explicitly instantiated, so dump explicit
    // instantiations when we dump the template itself.
    dumpTemplateDecl(D, true);
}

void KuzuDump::VisitClassTemplateDecl(const ClassTemplateDecl* D)
{
    dumpTemplateDecl(D, false);
}

void KuzuDump::VisitVarTemplateDecl(const VarTemplateDecl* D)
{
    dumpTemplateDecl(D, false);
}
