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

#include <string>
#include <unordered_map>
#include <vector>

#include "gaia/db/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"

#include "clang/AST/PrettyDeclStackTrace.h"
#include "clang/Basic/Attributes.h"
#include "clang/Basic/PrettyStackTrace.h"
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/Scope.h"

using namespace gaia;
using namespace std;
using namespace clang;

static string fieldTableName;

static const char *updateVarName = "UPDATE";
static const char *deleteVarName = "DELETE";
static const char *insertVarName = "INSERT";
static const char *noneVarName = "NONE";
static const char *ruleContextTypeName = "rule_context__type";

static QualType mapFieldType(catalog::data_type_t dbType, ASTContext *context)
{
    switch(dbType)
    {
        case catalog::data_type_t::e_bool:
            return context->BoolTy;
        case catalog::data_type_t::e_int8:
            return context->SignedCharTy;
        case catalog::data_type_t::e_uint8:
            return context->UnsignedCharTy;
        case catalog::data_type_t::e_int16:
            return context->ShortTy;
        case catalog::data_type_t::e_uint16:
            return context->UnsignedShortTy;
        case catalog::data_type_t::e_int32:
            return context->IntTy;
        case catalog::data_type_t::e_uint32:
            return context->UnsignedIntTy;
        case catalog::data_type_t::e_int64:
            return context->LongLongTy;
        case catalog::data_type_t::e_uint64:
            return context->UnsignedLongLongTy;
        case catalog::data_type_t::e_float:
            return context->FloatTy;
        case catalog::data_type_t::e_double:
            return context->DoubleTy;
        case catalog::data_type_t::e_string:
            return context->getPointerType((context->CharTy).withConst());
        default:
            return context->VoidTy;
    }
}

class DBMonitor
{
    public:
        DBMonitor()
        {
            gaia::db::begin_session();
            gaia::db::begin_transaction();
        }

        ~DBMonitor()
        {
            gaia::db::commit_transaction();
            gaia::db::end_session();
        }
};

static unordered_map<string, unordered_map<string, QualType>> getTableData(Sema *s, SourceLocation loc)
{
    unordered_map<string, unordered_map<string, QualType>> retVal;
    try
    {
        DBMonitor monitor;

        for(catalog::gaia_field_t field : catalog::gaia_field_t::list())
        {
            catalog::gaia_table_t tbl = field.gaia_table();
            if (!tbl)
            {
                s->Diag(loc, diag::err_invalid_table_field) << field.name();
                return unordered_map<string, unordered_map<string, QualType>>();
            }
            unordered_map<string, QualType> fields = retVal[tbl.name()];
            if (fields.find(field.name()) != fields.end())
            {
                s->Diag(loc, diag::err_duplicate_field) << field.name();
                return unordered_map<string, unordered_map<string, QualType>>();
            }
            fields[field.name()] = mapFieldType(static_cast<catalog::data_type_t>(field.type()), &s->Context);
            retVal[tbl.name()] = fields;
        }
    }
    catch (const exception &e)
    {
        s->Diag(loc, diag::err_catalog_exception) << e.what();
        return unordered_map<string, unordered_map<string, QualType>>();
    }
    return retVal;
}

unordered_set<string> Sema::getCatalogTableList(SourceLocation loc)
{
    unordered_set<string>  retVal;
    try
    {
        DBMonitor monitor;

        for(catalog::gaia_field_t field : catalog::gaia_field_t::list())
        {
            catalog::gaia_table_t tbl = field.gaia_table();
            if (!tbl)
            {
                Diag(loc, diag::err_invalid_table_field) << field.name();
                return unordered_set<string>();
            }
            retVal.emplace(tbl.name());
        }
    }
    catch (const exception &e)
    {
        Diag(loc, diag::err_catalog_exception) << e.what();
        return unordered_set<string>();
    }

    return retVal;
}

void Sema::addField(IdentifierInfo *name, QualType type, RecordDecl *RD, SourceLocation loc) const
{
    FieldDecl *Field = FieldDecl::Create(
        Context, RD, loc, loc,
        name, type, /*TInfo=*/nullptr,
        /*BitWidth=*/nullptr, /*Mutable=*/false, ICIS_NoInit);
    Field->setAccess(AS_public);
    Field->addAttr(GaiaFieldAttr::CreateImplicit(Context));

    RD->addDecl(Field);
}

void Sema::addMethod(IdentifierInfo *name, DeclSpec::TST retValType, DeclaratorChunk::ParamInfo *Params,
    unsigned NumParams, AttributeFactory &attrFactory, ParsedAttributes &attrs, Scope *S, RecordDecl *RD, SourceLocation loc)
{
    DeclSpec DS(attrFactory);
    const char *dummy;
    unsigned diagId;

    Declarator D(DS, DeclaratorContext::MemberContext);
    D.setFunctionDefinitionKind(FDK_Declaration);
    D.getMutableDeclSpec().SetTypeSpecType(retValType, loc, dummy,
        diagId, getPrintingPolicy());
    ActOnAccessSpecifier(AS_public, loc, loc, attrs);

    D.SetIdentifier(name, loc);

    DS.Finish(*this, getPrintingPolicy());

    D.AddTypeInfo(DeclaratorChunk::getFunction(
        true, false, loc, Params,
        NumParams, loc, loc,
        true, loc,
        /*MutableLoc=*/loc,
        EST_None, SourceRange(), nullptr,
        nullptr, 0, nullptr,
        nullptr, None, loc,
        loc, D, TypeResult(), &DS),
        std::move(attrs), loc);

    DeclarationNameInfo NameInfo = GetNameForDeclarator(D);

    TypeSourceInfo *tInfo = GetTypeForDeclarator(D,S);

    CXXMethodDecl *Ret = CXXMethodDecl::Create(
        Context, cast<CXXRecordDecl>(RD), SourceLocation(), NameInfo, tInfo->getType(),
        tInfo, SC_None, false, false, SourceLocation());
    Ret->setAccess(AS_public);
    RD->addDecl(Ret);
}

QualType Sema::getRuleContextType(SourceLocation loc)
{
    // Check if the type has been already created and return the created file
    auto &types = Context.getTypes();
    for (unsigned typeIdx = 0; typeIdx != types.size(); ++typeIdx)
    {
        const auto *type = types[typeIdx];
        const RecordDecl *record = type->getAsRecordDecl();
        if (record != nullptr)
        {
            const auto *id = record->getIdentifier();
            if (id != nullptr)
            {
                if (id->getName().equals(ruleContextTypeName))
                {
                    return QualType(type, 0);
                }
            }
        }
    }

    RecordDecl *RD = Context.buildImplicitRecord(ruleContextTypeName);
    RD->setLexicalDeclContext(CurContext);
    RD->startDefinition();
    Scope S(CurScope,Scope::DeclScope|Scope::ClassScope, Diags);
    ActOnTagStartDefinition(&S,RD);
    ActOnStartCXXMemberDeclarations(getCurScope(), RD, loc,
        false, loc);
    AttributeFactory attrFactory;
    ParsedAttributes attrs(attrFactory);


    //insert fields
    addField(&Context.Idents.get("ruleset_name"), Context.getPointerType((Context.CharTy.withConst()).withConst()), RD, loc);
    addField(&Context.Idents.get("rule_name"), Context.getPointerType((Context.CharTy.withConst()).withConst()), RD, loc);
    addField(&Context.Idents.get("event_type"), Context.UnsignedIntTy.withConst(), RD, loc);
    addField(&Context.Idents.get("gaia_type"), Context.UnsignedIntTy.withConst(), RD, loc);

    ActOnFinishCXXMemberSpecification(getCurScope(), loc, RD,
        loc, loc, attrs);
    ActOnTagFinishDefinition(getCurScope(), RD, SourceRange());

    return Context.getTagDeclType(RD);
}


QualType Sema::getTableType (IdentifierInfo *table, SourceLocation loc)
{
    std::string tableName = table->getName().str();
    DeclContext *c = getCurFunctionDecl();
    while (c)
    {
        if (isa<RulesetDecl>(c))
        {
            break;
        }
        c = c->getParent();
    }

    if (c == nullptr || !isa<RulesetDecl>(c))
    {
        Diag(loc, diag::err_no_ruleset_for_rule);
        return Context.VoidTy;
    }

    RulesetDecl *rulesetDecl = dyn_cast<RulesetDecl>(c);
    RulesetTableAttr *attr = rulesetDecl->getAttr<RulesetTableAttr>();

    if (attr != nullptr)
    {
        bool table_found = false;
        for (const IdentifierInfo * id : attr->tables())
        {
            if (id->getName().str() == tableName)
            {
                table_found = true;
                break;
            }

        }

        if (!table_found)
        {
            Diag(loc, diag::warn_table_referenced_not_in_table_attribute) << tableName;
        }
    }

    unordered_map<string, unordered_map<string, QualType>> tableData = getTableData(this, loc);
    auto tableDescription = tableData.find(tableName);
    if (tableDescription == tableData.end())
    {
        Diag(loc,diag::err_invalid_table_name) << tableName;
        return Context.VoidTy;
    }

    RecordDecl *RD = Context.buildImplicitRecord(tableName + "__type");
    RD->setLexicalDeclContext(CurContext);
    RD->startDefinition();
    Scope S(CurScope,Scope::DeclScope|Scope::ClassScope, Diags);
    ActOnTagStartDefinition(&S,RD);
    ActOnStartCXXMemberDeclarations(getCurScope(), RD, loc,
        false, loc);
    AttributeFactory attrFactory;
    ParsedAttributes attrs(attrFactory);

    for (const auto &f : tableDescription->second)
    {
        string fieldName = f.first;
        QualType fieldType = f.second;
        if (fieldType->isVoidType())
        {
            Diag(loc,diag::err_invalid_field_type) << fieldName;
            return Context.VoidTy;
        }
        addField(&Context.Idents.get(fieldName), fieldType, RD, loc);
    }

    //insert fields and methods that are not part of the schema
    addField(&Context.Idents.get("LastOperation"), Context.IntTy, RD, loc);

    addMethod(&Context.Idents.get("delete_row"), DeclSpec::TST_void, nullptr, 0, attrFactory, attrs, &S, RD, loc);
    addMethod(&Context.Idents.get("gaia_id"), DeclSpec::TST_int, nullptr, 0, attrFactory, attrs, &S, RD, loc);

    ActOnFinishCXXMemberSpecification(getCurScope(), loc, RD,
        loc, loc, attrs);
    ActOnTagFinishDefinition(getCurScope(), RD, SourceRange());

    return Context.getTagDeclType(RD);
}

QualType Sema::getFieldType (IdentifierInfo *id, SourceLocation loc)
{
    StringRef fieldNameStrRef = id->getName();

    if(!fieldNameStrRef.compare(updateVarName) ||
         !fieldNameStrRef.compare(deleteVarName) ||
         !fieldNameStrRef.compare(insertVarName) ||
         !fieldNameStrRef.compare(noneVarName))
    {
        return Context.IntTy.withConst();
    }

    std::string fieldName = fieldNameStrRef.str();

    DeclContext *c = getCurFunctionDecl();
    while (c)
    {
        if (isa<RulesetDecl>(c))
        {
            break;
        }
        c = c->getParent();
    }

    if (c == nullptr || !isa<RulesetDecl>(c))
    {
        Diag(loc, diag::err_no_ruleset_for_rule);
        return Context.VoidTy;
    }
    vector<string> tables;
    RulesetDecl *rulesetDecl = dyn_cast<RulesetDecl>(c);
    RulesetTableAttr * attr = rulesetDecl->getAttr<RulesetTableAttr>();
    unordered_map<string, unordered_map<string, QualType>> tableData = getTableData(this, loc);

    if (attr != nullptr)
    {
        for (const IdentifierInfo * id : attr->tables())
        {
            tables.push_back(id->getName().str());
        }
    }
    else
    {
        for (auto it : tableData)
        {
            tables.push_back(it.first);
        }
    }

    QualType retVal = Context.VoidTy;
    for (string tableName : tables)
    {
        auto tableDescription = tableData.find(tableName);
        if (tableDescription == tableData.end())
        {
            Diag(loc,diag::err_invalid_table_name) << tableName;
            return Context.VoidTy;
        }
        auto fieldDescription = tableDescription->second.find(fieldName);
        if(fieldDescription != tableDescription->second.end())
        {
            if (fieldDescription->second->isVoidType())
            {
                Diag(loc, diag::err_invalid_field_type) << fieldNameStrRef;
                return Context.VoidTy;
            }
            if (retVal != Context.VoidTy)
            {
                Diag(loc, diag::err_duplicate_field) << fieldNameStrRef;
                return Context.VoidTy;
            }

            retVal = fieldDescription->second;
            fieldTableName = tableName;
        }
    }

    if (retVal == Context.VoidTy)
    {
        Diag(loc, diag::err_unknown_field) << fieldNameStrRef;
    }

    return retVal;
}

NamedDecl *Sema::injectVariableDefinition(IdentifierInfo *II, SourceLocation loc, bool isGaiaFieldTable)
{
    QualType qualType = isGaiaFieldTable ? getTableType(II, loc) : getFieldType(II, loc);
    if (qualType->isVoidType())
    {
        return nullptr;
    }

    StringRef varName = II->getName();

    DeclContext *context  = getCurFunctionDecl();

    VarDecl *varDecl = VarDecl::Create(Context, context, loc, loc,
                            II, qualType, Context.getTrivialTypeSourceInfo(qualType, loc), SC_None);
    varDecl->setLexicalDeclContext(context);
    varDecl->setImplicit();

    if (!varName.compare(updateVarName))
    {
        varDecl->addAttr(GaiaLastOperationUPDATEAttr::CreateImplicit(Context));
        varDecl->setInit(ActOnIntegerConstant(loc, 0).get());
    }
    else if (!varName.compare(insertVarName))
    {
        varDecl->addAttr(GaiaLastOperationINSERTAttr::CreateImplicit(Context));
        varDecl->setInit(ActOnIntegerConstant(loc, 1).get());
    }
    else if (!varName.compare(deleteVarName))
    {
        varDecl->addAttr(GaiaLastOperationDELETEAttr::CreateImplicit(Context));
        varDecl->setInit(ActOnIntegerConstant(loc, 2).get());
    }
    else if (!varName.compare(noneVarName))
    {
        varDecl->addAttr(GaiaLastOperationNONEAttr::CreateImplicit(Context));
        varDecl->setInit(ActOnIntegerConstant(loc, 3).get());
    }
    else if (isGaiaFieldTable)
    {
        varDecl->addAttr(GaiaFieldAttr::CreateImplicit(Context));
        varDecl->addAttr(FieldTableAttr::CreateImplicit(Context, &Context.Idents.get(fieldTableName)));
    }
    else
    {
        varDecl->addAttr(GaiaFieldAttr::CreateImplicit(Context));
        varDecl->addAttr(FieldTableAttr::CreateImplicit(Context, &Context.Idents.get(fieldTableName)));
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

ExprResult Sema::ActOnGaiaRuleContext(SourceLocation Loc)
{
  QualType ruleContextType = getRuleContextType(Loc);
  if (ruleContextType.isNull())
  {
    return Diag(Loc, diag::err_invalid_rule_context_internal_error);
  }

  return new (Context) GaiaRuleContextExpr(Loc, ruleContextType);
}
