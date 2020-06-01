//===--- ParseGaia.cpp - Gaia Extensions Parser -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Gaia Extensions parsing of the Parser
// interface.
//
//===----------------------------------------------------------------------===//
/////////////////////////////////////////////
// Modifications Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "clang/AST/PrettyDeclStackTrace.h"
#include "clang/Basic/Attributes.h"
#include "clang/Basic/PrettyStackTrace.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/Scope.h"
using namespace clang;


QualType Sema::getFieldType (IdentifierInfo *id) const
{
    return Context.WIntTy;
}

NamedDecl *Sema::injectVariableDefinition(IdentifierInfo *II)
{
    DeclContext *context  = getCurFunctionDecl();
    QualType qualType = getFieldType(II);

    VarDecl *varDecl = VarDecl::Create(Context, context, SourceLocation(), SourceLocation(),
                            II, qualType, Context.getTrivialTypeSourceInfo(qualType, SourceLocation()), SC_None);
    varDecl->setLexicalDeclContext(context);
    varDecl->setImplicit();
    varDecl->addAttr(GaiaFieldAttr::CreateImplicit(Context));
    context->addDecl(varDecl);
    
    return varDecl;
}

Decl *Sema::ActOnRulesetDefStart(Scope *S, SourceLocation RulesetLoc,
    SourceLocation IdentLoc, IdentifierInfo *Ident, const ParsedAttributesView &AttrList )
{
    Scope *declRegionScope = S->getParent();

    RulesetDecl *ruleset = RulesetDecl::Create(Context, CurContext, 
        RulesetLoc, IdentLoc, Ident);
    
    ProcessDeclAttributeList(declRegionScope, ruleset, AttrList);
    
    PushOnScopeChains(ruleset, declRegionScope);
    ActOnDocumentableDecl(ruleset);
    PushDeclContext(S, ruleset);
    return ruleset;
}

void Sema::ActOnRulesetDefFinish()
{
    PopDeclContext();
}

