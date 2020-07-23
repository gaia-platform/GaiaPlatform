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
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/Scope.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "storage_engine.hpp"
#include "catalog_gaia_generated.h"

using namespace gaia;
using namespace std;
using namespace clang;

static string fieldTableName;

static QualType mapFieldType(catalog::gaia_data_type dbType, ASTContext *context)
{
    switch(dbType)
    {
        case catalog::gaia_data_type_BOOL:
            return context->BoolTy;
        case catalog::gaia_data_type_INT8:
            return context->SignedCharTy;
        case catalog::gaia_data_type_UINT8:
            return context->UnsignedCharTy;
        case catalog::gaia_data_type_INT16:
            return context->ShortTy;
        case catalog::gaia_data_type_UINT16:
            return context->UnsignedShortTy;
        case catalog::gaia_data_type_INT32:
            return context->IntTy;
        case catalog::gaia_data_type_UINT32:
            return context->UnsignedIntTy;
        case catalog::gaia_data_type_INT64:
            return context->LongLongTy;
        case catalog::gaia_data_type_UINT64:
            return context->UnsignedLongLongTy;
        case catalog::gaia_data_type_FLOAT32:
            return context->FloatTy;
        case catalog::gaia_data_type_FLOAT64:
            return context->DoubleTy;
        case catalog::gaia_data_type_STRING:
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

static unordered_map<string, unordered_map<string, QualType>> getTableData(Sema *s)
{
    unordered_map<string, unordered_map<string, QualType>> retVal;
    try 
    {
        DBMonitor monitor;

        for(catalog::Gaia_table table = catalog::Gaia_table::get_first();
            table; table = table.get_next())
        {
            unordered_map<string, QualType> fields;
            retVal[table.name()] = fields;
        }

        for(catalog::Gaia_field field = catalog::Gaia_field::get_first(); 
            field; field = field.get_next())
        {
            gaia_id_t tableId = field.table_id();
            catalog::Gaia_table tbl = catalog::Gaia_table::get(tableId);
            if (!tbl)
            {
                s->Diag(SourceLocation(), diag::err_invalid_table_field) << field.name();
                return unordered_map<string, unordered_map<string, QualType>>();
            }
            unordered_map<string, QualType> fields = retVal[tbl.name()];
            if (fields.find(field.name()) != fields.end())
            {
                s->Diag(SourceLocation(), diag::err_duplicate_field) << field.name();
                return unordered_map<string, unordered_map<string, QualType>>();
            }
            fields[field.name()] = mapFieldType(field.type(), &s->Context);

            retVal[tbl.name()] = fields;
        }
    }
    catch (exception e)
    {
        s->Diag(SourceLocation(), diag::err_catalog_exception) << e.what();
        return unordered_map<string, unordered_map<string, QualType>>();
    }
    return retVal;
}


void Sema::addField(IdentifierInfo *name, QualType type, RecordDecl *RD) const
{
    FieldDecl *Field = FieldDecl::Create(
        Context, RD, SourceLocation(), SourceLocation(),
        name, type, /*TInfo=*/nullptr,
        /*BitWidth=*/nullptr, /*Mutable=*/false, ICIS_NoInit);
    Field->setAccess(AS_public);
    Field->addAttr(GaiaFieldAttr::CreateImplicit(Context));
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
    unordered_map<string, unordered_map<string, QualType>> tableData = getTableData(this);
    auto tableDescription = tableData.find(tableName);
    if (tableDescription == tableData.end())
    {
        Diag(SourceLocation(),diag::err_invalid_table_name) << tableName;
        return Context.VoidTy;
    }
    
    RecordDecl *RD = Context.buildImplicitRecord(tableName + "__type");
    RD->setLexicalDeclContext(CurContext);
    RD->startDefinition();
    Scope S(CurScope,Scope::DeclScope|Scope::ClassScope, Diags);
    ActOnTagStartDefinition(&S,RD);
    ActOnStartCXXMemberDeclarations(getCurScope(), RD, SourceLocation(),
        false, SourceLocation());
    AttributeFactory attrFactory;
    ParsedAttributes attrs(attrFactory);   
    
    for (const auto f : tableDescription->second)
    {
        string fieldName = f.first;
        QualType fieldType = f.second;
        if (fieldType->isVoidType())
        {
            Diag(SourceLocation(),diag::err_invalid_field_type) << fieldName;
            return Context.VoidTy;
        }
        addField(&Context.Idents.get(fieldName), fieldType, RD);
    }

    //insert fields and methods that are not part of the schema
    addField(&Context.Idents.get("LastOperation"), Context.WIntTy.withConst(), RD);

    //addMethod(&Context.Idents.get("update"), DeclSpec::TST_void, nullptr, 0, attrFactory, attrs, &S, RD);
    
    ActOnFinishCXXMemberSpecification(getCurScope(), SourceLocation(), RD,
        SourceLocation(), SourceLocation(), attrs);
    ActOnTagFinishDefinition(getCurScope(), RD, SourceRange());
  
    return Context.getTagDeclType(RD);
}

QualType Sema::getFieldType (IdentifierInfo *id) 
{
    std::string fieldName = id->getName().str();

    if(fieldName == "UPDATE" ||
         fieldName == "DELETE" ||
         fieldName == "INSERT" ||
         fieldName == "NONE")
    {
        return Context.WIntTy.withConst();
    }

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
        Diag(SourceLocation(), diag::err_no_ruleset_for_rule);
        return Context.VoidTy;
    }
    vector<string> tables;
    RulesetDecl *rulesetDecl = dyn_cast<RulesetDecl>(c);
    RulesetTableAttr * attr = rulesetDecl->getAttr<RulesetTableAttr>();
    if (attr != nullptr)
    {
        for (const IdentifierInfo * id : attr->tables())
        {
            tables.push_back(id->getName().str());           
        }
    }

    unordered_map<string, unordered_map<string, QualType>> tableData = getTableData(this);
    if (tables.size() == 0)
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
            Diag(SourceLocation(),diag::err_invalid_table_name) << tableName;
            return Context.VoidTy;
        }

        auto fieldDescription = tableDescription->second.find(fieldName);
        if(fieldDescription != tableDescription->second.end())
        {
            if (fieldDescription->second->isVoidType())
            {
                Diag(SourceLocation(),diag::err_invalid_field_type) << fieldName;
                return Context.VoidTy;
            }
            if (retVal != Context.VoidTy)
            {
                Diag(SourceLocation(), diag::err_duplicate_field) << fieldName;
                return Context.VoidTy;
            }
                
            retVal = fieldDescription->second;
            fieldTableName = tableName;
        }      
    }

    if (retVal == Context.VoidTy)
    {
        Diag(SourceLocation(), diag::err_unknown_field) << fieldName;
    }

    return retVal;
}

NamedDecl *Sema::injectVariableDefinition(IdentifierInfo *II, bool isGaiaFieldTable)
{
    QualType qualType = isGaiaFieldTable ? getTableType(II) : getFieldType(II);
    if (qualType->isVoidType())
    {
        return nullptr;
    }

    std::string varName = II->getName().str();

    DeclContext *context  = getCurFunctionDecl();

    VarDecl *varDecl = VarDecl::Create(Context, context, SourceLocation(), SourceLocation(),
                            II, qualType, Context.getTrivialTypeSourceInfo(qualType, SourceLocation()), SC_None);
    varDecl->setLexicalDeclContext(context);
    varDecl->setImplicit();

    if (varName =="UPDATE")
    {
        varDecl->addAttr(GaiaLastOperationUPDATEAttr::CreateImplicit(Context));
        varDecl->setConstexpr(true);
        varDecl->setInit(ActOnIntegerConstant(SourceLocation(),0).get());
    }
    else if (varName == "INSERT")
    {
        varDecl->addAttr(GaiaLastOperationINSERTAttr::CreateImplicit(Context));
        varDecl->setConstexpr(true);
        varDecl->setInit(ActOnIntegerConstant(SourceLocation(),0).get());
    }
    else if (varName == "DELETE")
    {
        varDecl->addAttr(GaiaLastOperationDELETEAttr::CreateImplicit(Context));
        varDecl->setConstexpr(true);
        varDecl->setInit(ActOnIntegerConstant(SourceLocation(),0).get());
    }
    else if (varName == "NONE")
    {
        varDecl->addAttr(GaiaLastOperationNONEAttr::CreateImplicit(Context));
        varDecl->setConstexpr(true);
        varDecl->setInit(ActOnIntegerConstant(SourceLocation(),0).get());
    }
    else if (isGaiaFieldTable)
    {
        varDecl->addAttr(FieldTableAttr::CreateImplicit(Context,II));
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

