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

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "clang/AST/PrettyDeclStackTrace.h"
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/GaiaCatalogFacade.hpp"
#include "clang/Sema/Scope.h"
#include "clang/Sema/Sema.h"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"

using namespace gaia;
using namespace std;
using namespace clang;
using namespace clang::gaia_catalog;

static string fieldTableName;

// A little ugly to put this here.
static gaia_catalog::gaia_catalog_context_t get_gaia_context()
{
    static gaia_catalog::gaia_catalog_context_t gaia_context;
    return gaia_context;
}

static constexpr char ruleContextTypeName[] = "rule_context__type";

void printAmbiguousFieldReferenceDiagnostic(
    Sema& sema,
    SourceLocation loc,
    const std::string& fieldName,
    std::optional<table_facade_t> table,
    std::vector<field_facade_t> fields,
    std::vector<link_facade_t> links)
{
    sema.Diag(loc, diag::err_ambiguous_field_reference) << fieldName;

    if (table)
    {
        sema.Diag(loc, diag::note_table_reference) << table->table_name();
    }

    for (auto& field : fields)
    {
        sema.Diag(loc, diag::note_field_reference) << field.table_name() << field.field_name();
    }

    for (auto& link : links)
    {
        sema.Diag(loc, diag::note_link_reference) << link.from_table() << link.field_name();
    }
}

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

bool Sema::does_path_includes_tags(const std::vector<std::string>& path, SourceLocation loc)
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
    if (pathString.rfind("/@") == 0)
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
    if (path.size() > 1 || pathString.front() == '/' || !tagMap.empty() || does_path_includes_tags(path, loc))
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
        for (const catalog::gaia_field_t& field : catalog::gaia_field_t::list())
        {
            catalog::gaia_table_t tbl = field.table();
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
        for (const catalog::gaia_field_t& field : catalog::gaia_field_t::list())
        {
            catalog::gaia_table_t tbl = field.table();
            if (!tbl)
            {
                Diag(loc, diag::err_invalid_table_field) << field.name();
                return unordered_set<string>();
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
        for (const auto& relationship : catalog::gaia_relationship_t::list())
        {
            catalog::gaia_table_t child_table = relationship.child();
            if (!child_table)
            {
                Diag(loc, diag::err_invalid_child_table) << relationship.name();
                return unordered_multimap<string, Sema::TableLinkData_t>();
            }

            catalog::gaia_table_t parent_table = relationship.parent();
            if (!parent_table)
            {
                Diag(loc, diag::err_invalid_parent_table) << relationship.name();
                return unordered_multimap<string, Sema::TableLinkData_t>();
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

void Sema::addMethod(IdentifierInfo* name, DeclSpec::TST retValType, DeclaratorChunk::ParamInfo* Params, unsigned NumParams, AttributeFactory& attrFactory, ParsedAttributes& attrs, Scope* S, RecordDecl* RD, SourceLocation loc)
{
    DeclSpec DS(attrFactory);
    const char* dummy;
    unsigned diagId;

    Declarator D(DS, DeclaratorContext::MemberContext);
    D.setFunctionDefinitionKind(FDK_Declaration);
    D.getMutableDeclSpec().SetTypeSpecType(retValType, loc, dummy, diagId, getPrintingPolicy());
    ActOnAccessSpecifier(AS_public, loc, loc, attrs);

    D.SetIdentifier(name, loc);

    DS.Finish(*this, getPrintingPolicy());

    D.AddTypeInfo(DeclaratorChunk::getFunction(true, false, loc, Params, NumParams, loc, loc, true, loc,
                                               /*MutableLoc=*/loc, EST_None, SourceRange(), nullptr, nullptr, 0, nullptr, nullptr, None, loc, loc, D, TypeResult(), &DS),
                  std::move(attrs), loc);

    DeclarationNameInfo NameInfo = GetNameForDeclarator(D);

    TypeSourceInfo* tInfo = GetTypeForDeclarator(D, S);

    CXXMethodDecl* Ret = CXXMethodDecl::Create(
        Context, cast<CXXRecordDecl>(RD), SourceLocation(), NameInfo, tInfo->getType(),
        tInfo, SC_None, false, false, SourceLocation());
    Ret->setAccess(AS_public);
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

    //insert fields
    addField(&Context.Idents.get("ruleset_name"), Context.getPointerType((Context.CharTy.withConst()).withConst()), RD, loc);
    addField(&Context.Idents.get("rule_name"), Context.getPointerType((Context.CharTy.withConst()).withConst()), RD, loc);
    addField(&Context.Idents.get("event_type"), Context.UnsignedIntTy.withConst(), RD, loc);
    addField(&Context.Idents.get("gaia_type"), Context.UnsignedIntTy.withConst(), RD, loc);

    ActOnFinishCXXMemberSpecification(getCurScope(), loc, RD, loc, loc, attrs);
    ActOnTagFinishDefinition(getCurScope(), RD, SourceRange());

    return Context.getTagDeclType(RD);
}

QualType Sema::getLinkType(const std::string& linkName, const std::string& from_table, SourceLocation loc)
{
    // If you have (farmer)-[incubators]->(incubator), the type name is: farmer_incubators__type.
    std::string linkTypeName;
    linkTypeName
        .append(from_table)
        .append("_")
        .append(linkName)
        .append("__type");

    RecordDecl* RD = Context.buildImplicitRecord(linkTypeName);
    RD->setLexicalDeclContext(CurContext);
    RD->startDefinition();
    Scope S(CurScope, Scope::DeclScope | Scope::ClassScope, Diags);
    ActOnTagStartDefinition(&S, RD);
    ActOnStartCXXMemberDeclarations(getCurScope(), RD, loc, false, loc);
    AttributeFactory attrFactory;
    ParsedAttributes attrs(attrFactory);

    addMethod(&Context.Idents.get("Connect"), DeclSpec::TST_bool, nullptr, 0, attrFactory, attrs, &S, RD, loc);
    addMethod(&Context.Idents.get("Disconnect"), DeclSpec::TST_bool, nullptr, 0, attrFactory, attrs, &S, RD, loc);

    ActOnFinishCXXMemberSpecification(getCurScope(), loc, RD, loc, loc, attrs);
    ActOnTagFinishDefinition(getCurScope(), RD, SourceRange());

    return Context.getTagDeclType(RD);
}

QualType Sema::getTableType(const std::string& tableName, SourceLocation loc)
{
    DeclContext* context = getCurFunctionDecl();
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
    std::unordered_map<std::string, std::string> tagMapping = getTagMapping(getCurFunctionDecl(), loc);
    if (tagMapping.find(tableName) != tagMapping.end())
    {
        typeName = tagMapping[tableName];
    }
    fieldTableName = typeName;
    const Type* realType = nullptr;

    auto& types = Context.getTypes();
    for (unsigned typeIdx = 0; typeIdx != types.size(); ++typeIdx)
    {
        const auto* type = types[typeIdx];
        const RecordDecl* record = type->getAsRecordDecl();
        if (record != nullptr)
        {
            const auto* id = record->getIdentifier();
            if (id != nullptr)
            {
                // Check if EDC type is defined.
                if (id->getName().equals(typeName + "_t"))
                {
                    realType = type;
                    break;
                }
            }
        }
    }

    RulesetDecl* rulesetDecl = dyn_cast<RulesetDecl>(context);
    RulesetTableAttr* attr = rulesetDecl->getAttr<RulesetTableAttr>();

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

    auto tableData = get_gaia_context().find_table(typeName);

    if (!tableData)
    {
        Diag(loc, diag::err_invalid_table_name) << typeName;
        return Context.VoidTy;
    }

    RecordDecl* RD = Context.buildImplicitRecord(typeName + "__type");
    RD->setLexicalDeclContext(CurContext);
    RD->startDefinition();
    Scope S(CurScope, Scope::DeclScope | Scope::ClassScope, Diags);
    ActOnTagStartDefinition(&S, RD);
    ActOnStartCXXMemberDeclarations(getCurScope(), RD, loc, false, loc);
    AttributeFactory attrFactory;
    ParsedAttributes attrs(attrFactory);

    if (realType != nullptr)
    {
        QualType R = Context.getFunctionType(
            BuildReferenceType(QualType(realType, 0), true, loc, DeclarationName()),
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

    for (const auto& field : tableData->fields())
    {
        QualType fieldType = field.field_type(Context);

        if (fieldType->isVoidType())
        {
            Diag(loc, diag::err_invalid_field_type) << field.field_name();
            return Context.VoidTy;
        }

        addField(&Context.Idents.get(field.field_name()), fieldType, RD, loc);
    }

    for (const auto& link : tableData->outgoing_links())
    {
        QualType linkType = getLinkType(link.field_name(), link.from_table(), loc);
        addField(&Context.Idents.get(link.field_name()), linkType, RD, loc);
    }

    //insert fields and methods that are not part of the schema
    addMethod(&Context.Idents.get("delete_row"), DeclSpec::TST_void, nullptr, 0, attrFactory, attrs, &S, RD, loc);
    addMethod(&Context.Idents.get("gaia_id"), DeclSpec::TST_int, nullptr, 0, attrFactory, attrs, &S, RD, loc);
    // TODO this is weird, we have half API upper case and the other half lower case.
    //   IMHO we should stick to lower/snake case as we do for all the other APIs.
    //   The upper case is something David uses in his spec but, as himself said,
    //   it is something we are not forced to follow.
    addMethod(&Context.Idents.get("Connect"), DeclSpec::TST_bool, nullptr, 0, attrFactory, attrs, &S, RD, loc);
    addMethod(&Context.Idents.get("Disconnect"), DeclSpec::TST_bool, nullptr, 0, attrFactory, attrs, &S, RD, loc);

    ActOnFinishCXXMemberSpecification(getCurScope(), loc, RD, loc, loc, attrs);
    ActOnTagFinishDefinition(getCurScope(), RD, SourceRange());

    return Context.getTagDeclType(RD);
}

QualType Sema::getFieldType(std::string fieldName, SourceLocation loc)
{
    DeclContext* context = getCurFunctionDecl();

    std::unordered_map<std::string, std::string> tagMapping = getTagMapping(getCurFunctionDecl(), loc);
    auto pair = tagMapping.find(fieldName);

    // There is a tag, that hides the real field name.
    if (pair != tagMapping.end())
    {
        fieldName = pair->second;
    }

    if (!get_gaia_context().is_name_valid(fieldName))
    {
        Diag(loc, diag::err_unknown_field) << fieldName;
        return Context.VoidTy;
    }

    if (get_gaia_context().is_name_unique(fieldName))
    {
        std::optional<table_facade_t> tableData = get_gaia_context().find_table(fieldName);
        if (tableData)
        {
            return getTableType(fieldName, loc);
        }

        std::vector<field_facade_t> fields = get_gaia_context().find_fields(fieldName);
        if (!fields.empty())
        {
            fieldTableName = fields.front().table_name();
            return fields.front().field_type(Context);
        }

        std::vector<link_facade_t> links = get_gaia_context().find_links(fieldName);
        if (!links.empty())
        {
            auto link = links.front();
            fieldTableName = link.from_table();
            return getLinkType(link.field_name(), link.from_table(), loc);
        }

        assert(false && "Field name should appear at least in one place.");
    }

    // The name is not unique in the catalog. We look for a Table declaration
    // at the beginning of the ruleset to determine if we can disambiguate it.
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

    RulesetDecl* rulesetDecl = dyn_cast<RulesetDecl>(context);
    RulesetTableAttr* tableAttr = rulesetDecl->getAttr<RulesetTableAttr>();

    // There is no Table declaration, being the name non-unique it is an ambiguous reference.
    if (tableAttr == nullptr)
    {
        printAmbiguousFieldReferenceDiagnostic(
            *this,
            loc,
            fieldName,
            get_gaia_context().find_table(fieldName),
            get_gaia_context().find_fields(fieldName),
            get_gaia_context().find_links(fieldName));

        return Context.VoidTy;
    }

    vector<string> table_names;

    // Collect the tables defined in the Table attribute.
    for (const IdentifierInfo* id : tableAttr->tables())
    {
        std::optional<table_facade_t> table = get_gaia_context().find_table(id->getName().str());

        if (!table)
        {
            Diag(loc, diag::err_invalid_table_name) << id->getName().str();
            return Context.VoidTy;
        }

        table_names.push_back(table->table_name());
    }

    optional<table_facade_t> table = get_gaia_context().find_table(fieldName);
    std::vector<field_facade_t> fields = get_gaia_context().find_fields_in_tables(table_names, fieldName);
    std::vector<link_facade_t> links = get_gaia_context().find_links_in_tables(table_names, fieldName);

    // We collect inside this vector all the types deriving from the fields and relationships.
    // In order to be an unambiguous field reference this array must contain exactly one item.
    std::vector<QualType> types;

    if (std::find(table_names.begin(), table_names.end(), fieldName) != table_names.end())
    {
        fieldTableName = fieldName;
        types.push_back(getTableType(fieldName, loc));
    }

    for (auto field : fields)
    {
        fieldTableName = field.table_name();
        types.push_back(field.field_type(Context));
    }

    for (auto link : links)
    {
        fieldTableName = link.from_table();
        types.push_back(getLinkType(link.field_name(), link.from_table(), loc));
    }

    assert(!types.empty() && "It is expected to exists at least one field or relationship");

    if (types.size() > 1)
    {
        printAmbiguousFieldReferenceDiagnostic(
            *this, loc, fieldName,
            table, fields, links);

        return Context.VoidTy;
    }

    return types.front();
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
