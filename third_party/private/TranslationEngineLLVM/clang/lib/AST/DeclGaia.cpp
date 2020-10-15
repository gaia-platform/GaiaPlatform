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

#include "clang/AST/Decl.h"
using namespace clang;



RulesetDecl *RulesetDecl::Create(ASTContext &C, DeclContext *DC,
    SourceLocation StartLoc, SourceLocation IdLoc, IdentifierInfo *Id)
{
    return new (C, DC) RulesetDecl(C, DC, StartLoc, IdLoc, Id);
}

RulesetDecl *RulesetDecl::CreateDeserialized(ASTContext &C, unsigned ID)
{
    return new (C, ID) RulesetDecl(C, nullptr, SourceLocation(), SourceLocation(),
        nullptr);
}
