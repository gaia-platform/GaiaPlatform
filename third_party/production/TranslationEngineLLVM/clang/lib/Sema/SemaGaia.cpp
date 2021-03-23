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

#include "gaia_internal/catalog/catalog.hpp"
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

static const char ruleContextTypeName[] = "rule_context__type";

static QualType mapFieldType(catalog::data_type_t dbType, ASTContext *context)
{
    // Clang complains if we add a default clause to a switch that covers all values of an enum,
    // so this code is written to avoid that.
    QualType returnType = context->VoidTy;

    switch(dbType)
    {
        case catalog::data_type_t::e_bool:
            returnType = context->BoolTy;
            break;
        case catalog::data_type_t::e_int8:
            returnType = context->SignedCharTy;
            break;
        case catalog::data_type_t::e_uint8:
            returnType = context->UnsignedCharTy;
            break;
        case catalog::data_type_t::e_int16:
            returnType = context->ShortTy;
            break;
        case catalog::data_type_t::e_uint16:
            returnType = context->UnsignedShortTy;
            break;
        case catalog::data_type_t::e_int32:
            returnType = context->IntTy;
            break;
        case catalog::data_type_t::e_uint32:
            returnType = context->UnsignedIntTy;
            break;
        case catalog::data_type_t::e_int64:
            returnType = context->LongLongTy;
            break;
        case catalog::data_type_t::e_uint64:
            returnType = context->UnsignedLongLongTy;
            break;
        case catalog::data_type_t::e_float:
            returnType = context->FloatTy;
            break;
        case catalog::data_type_t::e_double:
            returnType = context->DoubleTy;
            break;
        case catalog::data_type_t::e_string:
            returnType = context->getPointerType((context->CharTy).withConst());
            break;
    }

    // We should not be reaching this line with this value,
    // unless there is an error in code.
    assert(returnType != context->VoidTy);

    return returnType;
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

unordered_map<string, unordered_map<string, QualType>> Sema::getTableData(SourceLocation loc)
{
    unordered_map<string, unordered_map<string, QualType>> retVal;
    try
    {
        DBMonitor monitor;

        for(const catalog::gaia_field_t &field : catalog::gaia_field_t::list())
        {
            catalog::gaia_table_t tbl = field.gaia_table();
            if (!tbl)
            {
                Diag(loc, diag::err_invalid_table_field) << field.name();
                return unordered_map<string, unordered_map<string, QualType>>();
            }
            unordered_map<string, QualType> fields = retVal[tbl.name()];
            if (fields.find(field.name()) != fields.end())
            {
                Diag(loc, diag::err_duplicate_field) << field.name();
                return unordered_map<string, unordered_map<string, QualType>>();
            }
            fields[field.name()] = mapFieldType(static_cast<catalog::data_type_t>(field.type()), &Context);
            retVal[tbl.name()] = fields;
        }
    }
    catch (const exception &e)
    {
        Diag(loc, diag::err_catalog_exception) << e.what();
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

        for(const catalog::gaia_field_t &field : catalog::gaia_field_t::list())
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


QualType Sema::getTableType (const std::string &tableName, SourceLocation loc)
{
    DeclContext *context = getCurFunctionDecl();
    while (context)
    {
        if (isa<RulesetDecl>(context))
        {
            break;
        }
        context = context->getParent();
    }

    if (context == nullptr || !isa<RulesetDecl>(context))
    {
        Diag(loc, diag::err_no_ruleset_for_rule);
        return Context.VoidTy;
    }
    std::string typeName = tableName;
    std::unordered_map<std::string, std::string> tagMapping = getTagMapping(getCurFunctionDecl());
    if (tagMapping.find(tableName) != tagMapping.end())
    {
        typeName = tagMapping[tableName];
    }
    fieldTableName = typeName;
    const Type *realType = nullptr;

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
                //check if EDC type is defined
                if (id->getName().equals(typeName + "_t"))
                {
                    realType = type;
                    break;
                }
            }
        }
    }

    RulesetDecl *rulesetDecl = dyn_cast<RulesetDecl>(context);
    RulesetTableAttr *attr = rulesetDecl->getAttr<RulesetTableAttr>();

    if (attr != nullptr)
    {
        bool table_found = false;
        for (const IdentifierInfo *id : attr->tables())
        {
            if (id->getName().str() == typeName)
            {
                table_found = true;
                break;
            }
        }

        if (!table_found)
        {
            Diag(loc, diag::warn_table_referenced_not_in_table_attribute) << typeName;
        }
    }

    unordered_map<string, unordered_map<string, QualType>> tableData = getTableData(loc);
    auto tableDescription = tableData.find(typeName);
    if (tableDescription == tableData.end())
    {
        Diag(loc,diag::err_invalid_table_name) << typeName;
        return Context.VoidTy;
    }

    RecordDecl *RD = Context.buildImplicitRecord(typeName + "__type");
    RD->setLexicalDeclContext(CurContext);
    RD->startDefinition();
    Scope S(CurScope,Scope::DeclScope|Scope::ClassScope, Diags);
    ActOnTagStartDefinition(&S,RD);
    ActOnStartCXXMemberDeclarations(getCurScope(), RD, loc,
        false, loc);
    AttributeFactory attrFactory;
    ParsedAttributes attrs(attrFactory);


    if (realType != nullptr)
    {
        QualType R = Context.getFunctionType(
            BuildReferenceType(QualType(realType,0), true, loc, DeclarationName()),
            None, FunctionProtoType::ExtProtoInfo());
        CanQualType ClassType = Context.getCanonicalType(R);
        DeclarationName Name = Context.DeclarationNames.getCXXConversionFunctionName(ClassType);
        DeclarationNameInfo NameInfo(Name, loc);

        auto conversionFunctionDeclaration = CXXConversionDecl::Create(
            Context, cast<CXXRecordDecl>(RD), loc, NameInfo, R,
            nullptr, false, false, false, SourceLocation());

        conversionFunctionDeclaration->setAccess(AS_public);
        RD->addDecl(conversionFunctionDeclaration);
    }

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
    addMethod(&Context.Idents.get("delete_row"), DeclSpec::TST_void, nullptr, 0, attrFactory, attrs, &S, RD, loc);
    addMethod(&Context.Idents.get("gaia_id"), DeclSpec::TST_int, nullptr, 0, attrFactory, attrs, &S, RD, loc);

    ActOnFinishCXXMemberSpecification(getCurScope(), loc, RD,
        loc, loc, attrs);
    ActOnTagFinishDefinition(getCurScope(), RD, SourceRange());

    return Context.getTagDeclType(RD);
}

QualType Sema::getFieldType (const std::string &fieldName, SourceLocation loc)
{
    DeclContext *context = getCurFunctionDecl();
    unordered_map<string, unordered_map<string, QualType>> tableData = getTableData(loc);
    std::unordered_map<std::string, std::string> tagMapping = getTagMapping(getCurFunctionDecl());

    if (tableData.find(fieldName) != tableData.end() || tagMapping.find(fieldName) != tagMapping.end())
    {
        for (auto iterator : tableData)
        {
            if (iterator.second.find(fieldName) != iterator.second.end())
            {
                Diag(loc,diag::err_ambiguous_field_name) << fieldName;
                return Context.VoidTy;
            }
        }

        if (tableData.find(fieldName) != tableData.end())
        {
            return getTableType(fieldName, loc);
        }
        else
        {
            return getTableType(tagMapping[fieldName], loc);
        }
    }

    while (context)
    {
        if (isa<RulesetDecl>(context))
        {
            break;
        }
        context = context->getParent();
    }

    if (context == nullptr || !isa<RulesetDecl>(context))
    {
        Diag(loc, diag::err_no_ruleset_for_rule);
        return Context.VoidTy;
    }
    vector<string> tables;
    RulesetDecl *rulesetDecl = dyn_cast<RulesetDecl>(context);
    RulesetTableAttr * attr = rulesetDecl->getAttr<RulesetTableAttr>();

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
                Diag(loc, diag::err_invalid_field_type) << fieldName;
                return Context.VoidTy;
            }
            if (retVal != Context.VoidTy)
            {
                Diag(loc, diag::err_duplicate_field) << fieldName;
                return Context.VoidTy;
            }

            retVal = fieldDescription->second;
            fieldTableName = tableName;
        }
    }

    if (retVal == Context.VoidTy)
    {
        Diag(loc, diag::err_unknown_field) << fieldName;
    }

    return retVal;
}


static bool parse_attribute(const string& attribute, string& table, string& tag)
{
    string tagless_attribute;
    size_t tag_position = attribute.find(':');
    if (tag_position != string::npos)
    {
        tag = attribute.substr(0, tag_position);
        tagless_attribute = attribute.substr(tag_position + 1);
    }
    else
    {
        return false;
    }
    size_t dot_position = tagless_attribute.find('.');
    // Handle fully qualified reference.
    if (dot_position != string::npos)
    {
        table = tagless_attribute.substr(0, dot_position);
        return true;
    }

    table = tagless_attribute;
    return true;
}

std::unordered_map<std::string, std::string> Sema::getTagMapping(const DeclContext *context) const
{
    std::unordered_map<std::string, std::string> retVal;
    const FunctionDecl *ruleContext = dyn_cast<FunctionDecl>(context);
    if (ruleContext == nullptr)
    {
         return retVal;
    }

    const GaiaOnUpdateAttr* update_attribute = ruleContext->getAttr<GaiaOnUpdateAttr>();
    const GaiaOnInsertAttr* insert_attribute = ruleContext->getAttr<GaiaOnInsertAttr>();
    const GaiaOnChangeAttr* change_attribute = ruleContext->getAttr<GaiaOnChangeAttr>();

    if (update_attribute != nullptr)
    {
        for (const auto& table_iterator : update_attribute->tables())
        {
            string table, tag;
            if (parse_attribute(table_iterator, table, tag))
            {
                retVal[tag] = table;
            }
        }
    }

    if (insert_attribute != nullptr)
    {
        for (const auto& table_iterator : insert_attribute->tables())
        {
            string table, tag;
            if (parse_attribute(table_iterator, table, tag))
            {
                retVal[tag] = table;
            }
        }
    }

    if (change_attribute != nullptr)
    {
        for (const auto& table_iterator : change_attribute->tables())
        {
            string table, tag;
            if (parse_attribute(table_iterator, table, tag))
            {
                retVal[tag] = table;
            }
        }
    }
    return retVal;
}

NamedDecl *Sema::injectVariableDefinition(IdentifierInfo *II, SourceLocation loc, bool isGaiaFieldTable)
{
    QualType qualType = isGaiaFieldTable ? getTableType(II->getName().str(), loc) :
        getFieldType(II->getName().str(), loc);
    if (qualType->isVoidType())
    {
        return nullptr;
    }
    DeclContext *context  = getCurFunctionDecl();

    VarDecl *varDecl = VarDecl::Create(Context, context, loc, loc,
                            II, qualType, Context.getTrivialTypeSourceInfo(qualType, loc), SC_None);
    varDecl->setLexicalDeclContext(context);
    varDecl->setImplicit();

    varDecl->addAttr(GaiaFieldAttr::CreateImplicit(Context));
    varDecl->addAttr(FieldTableAttr::CreateImplicit(Context, &Context.Idents.get(fieldTableName)));

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
