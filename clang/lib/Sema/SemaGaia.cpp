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

void Sema::addField(IdentifierInfo *name, QualType type, RecordDecl *RD) const
{
    FieldDecl *Field = FieldDecl::Create(
        Context, RD, SourceLocation(), SourceLocation(),
        name, type, /*TInfo=*/nullptr,
        /*BitWidth=*/nullptr, /*Mutable=*/false, ICIS_NoInit);
    Field->setAccess(AS_public);
    RD->addDecl(Field);
}

void Sema::addMethod(IdentifierInfo *name, DeclSpec::TST retValType, DeclaratorChunk::ParamInfo *Params,
    unsigned NumParams, AttributeFactory &attrFactory, ParsedAttributes &attrs, Scope *S, RecordDecl *RD) 
{
    DeclSpec DS(attrFactory);
    const char *dummy;
    unsigned diagId;

    Declarator D(DS, DeclaratorContext::MemberContext);
    D.setFunctionDefinitionKind(FDK_Declaration);
    D.getMutableDeclSpec().SetTypeSpecType(retValType, SourceLocation(), dummy,
        diagId, getPrintingPolicy());
    ActOnAccessSpecifier(AS_public, SourceLocation(), SourceLocation(), attrs);
            
    D.SetIdentifier(name, SourceLocation());
            
    DS.Finish(*this, getPrintingPolicy());

    D.AddTypeInfo(DeclaratorChunk::getFunction(
        true, false, SourceLocation(), Params,
        NumParams, SourceLocation(), SourceLocation(),
        true, SourceLocation(),
        /*MutableLoc=*/SourceLocation(),
        EST_None, SourceRange(), nullptr,
        nullptr, 0, nullptr,
        nullptr, None, SourceLocation(),
        SourceLocation(), D, TypeResult(), &DS),
        std::move(attrs), SourceLocation());
                                   
    DeclarationNameInfo NameInfo = GetNameForDeclarator(D);

    TypeSourceInfo *tInfo = GetTypeForDeclarator(D,S);

    CXXMethodDecl *Ret = CXXMethodDecl::Create(
        Context, cast<CXXRecordDecl>(RD), SourceLocation(), NameInfo, tInfo->getType(),
        tInfo, SC_None, false, false, SourceLocation());
    Ret->setAccess(AS_public);
    RD->addDecl(Ret);        
}

QualType Sema::getTableType (IdentifierInfo *table) 
{
    std::string tableName = table->getName().str();
    RecordDecl *RD = Context.buildImplicitRecord(tableName + "__type");
    RD->setLexicalDeclContext(CurContext);
    RD->startDefinition();
    Scope S(CurScope,Scope::DeclScope|Scope::ClassScope, Diags);
    ActOnTagStartDefinition(&S,RD);
    ActOnStartCXXMemberDeclarations(getCurScope(), RD, SourceLocation(),
        false, SourceLocation());
    AttributeFactory attrFactory;
    ParsedAttributes attrs(attrFactory);
    
    addField(&Context.Idents.get("x"), Context.WIntTy, RD);
    addMethod(&Context.Idents.get("update"), DeclSpec::TST_void, nullptr, 0, attrFactory, attrs, &S, RD);
    
    ActOnFinishCXXMemberSpecification(getCurScope(), SourceLocation(), RD,
        SourceLocation(), SourceLocation(), attrs);
    ActOnTagFinishDefinition(getCurScope(), RD, SourceRange());
  
    return Context.getTagDeclType(RD);
}

QualType Sema::getFieldType (IdentifierInfo *id) const
{
    return Context.WIntTy;
}

NamedDecl *Sema::injectVariableDefinition(IdentifierInfo *II, bool isGaiaFieldTable)
{
    std::string varName = II->getName().str();

    DeclContext *context  = getCurFunctionDecl();
    QualType qualType = isGaiaFieldTable ? getTableType(II) : getFieldType(II);


    VarDecl *varDecl = VarDecl::Create(Context, context, SourceLocation(), SourceLocation(),
                            II, qualType, Context.getTrivialTypeSourceInfo(qualType, SourceLocation()), SC_None);
    varDecl->setLexicalDeclContext(context);
    varDecl->setImplicit();
    if (varName == "UPDATE")
    {
        varDecl->addAttr(GaiaLastOperationUPDATEAttr::CreateImplicit(Context));
    }
    else if (varName == "INSERT")
    {
        varDecl->addAttr(GaiaLastOperationINSERTAttr::CreateImplicit(Context));
    }
    else if (varName == "DELETE")
    {
        varDecl->addAttr(GaiaLastOperationDELETEAttr::CreateImplicit(Context));
    }
    else
    {
        varDecl->addAttr(GaiaFieldAttr::CreateImplicit(Context));
    }
    if (isGaiaFieldTable)
    {
        varDecl->addAttr(FieldTableAttr::CreateImplicit(Context,II));
    }
    
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

void Sema::ActOnRulesetDefFinish(Decl *Dcl, SourceLocation RBrace)
{
    RulesetDecl *ruleset = dyn_cast_or_null<RulesetDecl>(Dcl);
    assert(ruleset && "Invalid parameter, expected RulesetDecl");
    ruleset->setRBraceLoc(RBrace);
    PopDeclContext();
}

