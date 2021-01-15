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

void Parser::ConsumeInvalidRuleset()
{
    while(!SkipUntil(tok::l_brace));
    while(!SkipUntil(tok::r_brace));
}

static std::string RandomString(std::string::size_type length)
{
    const char chrs[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
        "abcdefghijklmnopqrstuvwxyz0123456789";

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

// Insert a dummy function declaration to turn rule definition
// to function with special attribute
void Parser::InjectRuleFunction(Declarator &decl)
{
    SourceLocation loc = Tok.getLocation();
    if (!getCurScope()->isRulesetScope())
    {
        Diag(loc, diag::err_ruleset_ruleset_scope);
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
    IdentifierInfo *func = &PP.getIdentifierTable().get(RandomString(15));
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

bool Parser::ParseGaiaAttributeSpecifier(ParsedAttributesWithRange &attrs,
    SourceLocation *EndLoc)
{
    if (Tok.is(tok::comma))
    {
        ConsumeToken();
    }
    if (Tok.is(tok::identifier))
    {
        if (!Tok.getIdentifierInfo()->getName().compare("table"))
        {
            return ParseRulesetTable(attrs, EndLoc);
        }

        if (!Tok.getIdentifierInfo()->getName().compare("SerialStream"))
        {
            return ParseRulesetSerialStream(attrs, EndLoc);
        }
    }
    Diag(Tok, diag::err_invalid_ruleset_attribute);
    return false;
}

/// Parse Gaia Specific attributes
bool Parser::ParseGaiaAttributes(ParsedAttributesWithRange &attrs,
    SourceLocation *endLoc)
{
    SourceLocation StartLoc = Tok.getLocation(), Loc;
    if (!endLoc)
    {
        endLoc = &Loc;
    }

    do
    {
        if (!ParseGaiaAttributeSpecifier(attrs, endLoc))
        {
            return false;
        }
    } while (Tok.is(tok::comma));

    if (Tok.isNot(tok::l_brace))
    {
        Diag(Tok, diag::err_invalid_ruleset_attribute);
        return false;
    }

    attrs.Range = SourceRange(StartLoc, *endLoc);
    return true;
}

bool Parser::ParseRulesetSerialStream(ParsedAttributesWithRange &attrs,
    SourceLocation *endLoc)
{
    assert(!Tok.getIdentifierInfo()->getName().compare("SerialStream") && "Not a ruleset table!");

    ArgsVector argExprs;

    IdentifierInfo *kwName = Tok.getIdentifierInfo();
    SourceLocation kwLoc = ConsumeToken();

    BalancedDelimiterTracker tracker(*this, tok::l_paren);
    if (tracker.consumeOpen())
    {
        Diag(Tok, diag::err_expected) << tok::l_paren;
        return false;
    }


    if (Tok.is(tok::identifier))
    {
        argExprs.push_back(ParseIdentifierLoc());
    }
    else
    {
        Diag(Tok, diag::err_expected) << tok::identifier;
        return false;
    }

    if (tracker.consumeClose())
    {
        return false;
    }

    *endLoc = tracker.getCloseLocation();
    attrs.addNew(kwName, SourceRange(kwLoc, *endLoc), nullptr, kwLoc, argExprs.data(),
        argExprs.size(), ParsedAttr::AS_Keyword);

    if (argExprs.size() == 0)
    {
        Diag(Tok, diag::err_invalid_ruleset_attribute);
        return false;
    }
    return true;
}

bool Parser::ParseRulesetTable(ParsedAttributesWithRange &attrs,
    SourceLocation *endLoc)
{
    assert(!Tok.getIdentifierInfo()->getName().compare("table") && "Not a ruleset table!");

    ArgsVector argExprs;

    IdentifierInfo *kwName = Tok.getIdentifierInfo();
    SourceLocation kwLoc = ConsumeToken();

    BalancedDelimiterTracker tracker(*this, tok::l_paren);
    if (tracker.consumeOpen())
    {
        Diag(Tok, diag::err_expected) << tok::l_paren;
        return false;
    }

    do
    {
        if (Tok.is(tok::identifier))
        {
            argExprs.push_back(ParseIdentifierLoc());
        }
        else
        {
            if (Tok.isNot(tok::r_paren))
            {
                Diag(Tok, diag::err_expected) << tok::identifier;
                return false;
            }
        }
    } while(TryConsumeToken(tok::comma));

    if (tracker.consumeClose())
    {
        return false;
    }
    *endLoc = tracker.getCloseLocation();
    attrs.addNew(kwName, SourceRange(kwLoc, *endLoc), nullptr, kwLoc, argExprs.data(),
        argExprs.size(), ParsedAttr::AS_Keyword);
    if (argExprs.size() == 0)
    {
        Diag(Tok, diag::err_invalid_ruleset_attribute);
        return false;
    }
    return true;
}

Parser::DeclGroupPtrTy Parser::ParseRuleset()
{
    assert(Tok.is(tok::kw_ruleset) && "Not a ruleset!");

    ParsedAttributesWithRange attrs(AttrFactory);
    SourceLocation rulesetLoc = ConsumeToken();  // eat the 'rulespace'.

    if (Tok.isNot(tok::identifier))
    {
        Diag(Tok, diag::err_expected) << tok::identifier;
        ConsumeInvalidRuleset();
        return nullptr;
    }

    IdentifierInfo *ident = Tok.getIdentifierInfo();
    SourceLocation identLoc = ConsumeToken();

    if (Tok.is(tok::colon)) // attributes are specified
    {
        ConsumeToken();
        if (!ParseGaiaAttributes(attrs))
        {
            ConsumeInvalidRuleset();
            return nullptr;
        }
    }

    BalancedDelimiterTracker tracker(*this, tok::l_brace);
    if (tracker.consumeOpen())
    {
        Diag(Tok, diag::err_expected) << tok::l_brace;
        ConsumeInvalidRuleset();
        return nullptr;
    }

    if (getCurScope()->isInRulesetScope())
    {
        Diag(tracker.getOpenLocation(), diag::err_ruleset_ruleset_scope);
        tracker.skipToEnd();
        return nullptr;
    }

    // Enter a scope for the namespace.
    ParseScope rulesetScope(this, Scope::GaiaRulesetScope);
    Decl *rulesetDecl = Actions.ActOnRulesetDefStart(getCurScope(), rulesetLoc, identLoc, ident, attrs);

    PrettyDeclStackTraceEntry CrashInfo(Actions.Context, rulesetDecl,
                                      rulesetLoc, "parsing ruleset");

    ParseRulesetContents(tracker);

    rulesetScope.Exit();

    Actions.ActOnRulesetDefFinish(rulesetDecl, tracker.getCloseLocation());

    return Actions.ConvertDeclToDeclGroup(rulesetDecl, nullptr);
}

void Parser::ParseRulesetContents( BalancedDelimiterTracker &tracker)
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

ExprResult Parser::ParseGaiaThisRule() {
  assert(Tok.is(tok::kw_this_rule) && "Not 'this_rule'!");
  SourceLocation thisRuleLocation = ConsumeToken();
  if (Tok.isNot(tok::period))
  {
      return Diag(thisRuleLocation, diag::err_invalid_this_rule_use);
  }
  return Actions.ActOnGaiaThisRule(thisRuleLocation);
}
