//===--- SemaGaia.cpp - Gaia Extensions Parser -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Gaia Extensions semantic checks for Sema
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

#include "clang/AST/PrettyDeclStackTrace.h"
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/Sema.h"
#include <clang/Sema/Lookup.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"

using namespace gaia;
using namespace std;
using namespace clang;

static string fieldTableName;

static constexpr char ruleContextTypeName[] = "rule_context__type";

static string get_table_from_expression(const string& expression)
{
    size_t dot_position = expression.find('.');
    if (dot_position != string::npos)
    {
        return expression.substr(0, dot_position);
    }
    else
    {
        return expression;
    }
}

static QualType mapFieldType(catalog::data_type_t dbType, ASTContext* context)
{
    // Clang complains if we add a default clause to a switch that covers all values of an enum,
    // so this code is written to avoid that.
    QualType returnType = context->VoidTy;

    switch (dbType)
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

StringRef Sema::ConvertString(const string& str, SourceLocation loc)
{
    string literalString = string("\"") + str + string("\"");
    Token Toks[1];
    Toks[0].startToken();
    Toks[0].setKind(tok::string_literal);
    Toks[0].setLocation(loc);
    Toks[0].setLiteralData(literalString.data());
    Toks[0].setLength(literalString.size());

    StringLiteral* literal = cast<StringLiteral>(ActOnStringLiteral(Toks, nullptr).get());
    return literal->getString();
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

bool Sema::doesPathIncludesTags(const std::vector<std::string>& path, SourceLocation loc)
{
    std::unordered_map<std::string, std::string> tagMapping = getTagMapping(getCurFunctionDecl(), loc);
    if (tagMapping.empty())
    {
        return false;
    }
    for (const auto& path_component : path)
    {
        if (tagMapping.find(get_table_from_expression(path_component)) != tagMapping.end())
        {
            return true;
        }
    }
    return false;
}

std::string Sema::ParseExplicitPath(const std::string& pathString, SourceLocation loc)
{
    size_t searchStartPosition = 0;
    unordered_map<string, string> tagMap;
    vector<string> path;
    bool is_absolute = pathString.front() == '/';
    if (pathString.front() == '/' || pathString.front() == '@')
    {
        searchStartPosition = 1;
    }
    if (pathString.rfind("@/") == 0)
    {
        searchStartPosition = 2;
    }
    string tag;
    size_t tagPosition = 0, arrowPosition = 0;
    unordered_set<string> tableData = getCatalogTableList(loc);
    std::unordered_map<std::string, std::string> tagMapping = getTagMapping(getCurFunctionDecl(), loc);

    while (tagPosition != string::npos || arrowPosition != string::npos)
    {
        if (tagPosition < arrowPosition)
        {
            if (tagPosition == searchStartPosition)
            {
                Diag(loc, diag::err_invalid_explicit_path);
                return "";
            }
            tag = pathString.substr(searchStartPosition, tagPosition - searchStartPosition);
            searchStartPosition = tagPosition + 1;
            if (tableData.find(tag) != tableData.end())
            {
                Diag(loc, diag::err_ambiguous_tag_defined) << tag;
                return "";
            }
            if (tagMap.find(tag) != tagMap.end() || tagMapping.find(tag) != tagMapping.end())
            {
                Diag(loc, diag::err_tag_redefined) << tag;
                return "";
            }
        }
        else if (arrowPosition < tagPosition)
        {
            if (arrowPosition == searchStartPosition)
            {
                Diag(loc, diag::err_invalid_explicit_path);
                return "";
            }
            string table = pathString.substr(searchStartPosition, arrowPosition - searchStartPosition);

            if (!tag.empty())
            {
                tagMap[tag] = get_table_from_expression(table);
                tag.clear();
            }
            path.push_back(table);
            searchStartPosition = arrowPosition + 2;
        }

        tagPosition = pathString.find(':', searchStartPosition);
        arrowPosition = pathString.find("->", searchStartPosition);
    }
    string table = pathString.substr(searchStartPosition);
    if (table.empty())
    {
        Diag(loc, diag::err_invalid_explicit_path);
        return "";
    }
    if (!tag.empty())
    {
        tagMap[tag] = get_table_from_expression(table);
    }
    path.push_back(table);

    // If explicit path has one component only, this component will be checked at later stage
    // Therefore there is no need to perform more checks here.
    if (path.size() > 1 || pathString.front() == '/' || !tagMap.empty() || doesPathIncludesTags(path, loc))
    {
        unordered_multimap<string, TableLinkData_t> relationData = getCatalogTableRelations(loc);

        for (auto tagEntry : tagMap)
        {
            auto tableDescription = tableData.find(tagEntry.second);
            if (tableDescription == tableData.end())
            {
                Diag(loc, diag::err_invalid_table_name) << tagEntry.second;
                return "";
            }
        }

        unordered_set<string> duplicate_component_check_set;
        string previousTable, previousField;
        for (string pathComponent : path)
        {
            if (duplicate_component_check_set.find(pathComponent) != duplicate_component_check_set.end())
            {
                Diag(loc, diag::err_invalid_explicit_path);
                return "";
            }
            else
            {
                duplicate_component_check_set.insert(pathComponent);
            }
            string tableName, fieldName;
            size_t dotPosition = pathComponent.find('.');
            if (dotPosition != string::npos)
            {
                tableName = pathComponent.substr(0, dotPosition);
                fieldName = pathComponent.substr(dotPosition + 1);
            }
            else
            {
                tableName = pathComponent;
            }
            auto tagMappingIterator = tagMapping.find(tableName);
            auto tagMapIterator = tagMap.find(tableName);
            if (tagMappingIterator != tagMapping.end() || tagMapIterator != tagMap.end())
            {
                if (tagMappingIterator != tagMapping.end() && is_absolute)
                {
                    Diag(loc, diag::err_incorrect_tag_use_in_path) << tableName;
                    return "";
                }

                if (tagMappingIterator != tagMapping.end() && pathComponent != path.front() && pathComponent != path.back())
                {
                    Diag(loc, diag::err_incorrect_tag_use_in_path) << tableName;
                    return "";
                }

                if (pathComponent != path.front())
                {
                    Diag(loc, diag::err_incorrect_tag_use_in_path) << tableName;
                    return "";
                }
                if (tagMappingIterator != tagMapping.end())
                {
                    tableName = tagMappingIterator->second;
                }
                else
                {
                    tableName = tagMapIterator->second;
                }
            }
            auto tableDescription = tableData.find(tableName);
            if (tableDescription == tableData.end())
            {
                Diag(loc, diag::err_invalid_tag_defined) << tableName;
                return "";
            }

            if (!previousTable.empty())
            {
                auto relatedTablesIterator = relationData.equal_range(previousTable);

                if (relatedTablesIterator.first == relationData.end() && relatedTablesIterator.second == relationData.end())
                {
                    Diag(loc, diag::err_no_relations_table_in_path) << previousTable << pathComponent;
                    return "";
                }

                bool isMatchFound = false;
                for (auto tableIterator = relatedTablesIterator.first; tableIterator != relatedTablesIterator.second; ++tableIterator)
                {
                    if (tableIterator != relationData.end())
                    {
                        string table = tableIterator->second.table;
                        string field = tableIterator->second.field;
                        if (tableName == table)
                        {
                            if (!previousField.empty())
                            {
                                if (previousField == field)
                                {
                                    isMatchFound = true;
                                    break;
                                }
                            }
                            else
                            {
                                isMatchFound = true;
                                break;
                            }
                        }
                    }
                }
                if (!isMatchFound)
                {
                    Diag(loc, diag::err_no_relations_table_in_path) << previousTable << pathComponent;
                    return "";
                }
            }

            previousField = fieldName;
            previousTable = tableName;
        }
        explicitPathData[loc].path = path;
        explicitPathData[loc].tagMap = tagMap;
    }
    else
    {
        RemoveExplicitPathData(loc);
    }
    explicitPathTagMapping[loc] = tagMap;

    if (IsInExtendedExplicitPathScope())
    {
        extendedExplicitPathTagMapping[loc] = tagMap;
    }

    return path.back();
}

unordered_map<string, unordered_map<string, QualType>> Sema::getTableData(SourceLocation loc)
{
    unordered_map<string, unordered_map<string, QualType>> retVal;
    try
    {
        DBMonitor monitor;
        for (const catalog::gaia_field_t& field : catalog::gaia_field_t::list())
        {
            catalog::gaia_table_t tbl = field.table();
            if (!tbl)
            {
                Diag(loc, diag::err_invalid_table_field) << field.name();
                return unordered_map<string, unordered_map<string, QualType>>();
            }
            if (tbl.is_system())
            {
                continue;
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
    catch (const exception& e)
    {
        Diag(loc, diag::err_catalog_exception) << e.what();
        return unordered_map<string, unordered_map<string, QualType>>();
    }
    return retVal;
}

unordered_set<string> Sema::getCatalogTableList(SourceLocation loc)
{
    unordered_set<string> retVal;
    try
    {
        DBMonitor monitor;
        for (const catalog::gaia_field_t& field : catalog::gaia_field_t::list())
        {
            catalog::gaia_table_t tbl = field.table();
            if (!tbl)
            {
                Diag(loc, diag::err_invalid_table_field) << field.name();
                return unordered_set<string>();
            }
            if (tbl.is_system())
            {
                continue;
            }
            retVal.emplace(tbl.name());
        }
    }
    catch (const exception& e)
    {
        Diag(loc, diag::err_catalog_exception) << e.what();
        return unordered_set<string>();
    }

    return retVal;
}

unordered_multimap<string, Sema::TableLinkData_t> Sema::getCatalogTableRelations(SourceLocation loc)
{
    unordered_multimap<string, Sema::TableLinkData_t> retVal;
    try
    {
        DBMonitor monitor;
        for (const auto& relationship : catalog::gaia_relationship_t::list())
        {
            catalog::gaia_table_t child_table = relationship.child();
            if (!child_table)
            {
                Diag(loc, diag::err_invalid_child_table) << relationship.name();
                return unordered_multimap<string, Sema::TableLinkData_t>();
            }

            if (child_table.is_system())
            {
                continue;
            }

            catalog::gaia_table_t parent_table = relationship.parent();
            if (!parent_table)
            {
                Diag(loc, diag::err_invalid_parent_table) << relationship.name();
                return unordered_multimap<string, Sema::TableLinkData_t>();
            }

            if (parent_table.is_system())
            {
                continue;
            }

            TableLinkData_t link_data_1;
            link_data_1.table = parent_table.name();
            link_data_1.field = relationship.to_parent_link_name();
            TableLinkData_t link_data_n;
            link_data_n.table = child_table.name();
            link_data_n.field = relationship.to_child_link_name();

            retVal.emplace(child_table.name(), link_data_1);
            retVal.emplace(parent_table.name(), link_data_n);
        }
    }
    catch (const exception& e)
    {
        Diag(loc, diag::err_catalog_exception) << e.what();
        return unordered_multimap<string, Sema::TableLinkData_t>();
    }

    return retVal;
}

void Sema::addField(IdentifierInfo* name, QualType type, RecordDecl* RD, SourceLocation loc) const
{
    FieldDecl* Field = FieldDecl::Create(
        Context, RD, loc, loc,
        name, type, /*TInfo=*/nullptr,
        /*BitWidth=*/nullptr, /*Mutable=*/false, ICIS_NoInit);
    Field->setAccess(AS_public);
    Field->addAttr(GaiaFieldAttr::CreateImplicit(Context));

    RD->addDecl(Field);
}

void Sema::addMethod(IdentifierInfo* name, DeclSpec::TST retValType, SmallVector<QualType, 8> parameterTypes, AttributeFactory& attrFactory, ParsedAttributes& attrs, RecordDecl* RD, SourceLocation loc, bool isVariadic, ParsedType returnType)
{
    DeclContext* functionDecl = getCurFunctionDecl();
    DeclSpec DS(attrFactory);
    const char* dummy;
    unsigned diagId;

    Declarator D(DS, DeclaratorContext::MemberContext);
    D.setFunctionDefinitionKind(FDK_Declaration);
    if (!returnType)
    {
        D.getMutableDeclSpec().SetTypeSpecType(retValType, loc, dummy, diagId, getPrintingPolicy());
    }
    else
    {
        D.getMutableDeclSpec().SetTypeSpecType(retValType, loc, dummy, diagId, returnType, getPrintingPolicy());
    }
    ActOnAccessSpecifier(AS_public, loc, loc, attrs);

    D.SetIdentifier(name, loc);

    DS.Finish(*this, getPrintingPolicy());

    SmallVector<DeclaratorChunk::ParamInfo, 8> paramsInfo;
    SmallVector<ParmVarDecl*, 8> params;

    if (!parameterTypes.empty())
    {
        Declarator paramDeclarator(DS, DeclaratorContext::PrototypeContext);
        paramDeclarator.SetRangeBegin(loc);
        paramDeclarator.SetRangeEnd(loc);
        paramDeclarator.ExtendWithDeclSpec(DS);
        paramDeclarator.SetIdentifier(name, loc);
        int paramIndex = 1;

        for (QualType& type : parameterTypes)
        {
            // TODO we need a way to pass named params to have better diagnostics.
            string paramName = "param_" + to_string(paramIndex);
            ParmVarDecl* param = ParmVarDecl::Create(
                Context, Context.getTranslationUnitDecl(), loc, loc, &Context.Idents.get(paramName),
                Context.getAdjustedParameterType(type), nullptr, SC_None, nullptr);

            paramsInfo.push_back(
                DeclaratorChunk::ParamInfo(
                    paramDeclarator.getIdentifier(), paramDeclarator.getIdentifierLoc(), param));

            params.push_back(param);
            paramIndex++;
        }
    }

    D.AddTypeInfo(
        DeclaratorChunk::getFunction(
            true, false, loc, paramsInfo.data(),
            paramsInfo.size(), isVariadic ? loc : SourceLocation(), loc,
            true, loc,
            /*MutableLoc=*/loc,
            EST_None, SourceRange(), nullptr,
            nullptr, 0, nullptr,
            nullptr, None, loc,
            loc, D, TypeResult(), &DS),
        std::move(attrs), loc);

    DeclarationNameInfo NameInfo = GetNameForDeclarator(D);

    TypeSourceInfo* tInfo = GetTypeForDeclarator(D, getCurScope());

    CXXMethodDecl* Ret = CXXMethodDecl::Create(
        Context, cast<CXXRecordDecl>(RD), SourceLocation(), NameInfo, tInfo->getType(),
        tInfo, SC_None, false, false, SourceLocation());
    Ret->setAccess(AS_public);
    Ret->setParams(params);
    RD->addDecl(Ret);
}

QualType Sema::getRuleContextType(SourceLocation loc)
{
    RecordDecl* RD = Context.buildImplicitRecord(ruleContextTypeName);
    RD->setLexicalDeclContext(CurContext);
    RD->startDefinition();
    Scope S(CurScope, Scope::DeclScope | Scope::ClassScope, Diags);
    ActOnTagStartDefinition(&S, RD);
    ActOnStartCXXMemberDeclarations(getCurScope(), RD, loc, false, loc);
    AttributeFactory attrFactory;
    ParsedAttributes attrs(attrFactory);

    // Insert fields.
    addField(&Context.Idents.get("ruleset_name"), Context.getPointerType((Context.CharTy.withConst()).withConst()), RD, loc);
    addField(&Context.Idents.get("rule_name"), Context.getPointerType((Context.CharTy.withConst()).withConst()), RD, loc);
    addField(&Context.Idents.get("event_type"), Context.UnsignedIntTy.withConst(), RD, loc);
    addField(&Context.Idents.get("gaia_type"), Context.UnsignedIntTy.withConst(), RD, loc);

    ActOnFinishCXXMemberSpecification(getCurScope(), loc, RD, loc, loc, attrs);
    ActOnTagFinishDefinition(getCurScope(), RD, SourceRange());

    return Context.getTagDeclType(RD);
}

QualType Sema::getLinkType(const std::string& linkName, const std::string& from_table, const std::string& to_table, SourceLocation loc)
{
    // If you have (farmer)-[incubators]->(incubator), the type name is: farmer_incubators__type.
    // The table name is necessary because there could me multiple links in multiple tables
    // with the same name.
    std::string linkTypeName;
    linkTypeName
        .append(from_table)
        .append("_")
        .append(linkName)
        .append("__type");

    TagDecl* linkTypeDeclaration = lookupClass(linkTypeName, loc, getCurScope());

    if (linkTypeDeclaration)
    {
        return Context.getTagDeclType(linkTypeDeclaration);
    }

    RecordDecl* RD = Context.buildImplicitRecord(linkTypeName);
    RD->setLexicalDeclContext(CurContext);
    RD->startDefinition();
    PushOnScopeChains(RD, getCurScope());
    AttributeFactory attrFactory;
    ParsedAttributes attrs(attrFactory);

    addConnectDisconnect(RD, to_table, loc, attrFactory, attrs);

    RD->completeDefinition();

    return Context.getTagDeclType(RD);
}

TagDecl* Sema::lookupClass(std::string className, SourceLocation loc, Scope* scope)
{
    DeclarationName declName = &Context.Idents.get(className);

    LookupResult previousDeclLookup(*this, declName, loc, LookupTagName, forRedeclarationInCurContext());
    LookupName(previousDeclLookup, scope);

    TagDecl* classDecl = nullptr;

    if (!previousDeclLookup.empty())
    {
        for (auto iter = previousDeclLookup.begin();
             iter != previousDeclLookup.end();
             iter++)
        {
            NamedDecl* foundDecl = iter.getDecl();
            classDecl = cast<TagDecl>(previousDeclLookup.getFoundDecl());

            if (classDecl->isCompleteDefinition())
            {
                // This class has already been declared and defined in this context.
                return classDecl;
            }
        }
    }

    return classDecl;
}

TagDecl* Sema::lookupEDCClass(std::string className)
{
    // TODO do a search bound to a namespace:
    //   LookupResult gaiaNS(
    //       *this, &Context.Idents.get("gaia"), loc, LookupNamespaceName);
    //   LookupQualifiedName(gaiaNS, Context.getTranslationUnitDecl());
    //  ....

    for (Type* type : Context.getTypes())
    {
        RecordDecl* record = type->getAsRecordDecl();
        if (record != nullptr)
        {
            const IdentifierInfo* id = record->getIdentifier();
            if (id && id->getName().equals(className))
            {
                return llvm::cast_or_null<TagDecl>(record);
            }
        }
    }

    return nullptr;
}

void Sema::addConnectDisconnect(RecordDecl* sourceTableDecl, const string& targetTableName, SourceLocation loc, AttributeFactory& attrFactory, ParsedAttributes& attrs)
{
    SmallVector<TagDecl*, 2> targetTypes;

    // Look up the implicit class type (table__type).
    string implicitTableTypeName = targetTableName + "__type";
    TagDecl* implicitTargetTypeDecl = lookupClass(implicitTableTypeName, loc, getCurScope());

    if (!implicitTargetTypeDecl)
    {
        // The implicit type hasn't been declared yet. Creates a forward declaration.
        implicitTargetTypeDecl = CXXRecordDecl::Create(
            getASTContext(), RecordDecl::TagKind::TTK_Struct, Context.getTranslationUnitDecl(),
            SourceLocation(), SourceLocation(), &Context.Idents.get(implicitTableTypeName));

        implicitTargetTypeDecl->setLexicalDeclContext(getCurFunctionDecl());
        PushOnScopeChains(implicitTargetTypeDecl, getCurScope());
    }

    targetTypes.push_back(implicitTargetTypeDecl);

    // TODO [GAIAPLAT-1168] We should not statically build the EDC type, bust ask the Catalog for it.
    // Lookup the EDC class type (table_t)
    string edcTableTypeName = targetTableName + "_t";
    TagDecl* edcTargetTypeDecl = lookupEDCClass(edcTableTypeName);

    if (edcTargetTypeDecl)
    {
        targetTypes.push_back(edcTargetTypeDecl);
    }

    // Add connect/disconnect both for the implicit class and EDC class (if available).
    for (TagDecl* targetType : targetTypes)
    {
        // Creates a reference parameter (eg. incubator__type&).
        QualType connectDisconnectParam = Context.getTypeDeclType(targetType);
        QualType connectDisconnectParamRef = BuildReferenceType(QualType(targetType->getTypeForDecl(), 0), true, loc, DeclarationName()).withConst();

        SmallVector<QualType, 8> parameters;
        parameters.push_back(connectDisconnectParamRef);

        addMethod(&Context.Idents.get("connect"), DeclSpec::TST_bool, parameters, attrFactory, attrs, sourceTableDecl, SourceLocation(), false);
        addMethod(&Context.Idents.get("disconnect"), DeclSpec::TST_bool, parameters, attrFactory, attrs, sourceTableDecl, SourceLocation(), false);
    }
}

QualType Sema::getTableType(const std::string& tableName, SourceLocation loc)
{
    DeclContext* functionDecl = getCurFunctionDecl();
    DeclContext* rulesetContext = functionDecl;
    while (rulesetContext)
    {
        if (isa<RulesetDecl>(rulesetContext))
        {
            break;
        }
        rulesetContext = rulesetContext->getParent();
    }

    if (rulesetContext == nullptr || !isa<RulesetDecl>(rulesetContext))
    {
        Diag(loc, diag::err_no_ruleset_for_rule);
        return Context.VoidTy;
    }

    std::string typeName = tableName;
    std::unordered_map<std::string, std::string> tagMapping = getTagMapping(functionDecl, loc);
    if (tagMapping.find(tableName) != tagMapping.end())
    {
        typeName = tagMapping[tableName];
    }

    unordered_map<string, unordered_map<string, QualType>> tableData = getTableData(loc);
    if (tableData.empty())
    {
        return Context.VoidTy;
    }

    auto tableDescription = tableData.find(typeName);
    if (tableDescription == tableData.end())
    {
        Diag(loc, diag::err_invalid_table_name) << typeName;
        return Context.VoidTy;
    }

    // Look for a previous declaration of this table. There are 3 cases:
    //  1. It finds nothing: defines the type it from scratch.
    //  2. It finds the forward declaration: proceed with the type definition.
    //  3. It finds the definition and returns it. This happens if this method
    //     is called multiple times for the same table.

    string implicitClassName = typeName + "__type";

    TagDecl* previousDeclaration = lookupClass(implicitClassName, loc, getCurScope());
    fieldTableName = typeName;

    if (previousDeclaration && previousDeclaration->isCompleteDefinition())
    {
        return Context.getTagDeclType(previousDeclaration);
    }

    RulesetDecl* rulesetDecl = dyn_cast<RulesetDecl>(rulesetContext);
    RulesetTablesAttr* attr = rulesetDecl->getAttr<RulesetTablesAttr>();

    if (attr != nullptr)
    {
        bool table_found = false;
        for (const IdentifierInfo* id : attr->tables())
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

    RecordDecl* RD = CXXRecordDecl::Create(
        getASTContext(), RecordDecl::TagKind::TTK_Struct, Context.getTranslationUnitDecl(),
        SourceLocation(), SourceLocation(), &Context.Idents.get(implicitClassName),
        llvm::cast_or_null<CXXRecordDecl>(previousDeclaration));

    RD->setLexicalDeclContext(functionDecl);
    RD->startDefinition();
    PushOnScopeChains(RD, getCurScope());
    AttributeFactory attrFactory;
    ParsedAttributes attrs(attrFactory);

    // Adds a conversion function from the generated table type (table__type)
    // to the EDC type (table_t).
    string edcClassName = tableName + "_t";
    TagDecl* edcType = lookupEDCClass(edcClassName);
    if (edcType != nullptr)
    {
        QualType R = Context.getFunctionType(
            BuildReferenceType(Context.getTagDeclType(edcType), true, loc, DeclarationName()),
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

    // Add fields to the type.
    for (const auto& f : tableDescription->second)
    {
        string fieldName = f.first;
        QualType fieldType = f.second;
        if (fieldType->isVoidType())
        {
            Diag(loc, diag::err_invalid_field_type) << fieldName;
            return Context.VoidTy;
        }
        addField(&Context.Idents.get(fieldName), fieldType, RD, loc);
    }

    unordered_multimap<string, Sema::TableLinkData_t> relationships = getCatalogTableRelations(loc);
    auto links = relationships.equal_range(typeName);

    // For every relationship target table we count how many links
    // we have from tableName. This is needed to determine if we can
    // have a connect/disconnect method for a given target type.
    unordered_map<string, int> links_targets;

    for (auto link = links.first; link != links.second; ++link)
    {
        TableLinkData_t linkData = link->second;
        QualType type = getLinkType(linkData.field, tableName, linkData.table, loc);
        addField(&Context.Idents.get(linkData.field), type, RD, loc);
        links_targets[linkData.table]++;
    }

    // Insert fields and methods that are not part of the schema.  Note that we use the keyword 'remove' to
    // avoid conflicting with the C++ 'delete' keyword.
    addMethod(&Context.Idents.get("insert"), DeclSpec::TST_typename, {}, attrFactory, attrs, RD, loc, true, ParsedType::make(Context.getTagDeclType(RD)));
    addMethod(&Context.Idents.get("remove"), DeclSpec::TST_void, {}, attrFactory, attrs, RD, loc);
    addMethod(&Context.Idents.get("gaia_id"), DeclSpec::TST_int, {}, attrFactory, attrs, RD, loc);

    // connect and disconnect can be present only if the table has outgoing relationships.
    if (!links_targets.empty())
    {
        // For each outgoing relationship creates an overload connect/disconnect to the target types. eg:
        // incubator__type
        //   bool connect(actuator_type&);
        //   bool disconnect(actuator_type&);
        //   bool connect(farmer_type&);
        //   bool disconnect(farmer_type&);
        //   ....
        for (auto targetTablePair : links_targets)
        {
            // connect/disconnect are not appended to the table if there is more than one
            // link pointing to a target type.
            // The user will need to full qualify the relationship to use connect/disconnect:
            // incubator.actuators.connect(actuator);
            if (targetTablePair.second > 1)
            {
                continue;
            }

            addConnectDisconnect(RD, targetTablePair.first, loc, attrFactory, attrs);
        }
    }

    RD->completeDefinition();

    return Context.getTagDeclType(RD);
}

QualType Sema::getFieldType(const std::string& fieldOrTagName, SourceLocation loc)
{
    DeclContext* context = getCurFunctionDecl();
    unordered_map<string, unordered_map<string, QualType>> tableData = getTableData(loc);
    if (tableData.empty())
    {
        return Context.VoidTy;
    }

    std::unordered_map<std::string, std::string> tagMapping = getTagMapping(getCurFunctionDecl(), loc);

    // Check if the fieldOrTagName is a reference to a table or to a tag.
    if (tableData.find(fieldOrTagName) != tableData.end() || tagMapping.find(fieldOrTagName) != tagMapping.end())
    {
        for (auto iterator : tableData)
        {

            if (iterator.second.find(fieldOrTagName) != iterator.second.end())
            {
                // TODO add information about the source of ambiguity.
                Diag(loc, diag::err_ambiguous_field_reference) << fieldOrTagName;
                return Context.VoidTy;
            }
        }

        if (tableData.find(fieldOrTagName) != tableData.end())
        {
            return getTableType(fieldOrTagName, loc);
        }
        else
        {
            return getTableType(tagMapping[fieldOrTagName], loc);
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
    RulesetDecl* rulesetDecl = dyn_cast<RulesetDecl>(context);
    RulesetTablesAttr* attr = rulesetDecl->getAttr<RulesetTablesAttr>();

    if (attr != nullptr)
    {
        for (const IdentifierInfo* id : attr->tables())
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
        // Search if there is a match in the table fields.

        auto tableDescription = tableData.find(tableName);
        if (tableDescription == tableData.end())
        {
            Diag(loc, diag::err_invalid_table_name) << tableName;
            return Context.VoidTy;
        }

        auto fieldDescription = tableDescription->second.find(fieldOrTagName);

        if (fieldDescription != tableDescription->second.end())
        {
            if (fieldDescription->second->isVoidType())
            {
                Diag(loc, diag::err_invalid_field_type) << fieldOrTagName;
                return Context.VoidTy;
            }
            if (retVal != Context.VoidTy)
            {
                Diag(loc, diag::err_duplicate_field) << fieldOrTagName;
                return Context.VoidTy;
            }

            retVal = fieldDescription->second;
            fieldTableName = tableName;
        }
        else
        {
            // Search if there is a match in the table links.

            unordered_multimap<string, Sema::TableLinkData_t> relationships = getCatalogTableRelations(loc);
            auto links = relationships.equal_range(tableName);

            for (auto link = links.first; link != links.second; ++link)
            {
                TableLinkData_t linkData = link->second;

                if (linkData.field == fieldOrTagName)
                {
                    if (retVal != Context.VoidTy)
                    {
                        Diag(loc, diag::err_duplicate_link) << fieldOrTagName;
                        return Context.VoidTy;
                    }

                    retVal = getLinkType(linkData.field, tableName, linkData.table, loc);
                    fieldTableName = tableName;
                }
            }
        }
    }

    if (retVal == Context.VoidTy)
    {
        Diag(loc, diag::err_unknown_field) << fieldOrTagName;
    }

    return retVal;
}

static bool parse_tagged_attribute(const string& attribute, string& table, string& tag)
{
    size_t tag_position = attribute.find(':');
    if (tag_position != string::npos)
    {
        tag = attribute.substr(0, tag_position);
    }
    else
    {
        return false;
    }
    size_t dot_position = attribute.find('.', tag_position + 1);
    // Handle fully qualified reference.
    if (dot_position != string::npos)
    {
        table = attribute.substr(tag_position + 1, dot_position - tag_position - 1);
        return true;
    }

    table = attribute.substr(tag_position + 1);
    return true;
}

std::unordered_map<std::string, std::string> Sema::getTagMapping(const DeclContext* context, SourceLocation loc)
{
    std::unordered_map<std::string, std::string> retVal;
    const FunctionDecl* ruleContext = dyn_cast<FunctionDecl>(context);
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
            if (parse_tagged_attribute(table_iterator, table, tag))
            {
                if (retVal.find(tag) != retVal.end())
                {
                    Diag(loc, diag::err_tag_redefined) << tag;
                    return std::unordered_map<std::string, std::string>();
                }
                else
                {
                    retVal[tag] = table;
                }
            }
        }
    }

    if (insert_attribute != nullptr)
    {
        for (const auto& table_iterator : insert_attribute->tables())
        {
            string table, tag;
            if (parse_tagged_attribute(table_iterator, table, tag))
            {
                if (retVal.find(tag) != retVal.end())
                {
                    Diag(loc, diag::err_tag_redefined) << tag;
                    return std::unordered_map<std::string, std::string>();
                }
                else
                {
                    retVal[tag] = table;
                }
            }
        }
    }

    if (change_attribute != nullptr)
    {
        for (const auto& table_iterator : change_attribute->tables())
        {
            string table, tag;
            if (parse_tagged_attribute(table_iterator, table, tag))
            {
                if (retVal.find(tag) != retVal.end())
                {
                    Diag(loc, diag::err_tag_redefined) << tag;
                    return std::unordered_map<std::string, std::string>();
                }
                else
                {
                    retVal[tag] = table;
                }
            }
        }
    }

    for (const auto& explicitPathTagMapIterator : explicitPathTagMapping)
    {
        const auto& tagMap = explicitPathTagMapIterator.second;
        for (const auto& tagMapIterator : tagMap)
        {
            if (retVal.find(tagMapIterator.first) != retVal.end())
            {
                Diag(loc, diag::err_tag_redefined) << tagMapIterator.first;
                return std::unordered_map<std::string, std::string>();
            }
            else
            {
                retVal[tagMapIterator.first] = tagMapIterator.second;
            }
        }
    }

    for (const auto& explicitPathTagMapIterator : extendedExplicitPathTagMapping)
    {
        const auto& tagMap = explicitPathTagMapIterator.second;
        for (const auto& tagMapIterator : tagMap)
        {
            if (explicitPathTagMapping.find(explicitPathTagMapIterator.first) == explicitPathTagMapping.end())
            {
                if (retVal.find(tagMapIterator.first) != retVal.end())
                {
                    Diag(loc, diag::err_tag_redefined) << tagMapIterator.first;
                    return std::unordered_map<std::string, std::string>();
                }
                else
                {
                    retVal[tagMapIterator.first] = tagMapIterator.second;
                }
            }
        }
    }

    return retVal;
}

NamedDecl* Sema::injectVariableDefinition(IdentifierInfo* II, SourceLocation loc, const string& explicitPath)
{
    QualType qualType = Context.VoidTy;

    string table = ParseExplicitPath(explicitPath, loc);

    if (!table.empty())
    {
        size_t dot_position = table.find('.');

        if (dot_position != string::npos)
        {
            qualType = getTableType(table.substr(0, dot_position), loc);
        }
        else
        {
            qualType = getFieldType(table, loc);
        }
    }

    if (qualType->isVoidType())
    {
        return nullptr;
    }
    DeclContext* context = getCurFunctionDecl();
    VarDecl* varDecl = VarDecl::Create(Context, context, loc, loc, II, qualType, Context.getTrivialTypeSourceInfo(qualType, loc), SC_None);
    varDecl->setLexicalDeclContext(context);
    varDecl->setImplicit();

    varDecl->addAttr(GaiaFieldAttr::CreateImplicit(Context));
    varDecl->addAttr(FieldTableAttr::CreateImplicit(Context, &Context.Idents.get(fieldTableName)));

    SourceLocation startLocation, endLocation;
    std::string path;

    if (GetExplicitPathData(loc, startLocation, endLocation, path))
    {
        std::unordered_map<std::string, std::string> tagMapping = getTagMapping(getCurFunctionDecl(), loc);
        SmallVector<StringRef, 4> argPathComponents, argTagKeys, argTagTables,
            argDefinedTagKeys, argDefinedTagTables;
        for (auto pathComponentsIterator : explicitPathData[loc].path)
        {
            argPathComponents.push_back(ConvertString(pathComponentsIterator, loc));
        }

        for (auto tagsIterator : explicitPathData[loc].tagMap)
        {
            argTagKeys.push_back(ConvertString(tagsIterator.first, loc));
            argTagTables.push_back(ConvertString(tagsIterator.second, loc));
            argDefinedTagKeys.push_back(ConvertString(tagsIterator.second, loc));
            argDefinedTagTables.push_back(ConvertString(tagsIterator.first, loc));
        }

        for (auto tagsIterator : explicitPathTagMapping[loc])
        {
            argTagKeys.push_back(ConvertString(tagsIterator.first, loc));
            argTagTables.push_back(ConvertString(tagsIterator.second, loc));
        }
        for (auto tagsIterator : extendedExplicitPathTagMapping[loc])
        {
            argTagKeys.push_back(ConvertString(tagsIterator.first, loc));
            argTagTables.push_back(ConvertString(tagsIterator.second, loc));
        }
        for (auto tagsIterator : tagMapping)
        {
            argTagKeys.push_back(ConvertString(tagsIterator.first, loc));
            argTagTables.push_back(ConvertString(tagsIterator.second, loc));
        }

        varDecl->addAttr(GaiaExplicitPathAttr::CreateImplicit(Context, path, startLocation.getRawEncoding(), endLocation.getRawEncoding(), argPathComponents.data(), argPathComponents.size()));
        varDecl->addAttr(GaiaExplicitPathTagKeysAttr::CreateImplicit(Context, argTagKeys.data(), argTagKeys.size()));
        varDecl->addAttr(GaiaExplicitPathTagValuesAttr::CreateImplicit(Context, argTagTables.data(), argTagTables.size()));
        varDecl->addAttr(GaiaExplicitPathDefinedTagKeysAttr::CreateImplicit(Context, argDefinedTagKeys.data(), argDefinedTagKeys.size()));
        varDecl->addAttr(GaiaExplicitPathDefinedTagValuesAttr::CreateImplicit(Context, argDefinedTagTables.data(), argDefinedTagTables.size()));
    }
    context->addDecl(varDecl);

    return varDecl;
}

Decl* Sema::ActOnRulesetDefStart(Scope* S, SourceLocation RulesetLoc, SourceLocation IdentLoc, IdentifierInfo* Ident, const ParsedAttributesView& AttrList)
{
    Scope* declRegionScope = S->getParent();

    RulesetDecl* ruleset = RulesetDecl::Create(Context, CurContext, RulesetLoc, IdentLoc, Ident);

    ProcessDeclAttributeList(declRegionScope, ruleset, AttrList);

    PushOnScopeChains(ruleset, declRegionScope);
    ActOnDocumentableDecl(ruleset);
    PushDeclContext(S, ruleset);

    return ruleset;
}

void Sema::ActOnRulesetDefFinish(Decl* Dcl, SourceLocation RBrace)
{
    RulesetDecl* ruleset = dyn_cast_or_null<RulesetDecl>(Dcl);
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

void Sema::AddExplicitPathData(SourceLocation location, SourceLocation startLocation, SourceLocation endLocation, const std::string& explicitPath)
{
    explicitPathData[location] = {startLocation, endLocation, explicitPath, std::vector<std::string>(), std::unordered_map<std::string, std::string>()};
}

void Sema::RemoveExplicitPathData(SourceLocation location)
{
    explicitPathData.erase(location);
}

bool Sema::IsExpressionInjected(const Expr* expression) const
{
    if (expression == nullptr)
    {
        return false;
    }
    return injectedVariablesLocation.find(expression->getExprLoc()) != injectedVariablesLocation.end();
}

bool Sema::GetExplicitPathData(SourceLocation location, SourceLocation& startLocation, SourceLocation& endLocation, std::string& explicitPath)
{
    auto data = explicitPathData.find(location);
    if (data == explicitPathData.end())
    {
        return false;
    }
    startLocation = data->second.startLocation;
    endLocation = data->second.endLocation;
    explicitPath = data->second.explicitPath;
    return true;
}

bool Sema::RemoveTagData(SourceRange range)
{
    if (range.isValid())
    {
        auto startLocationIterator = extendedExplicitPathTagMapping.lower_bound(range.getBegin());
        auto endLocationIterator = extendedExplicitPathTagMapping.upper_bound(range.getEnd());
        if (startLocationIterator == extendedExplicitPathTagMapping.end() && endLocationIterator == extendedExplicitPathTagMapping.end())
        {
            return false;
        }
        extendedExplicitPathTagMapping.erase(startLocationIterator, endLocationIterator);
    }
    return true;
}

bool Sema::IsExplicitPathInRange(SourceRange range) const
{
    if (range.isValid())
    {
        return extendedExplicitPathTagMapping.lower_bound(range.getBegin()) != extendedExplicitPathTagMapping.end() || extendedExplicitPathTagMapping.upper_bound(range.getEnd()) != extendedExplicitPathTagMapping.end();
    }
    return false;
}

void Sema::ActOnStartDeclarativeLabel(const string& label)
{
    declarativeLabelsInProcess.emplace(label);
}

bool Sema::ActOnStartLabel(const string& label)
{
    if (declarativeLabelsInProcess.find(label) != declarativeLabelsInProcess.end())
    {
        return false;
    }
    labelsInProcess.emplace(label);
    return true;
}

bool Sema::ValidateLabel(const LabelDecl* label)
{
    string labelName = label->getName().str();

    // Check if there is a declarative label which is not currently processed as a regular label.
    for (auto declarativeLabel : declarativeLabelsInProcess)
    {
        if (labelsInProcess.find(declarativeLabel) == labelsInProcess.end())
        {
            Diag(label->getLocation(), diag::err_declarative_label_does_not_exist);
            return false;
        }
    }
    auto labelIterator = labelsInProcess.find(labelName);
    auto declarativeLabelIterator = declarativeLabelsInProcess.find(labelName);
    if (declarativeLabelIterator == declarativeLabelsInProcess.end())
    {
        labelsInProcess.erase(labelName);
        return true;
    }
    const LabelStmt* LabelStatement = label->getStmt();
    if (LabelStatement == nullptr)
    {
        Diag(label->getLocation(), diag::err_declarative_label_statement_is_invalid);
        return false;
    }
    const Stmt* statement = LabelStatement->getSubStmt();
    if (statement == nullptr)
    {
        Diag(label->getLocation(), diag::err_declarative_label_statement_is_invalid);
        return false;
    }
    if (!GaiaForStmt::classof(statement) && !IfStmt::classof(statement))
    {
        Diag(label->getLocation(), diag::err_declarative_label_wrong_statement);
        return false;
    }
    labelsInProcess.erase(labelName);
    declarativeLabelsInProcess.erase(labelName);
    return true;
}
