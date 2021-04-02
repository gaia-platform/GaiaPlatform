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

struct ExplicitPath
{
    SourceLocation startLocation;
    SourceLocation endLocation;
    std::string path;
};


std::string Parser::GetExplicitNavigationPath()
{
    ExplicitPath path;
    std::string returnValue;
    if (Tok.isNot(tok::identifier))
    {
        return returnValue;
    }

    if (getPreviousToken(Tok).is(tok::slash))
    {
        returnValue = "/";
        path.startLocation = getPreviousToken(Tok).getLocation();
    }
    else
    {
        path.startLocation = Tok.getLocation();
    }
    returnValue.append(Tok.getIdentifierInfo()->getName().str());
    unsigned tokenIterator = 1;

    while (GetLookAheadToken(tokenIterator).isOneOf(tok::arrow, tok::colon, tok::period, tok::identifier))
    {
        switch (GetLookAheadToken(tokenIterator).getKind())
        {
            case tok::arrow:
                returnValue.append("->");
                break;
            case tok::colon:
                returnValue.append(":");
                break;
            case tok::period:
                returnValue.append(".");
                break;
            case tok::identifier:
                returnValue.append(GetLookAheadToken(tokenIterator).getIdentifierInfo()->getName().str());
                break;
            default:
                break;
        }
        ++tokenIterator;
    }
    path.endLocation = GetLookAheadToken(tokenIterator - 1).getEndLocation();
    return returnValue;
}

// Insert a dummy function declaration to turn rule definition
// to function with special attribute
void Parser::InjectRuleFunction(Declarator &decl, ParsedAttributesWithRange &attrs)
{
    SourceLocation loc = Tok.getLocation();
    if (!getCurScope()->isRulesetScope())
    {
        Diag(loc, diag::err_ruleset_ruleset_scope);
        return;
    }

    SourceLocation endLoc;
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

bool Parser::ParseGaiaAttributeSpecifier(ParsedAttributesWithRange &attrs, GaiaAttributeType attributeType,
    SourceLocation *EndLoc)
{
    if (Tok.is(tok::comma))
    {
        ConsumeToken();
    }
    if (Tok.is(tok::identifier))
    {
        if (attributeType == Ruleset)
        {
            if (Tok.getIdentifierInfo()->getName().equals("Table"))
            {
                return ParseRulesetTable(attrs, EndLoc);
            }

            if (Tok.getIdentifierInfo()->getName().equals("SerialStream"))
            {
                return ParseRulesetSerialStream(attrs, EndLoc);
            }
        }
        else if (attributeType == Rule)
        {
            if (Tok.getIdentifierInfo()->getName().equals(c_on_update_rule_attribute) ||
                Tok.getIdentifierInfo()->getName().equals(c_on_insert_rule_attribute) ||
                Tok.getIdentifierInfo()->getName().equals(c_on_change_rule_attribute))
            {
                return ParseRuleSubscriptionAttributes(attrs, EndLoc);
            }
        }

    }
    Diag(Tok, diag::err_invalid_Gaia_attribute);
    return false;
}

bool Parser::ParseRuleSubscriptionAttributes(ParsedAttributesWithRange &attrs,
    SourceLocation *endLoc)
{
    ArgsVector argExprs;

    IdentifierInfo *attributeName = Tok.getIdentifierInfo();
    SourceLocation attributeLocation = ConsumeToken();

    BalancedDelimiterTracker tracker(*this, tok::l_paren);
    if (tracker.consumeOpen())
    {
        Diag(Tok, diag::err_expected) << tok::l_paren;
        return false;
    }

    std::unordered_set<std::string> tags;
    do
    {
        SourceLocation tokenLocation = Tok.getLocation();
        if (Tok.is(tok::identifier))
        {
            std::string table = Tok.getIdentifierInfo()->getName().str();
            if (NextToken().is(tok::colon))
            {
                if (tags.find(table) != tags.end())
                {
                    Diag(Tok, diag::err_duplicate_tag_defined) << table;
                    return false;
                }
                else
                {
                    tags.emplace(Tok.getIdentifierInfo()->getName().str());
                }
                table += std::string(":");
                ConsumeToken();
                if (NextToken().is(tok::identifier))
                {
                    ConsumeToken();
                    table += Tok.getIdentifierInfo()->getName().str();
                }
                else
                {
                    Diag(Tok, diag::err_expected) << tok::identifier;
                    return false;
                }
            }

            if (NextToken().is(tok::period))
            {
                ConsumeToken();
                if (NextToken().is(tok::identifier))
                {
                    ConsumeToken();
                    table += std::string(".") + Tok.getIdentifierInfo()->getName().str();
                }
                else
                {
                    Diag(Tok, diag::err_expected) << tok::identifier;
                    return false;
                }
            }
            table = std::string("\"") + table + std::string("\"");
            Token Toks[1];
            Toks[0].startToken();
            Toks[0].setKind(tok::string_literal);
            Toks[0].setLocation(tokenLocation);
            Toks[0].setLiteralData(table.data());
            Toks[0].setLength(table.size());

            StringLiteral *tableString =
                cast<StringLiteral>(Actions.ActOnStringLiteral(Toks, nullptr).get());

            argExprs.push_back(tableString);
            ConsumeToken();
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
    if (argExprs.size() == 0)
    {
        Diag(Tok, diag::err_invalid_Gaia_rule_attribute);
        return false;
    }

    *endLoc = tracker.getCloseLocation();
    attrs.addNew(attributeName, SourceRange(attributeLocation, *endLoc), nullptr, attributeLocation, argExprs.data(),
        argExprs.size(), ParsedAttr::AS_Keyword);

    return true;
}

/// Parse Gaia Specific attributes
bool Parser::ParseGaiaAttributes(ParsedAttributesWithRange &attrs, GaiaAttributeType attributeType,
    SourceLocation *endLoc)
{
    SourceLocation StartLoc = Tok.getLocation(), Loc;
    if (!endLoc)
    {
        endLoc = &Loc;
    }

    do
    {
        if (!ParseGaiaAttributeSpecifier(attrs, attributeType, endLoc))
        {
            return false;
        }
    } while (Tok.is(tok::comma));

    if (Tok.isNot(tok::l_brace))
    {
        Diag(Tok, diag::err_invalid_Gaia_attribute);
        return false;
    }

    attrs.Range = SourceRange(StartLoc, *endLoc);
    return true;
}

bool Parser::ParseRulesetSerialStream(ParsedAttributesWithRange &attrs,
    SourceLocation *endLoc)
{
    assert(Tok.getIdentifierInfo()->getName().equals("SerialStream") && "Not a ruleset table!");

    ArgsVector argExprs;

    IdentifierInfo *attributeName = Tok.getIdentifierInfo();
    SourceLocation attributeLocation = ConsumeToken();

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

    if (argExprs.size() == 0)
    {
        Diag(Tok, diag::err_invalid_ruleset_attribute);
        return false;
    }

    *endLoc = tracker.getCloseLocation();
    attrs.addNew(attributeName, SourceRange(attributeLocation, *endLoc), nullptr, attributeLocation, argExprs.data(),
        argExprs.size(), ParsedAttr::AS_Keyword);

    return true;
}

bool Parser::ParseRulesetTable(ParsedAttributesWithRange &attrs,
    SourceLocation *endLoc)
{
    assert(!Tok.getIdentifierInfo()->getName().equals("Table") && "Not a ruleset table!");

    ArgsVector argExprs;

    IdentifierInfo *attributeName = Tok.getIdentifierInfo();
    SourceLocation attributeLocation = ConsumeToken();

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
    if (argExprs.size() == 0)
    {
        Diag(Tok, diag::err_invalid_ruleset_attribute);
        return false;
    }

    *endLoc = tracker.getCloseLocation();
    attrs.addNew(attributeName, SourceRange(attributeLocation, *endLoc), nullptr, attributeLocation, argExprs.data(),
        argExprs.size(), ParsedAttr::AS_Keyword);

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
        if (!ParseGaiaAttributes(attrs, Ruleset))
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

ExprResult Parser::ParseGaiaRuleContext()
{
    assert(Tok.is(tok::kw_rule_context) && "Not 'rule_context'!");
    SourceLocation ruleContextLocation = ConsumeToken();
    if (Tok.isNot(tok::period))
    {
        return Diag(ruleContextLocation, diag::err_invalid_rule_context_use);
    }
    return Actions.ActOnGaiaRuleContext(ruleContextLocation);
}

Token Parser::getPreviousToken(Token token) const
{
    Token returnToken;
    returnToken.setKind(tok::unknown);
    SourceLocation location = token.getLocation().getLocWithOffset(-1);
    SourceManager &sourceManager = PP.getSourceManager();
    auto langOptions = getLangOpts();
    auto StartOfFile = sourceManager.getLocForStartOfFile(sourceManager.getFileID(location));
    while (location != StartOfFile)
    {
        location = Lexer::GetBeginningOfToken(location, sourceManager, langOptions);
        if (!Lexer::getRawToken(location, returnToken, sourceManager, langOptions) &&
            !returnToken.is(tok::comment))
        {
            break;
        }
        location = location.getLocWithOffset(-1);
    }
    return returnToken;
}

void Parser::ParseRule(Declarator &D)
{
    if (Tok.isNot(tok::identifier))
    {
        Diag(Tok, diag::err_expected) << tok::identifier;
        return;
    }
    ParsedAttributesWithRange attrs(AttrFactory);
    if (!ParseGaiaAttributes(attrs, Rule))
    {
        return;
    }
    InjectRuleFunction(D, attrs);
}
