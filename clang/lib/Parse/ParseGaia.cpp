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
#include "clang/Parse/Parser.h"
#include "clang/Parse/RAIIObjectsForParser.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/TypoCorrection.h"
#include <random>
using namespace clang;


std::string Parser::randomString(std::string::size_type length) const
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

void Parser::injectRuleFunction(Declarator &decl)
{
    SourceLocation loc = Tok.getLocation();
    if (!getCurScope()->isRulesetScope())
    {
        Diag(loc, diag::err_ruleset_ruleset_scope);
        cutOffParsing();;
        return;
    }

    SourceLocation endLoc;
    ParsedAttributesWithRange attrs(AttrFactory);
    attrs.addNew(&PP.getIdentifierTable().get("rule"),
        SourceRange(loc, loc),
        nullptr, loc, nullptr, 0,
        ParsedAttr::AS_Keyword); 

    // Set return type
    const char *prevSpec = nullptr;
    unsigned int diagId = 0;
    decl.getMutableDeclSpec().SetTypeSpecType(DeclSpec::TST_void, loc, prevSpec,
        diagId, Actions.getPrintingPolicy());
    decl.getMutableDeclSpec().takeAttributesFrom(attrs);

    DeclSpec declSpec(AttrFactory);
    declSpec.takeAttributesFrom(attrs);

    declSpec.Finish(Actions, Actions.getASTContext().getPrintingPolicy());
 
    // Set function name
    IdentifierInfo *func = &PP.getIdentifierTable().get(randomString(10));
    decl.SetIdentifier(func, loc);
    
    decl.takeAttributes(attrs, endLoc);
    decl.AddTypeInfo(DeclaratorChunk::getFunction(
                    true, false, loc, nullptr,
                    0, loc, loc,
                    true, SourceLocation(),
                    /*MutableLoc=*/SourceLocation(),
                    EST_None, SourceRange(), nullptr,
                    nullptr, 0,
                    nullptr,
                    nullptr, None, loc,
                    loc, decl, TypeResult(), &declSpec),
                std::move(attrs), loc);
}

void Parser::parseGaiaAttributeSpecifier(ParsedAttributesWithRange &attrs,
    SourceLocation *EndLoc)
{
    if (Tok.is(tok::comma))
    {
        ConsumeToken();
    }
    switch (Tok.getKind())
    {
        case tok::kw_table:
            parseRulesetTable(attrs, EndLoc);
            return;
        default:
            Diag(Tok, diag::err_invalid_ruleset_attribute);
            cutOffParsing();
    }    
}

/// Parse Gaia Specific attributes
void Parser::parseGaiaAttributes(ParsedAttributesWithRange &attrs,
    SourceLocation *endLoc)
{
    SourceLocation StartLoc = Tok.getLocation(), Loc;
    if (!endLoc)
    {
        endLoc = &Loc;
    }

    do 
    {
        parseGaiaAttributeSpecifier(attrs, endLoc);
    } while (Tok.is(tok::comma));

    attrs.Range = SourceRange(StartLoc, *endLoc);
}

void Parser::parseRulesetTable(ParsedAttributesWithRange &attrs,
    SourceLocation *endLoc)
{
    assert(Tok.is(tok::kw_table) && "Not a ruleset table!");

    ArgsVector argExprs;

    IdentifierInfo *kwName = Tok.getIdentifierInfo();
    SourceLocation kwLoc = ConsumeToken();

    BalancedDelimiterTracker tracker(*this, tok::l_paren);
    if (tracker.expectAndConsume())
    {
        Diag(Tok, diag::err_expected) << tok::l_brace;
        return;
    }

    do
    {
        if (Tok.is(tok::identifier))
        {
            argExprs.push_back(ParseIdentifierLoc());
        }
    } while(TryConsumeToken(tok::comma));

    if (tracker.consumeClose())
    {
        Diag(Tok, diag::err_expected) << tok::r_paren;
    }
    *endLoc = tracker.getCloseLocation();
    attrs.addNew(kwName, SourceRange(kwLoc, *endLoc), nullptr, kwLoc, argExprs.data(), 
        argExprs.size(), ParsedAttr::AS_Keyword);   
}

Parser::DeclGroupPtrTy Parser::parseRuleset()
{
    assert(Tok.is(tok::kw_ruleset) && "Not a ruleset!");
    
    ParsedAttributesWithRange attrs(AttrFactory);
    SourceLocation rulesetLoc = ConsumeToken();  // eat the 'rulespace'.

    if (Tok.isNot(tok::identifier))
    {
        Diag(Tok, diag::err_expected) << tok::identifier;
        cutOffParsing();
        return nullptr;
    }

    IdentifierInfo *ident = Tok.getIdentifierInfo();
    SourceLocation identLoc = ConsumeToken();

    if (Tok.is(tok::colon)) // attributes are specified
    {
        ConsumeToken();
        parseGaiaAttributes(attrs);
    }
        
    BalancedDelimiterTracker tracker(*this, tok::l_brace);
    if (tracker.consumeOpen()) 
    {
        Diag(Tok, diag::err_expected) << tok::l_brace;
        return nullptr;
    }
    
    if (getCurScope()->isRulesetScope())
    {
        Diag(tracker.getOpenLocation(), diag::err_ruleset_ruleset_scope);
        SkipUntil(tok::r_brace);
        return nullptr;
    }

    // Enter a scope for the namespace.
    ParseScope rulesetScope(this, Scope::GaiaRulesetScope);
    Decl *rulesetDecl = Actions.ActOnRulesetDefStart(getCurScope(), rulesetLoc, identLoc, ident, attrs);

    PrettyDeclStackTraceEntry CrashInfo(Actions.Context, rulesetDecl,
                                      rulesetLoc, "parsing ruleset");

    parseInnerRuleset(tracker);

    rulesetScope.Exit();

    Actions.ActOnRulesetDefFinish();

    return Actions.ConvertDeclToDeclGroup(rulesetDecl, nullptr);
}

void Parser::parseInnerRuleset( BalancedDelimiterTracker &tracker)
{
    while (Tok.isNot(tok::r_brace) && Tok.isNot(tok::eof)) 
    {
        ParsedAttributesWithRange attrs(AttrFactory);
        ParseExternalDeclaration(attrs);
    }

    // The caller is what called check -- we are simply calling
    // the close for it.
    if (tracker.consumeClose())
    {
        Diag(Tok, diag::err_expected) << tok::r_brace;
    }
}