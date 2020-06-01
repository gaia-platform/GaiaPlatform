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
#include "clang/Parse/LoopHint.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/RAIIObjectsForParser.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/TypoCorrection.h"
#include <random>
using namespace clang;


std::string Parser::RandomString(std::string::size_type length) const
{
    const char chrs[] = "ABCDEFGIJKLMNOPQRSTUVWXYZ_"
        "abcdefghijklmnopqrstuvwxyz";

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<std::string::size_type> dis;

    std::string s;

    s.reserve(length);
    while(length--)
    {
        s += chrs[dis(gen)%(sizeof(chrs) - 1)];
    }

    return s;
}

QualType Sema::getFieldType (IdentifierInfo *id) const
{
    return Context.WIntTy;
}

NamedDecl *Sema::InjectVariableDefinition(IdentifierInfo *II)
{
    DeclContext *DC  = getCurFunctionDecl();
    QualType qualType = getFieldType(II);

    VarDecl *NewVD = VarDecl::Create(Context, DC, SourceLocation(), SourceLocation(),
                            II, qualType, Context.getTrivialTypeSourceInfo(qualType, SourceLocation()), SC_None);
    NewVD->setLexicalDeclContext(DC);
    NewVD->setImplicit();
    NewVD->addAttr(GaiaFieldAttr::CreateImplicit(Context));
    DC->addDecl(NewVD);
    
    return NewVD;
}

void Parser::InjectRuleFunction(Declarator &D)
{
    SourceLocation Loc = Tok.getLocation();
    if (!getCurScope()->isRulesetScope())
    {
        Diag(Loc, diag::err_ruleset_ruleset_scope);
        cutOffParsing();;
        return;
    }

    SourceLocation endLoc;
    ParsedAttributesWithRange attrs(AttrFactory);
    attrs.addNew(&PP.getIdentifierTable().get("rule"),
        SourceRange(Loc, Loc),
        nullptr, Loc, nullptr, 0,
        ParsedAttr::AS_Keyword); 

    // Set return type
    const char *PrevSpec = nullptr;
    unsigned int DiagId = 0;
    D.getMutableDeclSpec().SetTypeSpecType(DeclSpec::TST_void, Loc, PrevSpec,
        DiagId, Actions.getPrintingPolicy());
    D.getMutableDeclSpec().takeAttributesFrom(attrs);


    DeclSpec DS(AttrFactory);
    DS.takeAttributesFrom(attrs);

    DS.Finish(Actions, Actions.getASTContext().getPrintingPolicy());
 

    // Set function name
    IdentifierInfo *func = &PP.getIdentifierTable().get(RandomString(10));
    D.SetIdentifier(func, Loc);
    
    D.takeAttributes(attrs, endLoc);
    D.AddTypeInfo(DeclaratorChunk::getFunction(
                    true, false, Loc, nullptr,
                    0, Loc, Loc,
                    true, SourceLocation(),
                    /*MutableLoc=*/SourceLocation(),
                    EST_None, SourceRange(), nullptr,
                    nullptr, 0,
                    nullptr,
                    nullptr, None, Loc,
                    Loc, D, TypeResult(), &DS),
                std::move(attrs), Loc);
}

RulesetDecl *RulesetDecl::Create(ASTContext &C, DeclContext *DC,
    SourceLocation StartLoc,
    SourceLocation IdLoc, IdentifierInfo *Id)
{
    return new (C, DC) RulesetDecl(C, DC, StartLoc, IdLoc, Id);
}

RulesetDecl *RulesetDecl::CreateDeserialized(ASTContext &C, unsigned ID)
{
    return new (C, ID) RulesetDecl(C, nullptr, SourceLocation(), SourceLocation(),
        nullptr);
}

Decl *Sema::ActOnRulesetDefStart(Scope *S, SourceLocation RulesetLoc,
    SourceLocation IdentLoc, IdentifierInfo *Ident, const ParsedAttributesView &AttrList )
{
    Scope *DeclRegionScope = S->getParent();

    RulesetDecl *ruleset = RulesetDecl::Create(Context, CurContext, 
        RulesetLoc, IdentLoc, Ident);
    
    ProcessDeclAttributeList(DeclRegionScope, ruleset, AttrList);
    
    PushOnScopeChains(ruleset, DeclRegionScope);
    ActOnDocumentableDecl(ruleset);
    PushDeclContext(S, ruleset);
    return ruleset;
}

void Sema::ActOnRulesetDefFinish()
{
    PopDeclContext();
}

void Parser::ParseGaiaAttributeSpecifier(ParsedAttributesWithRange &attrs,
    SourceLocation *EndLoc)
{
    if (Tok.is(tok::comma))
    {
        ConsumeToken();
    }
    switch (Tok.getKind())
    {
        case tok::kw_table:
            ParseRulesetTable(attrs, EndLoc);
            return;
        default:
            llvm::errs() << "Unexpected attribute\n"; 
            cutOffParsing();
    }    
}

/// Parse Gaia Specific attributes
void Parser::ParseGaiaAttributes(ParsedAttributesWithRange &attrs,
    SourceLocation *endLoc)
{
    SourceLocation StartLoc = Tok.getLocation(), Loc;
    if (!endLoc)
    {
        endLoc = &Loc;
    }

    do 
    {
        ParseGaiaAttributeSpecifier(attrs, endLoc);
    } while (Tok.is(tok::comma));

    attrs.Range = SourceRange(StartLoc, *endLoc);
}

void Parser::ParseRulesetTable(ParsedAttributesWithRange &attrs,
    SourceLocation *EndLoc)
{
    assert(Tok.is(tok::kw_table) && "Not a ruleset table!");

    ArgsVector ArgExprs;

    IdentifierInfo *KWName = Tok.getIdentifierInfo();
    SourceLocation KWLoc = ConsumeToken();

    BalancedDelimiterTracker T(*this, tok::l_paren);
    if (T.expectAndConsume())
    {
        Diag(Tok, diag::err_expected) << tok::l_brace;
        return;
    }

    do
    {
        if (Tok.is(tok::identifier))
        {
            /*ExprResult StringResult = ParseStringLiteralExpression();
            StringLiteral *SegmentName = cast<StringLiteral>(StringResult.get());
            SegmentName->outputString(llvm::errs());
            llvm::errs() << "\n";*/
            //IdentifierInfo *Ident = Tok.getIdentifierInfo();
            ArgExprs.push_back(ParseIdentifierLoc());
            //llvm::errs() << Ident->getName().str().c_str() << "\n";
        }
    } while(TryConsumeToken(tok::comma));

    if (T.consumeClose())
    {
        Diag(Tok, diag::err_expected) << tok::r_paren;
    }
    *EndLoc = T.getCloseLocation();
    attrs.addNew(KWName, SourceRange(KWLoc, *EndLoc), nullptr, KWLoc, ArgExprs.data(), ArgExprs.size(), ParsedAttr::AS_Keyword);   
}

Parser::DeclGroupPtrTy Parser::ParseRuleset()
{
    assert(Tok.is(tok::kw_ruleset) && "Not a ruleset!");
    
    ParsedAttributesWithRange attrs(AttrFactory);
    SourceLocation RulesetLoc = ConsumeToken();  // eat the 'rulespace'.

    if (Tok.isNot(tok::identifier))
    {
        Diag(Tok, diag::err_expected) << tok::identifier;
        cutOffParsing();
        return nullptr;
    }

    IdentifierInfo *Ident = Tok.getIdentifierInfo();
    SourceLocation identLoc = ConsumeToken();

    if (Tok.is(tok::colon)) // attributes are specified
    {
        ConsumeToken();
        ParseGaiaAttributes(attrs);
    }
        
    BalancedDelimiterTracker T(*this, tok::l_brace);
    if (T.consumeOpen()) 
    {
        Diag(Tok, diag::err_expected) << tok::l_brace;
        return nullptr;
    }
    
    if (getCurScope()->isRulesetScope())
    {
        Diag(T.getOpenLocation(), diag::err_ruleset_ruleset_scope);
        SkipUntil(tok::r_brace);
        return nullptr;
    }

    // Enter a scope for the namespace.
    ParseScope RulesetScope(this, Scope::GaiaRulesetScope);
    Decl *rulesetDecl = Actions.ActOnRulesetDefStart(getCurScope(), RulesetLoc, identLoc, Ident, attrs);

    PrettyDeclStackTraceEntry CrashInfo(Actions.Context, rulesetDecl,
                                      RulesetLoc, "parsing ruleset");

    ParseInnerRuleset(T);

    RulesetScope.Exit();

    Actions.ActOnRulesetDefFinish();

    return Actions.ConvertDeclToDeclGroup(rulesetDecl, nullptr);
}

void Parser::ParseInnerRuleset( BalancedDelimiterTracker &Tracker)
{
    while (Tok.isNot(tok::r_brace) && Tok.isNot(tok::eof)) 
    {
        ParsedAttributesWithRange attrs(AttrFactory);
        ParseExternalDeclaration(attrs);
    }

    // The caller is what called check -- we are simply calling
    // the close for it.
    if (Tracker.consumeClose())
    {
        Diag(Tok, diag::err_expected) << tok::r_brace;
    }
}