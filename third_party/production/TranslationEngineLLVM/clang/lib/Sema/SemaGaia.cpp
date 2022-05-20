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

#include "clang/AST/PrettyDeclStackTrace.h"
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/Lookup.h"
#include "clang/Catalog/GaiaCatalog.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/Twine.h"
using namespace gaia;
using namespace gaia::catalog::gaiat;
using namespace std;
using namespace clang;

static SmallString<20> fieldTableName;

static constexpr char ruleContextTypeNameBase[] = "rule_context";
static constexpr char connectFunctionName[] = "connect";
static constexpr char disconnectFunctionName[] = "disconnect";
static constexpr char clearFunctionName[] = "clear";

static SmallString<32> calculateNameHash(StringRef name)
{
    llvm::MD5 Hasher;
    llvm::MD5::MD5Result Hash;
    Hasher.update(name);
    Hasher.final(Hash);
    return Hash.digest();
}

static StringRef getTableFromExpression(StringRef expression)
{
    size_t dot_position = expression.find('.');
    if (dot_position != StringRef::npos)
    {
        return expression.substr(0, dot_position);
    }
    else
    {
        return expression;
    }
}

static QualType mapFieldType(catalog::data_type_t dbType, bool isArray, ASTContext* context)
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

    if (isArray)
    {
        return context->getIncompleteArrayType(returnType, ArrayType::Normal, 0);
    }

    return returnType;
}

StringRef Sema::ConvertString(StringRef str, SourceLocation loc)
{
    llvm::SmallString<20> literalString;
    literalString += '"';
    literalString += str;
    literalString += '"';
    Token Toks[1];
    Toks[0].startToken();
    Toks[0].setKind(tok::string_literal);
    Toks[0].setLocation(loc);
    Toks[0].setLiteralData(literalString.data());
    Toks[0].setLength(literalString.size());

    StringLiteral* literal = cast<StringLiteral>(ActOnStringLiteral(Toks, nullptr).get());
    return literal->getString();
}

bool Sema::doesPathIncludesTags(const SmallVector<std::string, 8>& path, SourceLocation loc)
{
    const llvm::StringMap<std::string>& tagMapping = getTagMapping(getCurFunctionDecl(), loc);
    if (tagMapping.empty())
    {
        return false;
    }
    for (const auto& path_component : path)
    {
        if (tagMapping.find(getTableFromExpression(path_component)) != tagMapping.end())
        {
            return true;
        }
    }
    return false;
}

std::string Sema::ParseExplicitPath(StringRef pathString, SourceLocation loc, StringRef& firstComponent)
{
    bool isFirstComponentProcessed = false;
    size_t searchStartPosition = 0;
    llvm::StringMap<string> tagMap;
    SmallVector<string, 8> path;
    bool is_absolute = pathString.front() == '/';
    if (is_absolute || pathString.front() == '@')
    {
        searchStartPosition = 1;
    }
    if (pathString.rfind("@/") == 0)
    {
        searchStartPosition = 2;
    }
    StringRef tag;
    size_t tagPosition = 0, arrowPosition = 0;
    const llvm::StringSet<>& tableData = getCatalogTableList();
    const llvm::StringMap<std::string>& tagMapping = getTagMapping(getCurFunctionDecl(), loc);

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
            StringRef table = pathString.substr(searchStartPosition, arrowPosition - searchStartPosition);

            if (!tag.empty())
            {
                tagMap[tag] = getTableFromExpression(table);
                tag = StringRef();
            }
            path.push_back(table);
            searchStartPosition = arrowPosition + 2;
            if (!isFirstComponentProcessed)
            {
                isFirstComponentProcessed = true;
                firstComponent = getTableFromExpression(table);
            }
        }

        tagPosition = pathString.find(':', searchStartPosition);
        arrowPosition = pathString.find("->", searchStartPosition);
    }
    StringRef table = pathString.substr(searchStartPosition);
    if (table.empty())
    {
        Diag(loc, diag::err_invalid_explicit_path);
        return "";
    }
    if (!tag.empty())
    {
        tagMap[tag] = getTableFromExpression(table);
    }
    path.push_back(table);

    if (firstComponent.empty())
    {
        firstComponent = getTableFromExpression(table);
    }

    // If explicit path has one component only, this component will be checked at later stage
    // Therefore there is no need to perform more checks here.
    if (path.size() > 1 || pathString.front() == '/' || !tagMap.empty() || doesPathIncludesTags(path, loc))
    {
        for (const auto& tagEntry : tagMap)
        {
            const auto& tableDescription = tableData.find(tagEntry.second);
            if (tableDescription == tableData.end())
            {
                Diag(loc, diag::err_table_not_found) << tagEntry.second;
                return "";
            }
        }

        llvm::StringSet<> duplicate_component_check_set;
        StringRef previousTable, previousField;
        for (StringRef pathComponent : path)
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
            StringRef tableName, fieldName;
            size_t dotPosition = pathComponent.find('.');
            if (dotPosition != string::npos)
            {
                tableName = pathComponent.substr(0, dotPosition);
                fieldName = pathComponent.substr(dotPosition + 1);
                dotPosition = fieldName.find('.');
                // If there is a dot in a field it is actually a link.
                // The links are allowed to reference connect/disconnect functions.
                if (dotPosition != string::npos)
                {
                    string postLink = fieldName.substr(dotPosition + 1);
                    if (postLink != connectFunctionName && postLink != disconnectFunctionName && postLink != clearFunctionName)
                    {
                        Diag(loc, diag::err_invalid_explicit_path);
                        return "";
                    }
                }
            }
            else
            {
                tableName = pathComponent;
            }
            const auto& tagMappingIterator = tagMapping.find(tableName);
            const auto& tagMapIterator = tagMap.find(tableName);
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
            const auto& tableDescription = tableData.find(tableName);
            if (tableDescription == tableData.end())
            {
                // Test to see if this is a field name
                if (findFieldType(tableName, loc))
                {
                    Diag(loc, diag::err_invalid_field_usage) << tableName;
                }
                else
                {
                    Diag(loc, diag::err_invalid_tag_defined) << tableName;
                }
                return "";
            }

            if (!previousTable.empty())
            {
                const llvm::StringMap<gaia::catalog::CatalogTableData>& catalogData = gaia::catalog::getCatalogTableData();
                const auto& relatedTablesIterator = catalogData.find(previousTable);

                if (relatedTablesIterator == catalogData.end() || relatedTablesIterator->second.linkData.empty())
                {
                    Diag(loc, diag::err_no_relations_table_in_path) << previousTable << pathComponent;
                    return "";
                }

                bool isMatchFound = false;
                for (const auto& tableIterator : relatedTablesIterator->second.linkData)
                {
                    StringRef table = tableIterator.second.targetTable;
                    StringRef field = tableIterator.first();
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

llvm::StringMap<llvm::StringMap<QualType>> Sema::getTableData()
{
    llvm::StringMap<llvm::StringMap<QualType>> result;
    const llvm::StringMap<gaia::catalog::CatalogTableData>& catalogData = gaia::catalog::getCatalogTableData();

    for (const auto& catalogDataItem : catalogData)
    {
        llvm::StringMap<QualType> fields;
        for (const auto& fieldData : catalogDataItem.second.fieldData)
        {
            fields[fieldData.first()] =  mapFieldType(fieldData.second.fieldType, fieldData.second.isArray, &Context);
        }
        result[catalogDataItem.first()] = fields;
    }

    return result;
}

llvm::StringSet<> Sema::getCatalogTableList()
{
    llvm::StringSet<> result;
    const llvm::StringMap<gaia::catalog::CatalogTableData>& catalogData = gaia::catalog::getCatalogTableData();

    for (const auto& catalogDataItem : catalogData)
    {
        result.insert(catalogDataItem.first());
    }

    return result;
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

void Sema::addMethod(IdentifierInfo* name, DeclSpec::TST retValType, const SmallVector<QualType, 8>& parameterTypes, AttributeFactory& attrFactory, ParsedAttributes& attrs, RecordDecl* RD, SourceLocation loc, bool isVariadic, ParsedType returnType)
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

        for (const QualType& type : parameterTypes)
        {
            // TODO we need a way to pass named params to have better diagnostics.
            string paramName = (Twine("param_") + Twine(paramIndex)).str();
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
    llvm::SmallString<64> ruleContextTypeName;
    ruleContextTypeName += ruleContextTypeNameBase;
    ruleContextTypeName += '_';
    ruleContextTypeName += calculateNameHash(ruleContextTypeNameBase);
    ruleContextTypeName += "__type";

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

    auto typeIterator = Context.getEDCTypes().find("event_type_t");
    if (typeIterator != Context.getEDCTypes().end())
    {
        addField(&Context.Idents.get("event_type"), typeIterator->second->getLocallyUnqualifiedSingleStepDesugaredType().withConst(), RD, loc);
    }
    else
    {
        addField(&Context.Idents.get("event_type"), Context.UnsignedIntTy.withConst(), RD, loc);
    }
    addField(&Context.Idents.get("gaia_type"), Context.UnsignedIntTy.withConst(), RD, loc);

    ActOnFinishCXXMemberSpecification(getCurScope(), loc, RD, loc, loc, attrs);
    ActOnTagFinishDefinition(getCurScope(), RD, SourceRange());

    return Context.getTagDeclType(RD);
}

QualType Sema::getLinkType(StringRef linkName, StringRef from_table, StringRef to_table, bool is_one_to_many, bool is_from_parent, bool is_value_linked, SourceLocation loc)
{
    // If you have (farmer)-[incubators]->(incubator), the type name is: farmer_incubators__type.
    // The table name is necessary because there could me multiple links in multiple tables
    // with the same name.
    llvm::SmallString<64> linkTypeName = from_table;
    linkTypeName += '_';
    linkTypeName += linkName;
    linkTypeName += '_';
    linkTypeName += to_table;
    linkTypeName += '_';
    linkTypeName += calculateNameHash(linkTypeName);
    linkTypeName += "__type";

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

    if (!is_value_linked)
    {
        addConnectDisconnect(RD, to_table, is_one_to_many, is_from_parent, loc, attrFactory, attrs);
    }

    RD->completeDefinition();

    return Context.getTagDeclType(RD);
}

TagDecl* Sema::lookupClass(StringRef className, SourceLocation loc, Scope* scope)
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

TagDecl* Sema::lookupEDCClass(StringRef className)
{
    // TODO do a search bound to a namespace:
    //   LookupResult gaiaNS(
    //       *this, &Context.Idents.get("gaia"), loc, LookupNamespaceName);
    //   LookupQualifiedName(gaiaNS, Context.getTranslationUnitDecl());
    //  ....

    auto typeIterator = Context.getEDCTypes().find(className);
    if (typeIterator != Context.getEDCTypes().end())
    {
        return llvm::cast_or_null<TagDecl>(typeIterator->second->getAsRecordDecl());
    }
    return nullptr;
}

void Sema::addConnectDisconnect(RecordDecl* sourceTableDecl, StringRef targetTableName, bool is_one_to_many, bool is_from_parent, SourceLocation loc, AttributeFactory& attrFactory, ParsedAttributes& attrs)
{
    SmallVector<TagDecl*, 2> targetTypes;

    // Look up the implicit class type (table__type).
    llvm::SmallString<64> implicitTableTypeName = targetTableName;
    implicitTableTypeName += '_';
    implicitTableTypeName += calculateNameHash(targetTableName);
    implicitTableTypeName += "__type";
    TagDecl* implicitTargetTypeDecl = lookupClass(implicitTableTypeName, loc, getCurScope());

    if (!implicitTargetTypeDecl)
    {
        // The implicit type hasn't been declared yet. Creates a forward declaration.
        implicitTargetTypeDecl = CXXRecordDecl::Create(
            getASTContext(), RecordDecl::TagKind::TTK_Struct, Context.getTranslationUnitDecl(),
            SourceLocation(), SourceLocation(), &Context.Idents.get(implicitTableTypeName));

        implicitTargetTypeDecl->addAttr(GaiaTableAttr::CreateImplicit(Context, &Context.Idents.get(targetTableName)));
        implicitTargetTypeDecl->setLexicalDeclContext(getCurFunctionDecl());
        PushOnScopeChains(implicitTargetTypeDecl, getCurScope());
    }

    targetTypes.push_back(implicitTargetTypeDecl);

    // Lookup the EDC class type.
    auto edcClassName = table_facade_t::class_name(targetTableName);
    // llvm::SmallString<20> edcTableTypeName = edcClassName;
    TagDecl* edcTargetTypeDecl = lookupEDCClass(edcClassName);
    if (edcTargetTypeDecl)
    {
        targetTypes.push_back(edcTargetTypeDecl);
        // TODO ideally we should apply this to all EDC classes...
        edcTargetTypeDecl->addAttr(GaiaTableAttr::CreateImplicit(Context, &Context.Idents.get(targetTableName)));
    }

    // Add connect/disconnect both for the implicit class and EDC class (if available).
    for (TagDecl* targetType : targetTypes)
    {
        // Creates a reference parameter (eg. incubator__type&).
        QualType connectDisconnectParamRef = BuildReferenceType(QualType(targetType->getTypeForDecl(), 0).withConst(), true, loc, DeclarationName());

        SmallVector<QualType, 8> parameters;
        parameters.push_back(connectDisconnectParamRef);

        addMethod(&Context.Idents.get(connectFunctionName), DeclSpec::TST_void, parameters, attrFactory, attrs, sourceTableDecl, SourceLocation(), false);

        // The disconnect with argument is available only 1:n relationships.
        if (!(is_from_parent ^ is_one_to_many))
        {
            addMethod(&Context.Idents.get(disconnectFunctionName), DeclSpec::TST_void, parameters, attrFactory, attrs, sourceTableDecl, SourceLocation(), false);
        }
    }

    if (!(is_from_parent ^ is_one_to_many) && !sourceTableDecl->hasAttr<GaiaTableAttr>())
    {
        addMethod(&Context.Idents.get("clear"), DeclSpec::TST_void, {}, attrFactory, attrs, sourceTableDecl, SourceLocation(), false);
    }

    // The disconnect without arguments is available only for 1:1 relationships that have explicit link:
    //  person.mother.disconnect();
    if ((is_from_parent ^ is_one_to_many) && !sourceTableDecl->hasAttr<GaiaTableAttr>())
    {
        addMethod(&Context.Idents.get(disconnectFunctionName), DeclSpec::TST_void, {}, attrFactory, attrs, sourceTableDecl, SourceLocation(), false);
    }
}

QualType Sema::getTableType(StringRef tableName, SourceLocation loc)
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

    llvm::SmallString<20> typeName = tableName;
    const llvm::StringMap<std::string>& tagMapping = getTagMapping(functionDecl, loc);
    const auto& tagMappingIterator = tagMapping.find(tableName);
    if (tagMappingIterator != tagMapping.end())
    {
        typeName = tagMappingIterator->second;
    }

    const llvm::StringMap<llvm::StringMap<QualType>>& tableData = getTableData();
    if (tableData.empty())
    {
        return Context.VoidTy;
    }

    const auto& tableDescription = tableData.find(typeName);
    if (tableDescription == tableData.end())
    {
        Diag(loc, diag::err_table_not_found) << typeName;
        return Context.VoidTy;
    }
    if (IsInExtendedExplicitPathScope())
    {
        StringRef anchorVariable = typeName;

        const auto& definedTagIterator = extendedExplicitPathTagMapping.find(loc);
        if (definedTagIterator != extendedExplicitPathTagMapping.end())
        {
            for (const auto& tagIterator : definedTagIterator->second)
            {
                if (tagIterator.second == typeName)
                {
                    anchorVariable = tagIterator.first();
                    break;
                }
            }
        }
        AddTableSearchAnchor(typeName, anchorVariable);
    }

    // Look for a previous declaration of this table. There are 3 cases:
    //  1. It finds nothing: defines the type it from scratch.
    //  2. It finds the forward declaration: proceed with the type definition.
    //  3. It finds the definition and returns it. This happens if this method
    //     is called multiple times for the same table.

    llvm::SmallString<64> implicitClassName;
    implicitClassName += typeName;
    implicitClassName += "_";
    implicitClassName += calculateNameHash(typeName);
    implicitClassName += "__type";

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
            if (id->getName() == typeName)
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

    // The attribute may have already been set in the forward declaration.
    if (!RD->hasAttr<GaiaTableAttr>())
    {
        RD->addAttr(GaiaTableAttr::CreateImplicit(Context, &Context.Idents.get(typeName)));
    }
    PushOnScopeChains(RD, getCurScope());
    AttributeFactory attrFactory;
    ParsedAttributes attrs(attrFactory);

    // Adds a conversion function from the generated table type (table__type)
    // to the EDC type (table_t).
    auto edcClassName = table_facade_t::class_name(typeName.c_str());
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

    // Adds a conversion function from the generated table type to a boolean
    QualType BooleanConversionType = Context.getFunctionType(Context.BoolTy, None, FunctionProtoType::ExtProtoInfo());
    CanQualType ClassBooleanConversionType = Context.getCanonicalType(BooleanConversionType);
    DeclarationName BooleanConversionName = Context.DeclarationNames.getCXXConversionFunctionName(ClassBooleanConversionType);
    DeclarationNameInfo BooleanConversionNameInfo(BooleanConversionName, loc);

    auto booleanConversionFunctionDeclaration = CXXConversionDecl::Create(
        Context, cast<CXXRecordDecl>(RD), loc, BooleanConversionNameInfo, BooleanConversionType,
        nullptr, false, true, false, SourceLocation());

    booleanConversionFunctionDeclaration->setAccess(AS_public);
    RD->addDecl(booleanConversionFunctionDeclaration);

    // Add fields to the type.
    for (const auto& f : tableDescription->second)
    {
        string fieldName = f.first();
        QualType fieldType = f.second;
        if (fieldType->isVoidType())
        {
            Diag(loc, diag::err_invalid_field_type) << fieldName;
            return Context.VoidTy;
        }
        addField(&Context.Idents.get(fieldName), fieldType, RD, loc);
    }

    const llvm::StringMap<gaia::catalog::CatalogTableData>& catalogData = gaia::catalog::getCatalogTableData();
    const auto& links = catalogData.find(typeName)->second.linkData;

    // For every relationship target table we count how many links
    // we have from tableName. This is needed to determine if we can
    // have a connect/disconnect method for a given target type.
    llvm::StringMap<int> links_target_tables;

    // Stores the cardinality for a given target table.
    // true -> one-to-many, false otherwise.
    // Note that if a table has multiple links it does not matter
    // because the connect/disconnect methods are not generated.
    llvm::StringMap<bool> links_cardinality;

    // Stores if a link is from parent to children.
    llvm::StringMap<bool> links_from_parent;

    // Stores if a link is value linked.
    llvm::StringMap<bool> value_linked_link;

    for (const auto& link : links)
    {
        const auto& linkData = link.second;

        // For now, connect/disconnect is supported only from the parent side
        if (linkData.isFromParent)
        {
            QualType type = getLinkType(link.first(), tableName, linkData.targetTable
                , linkData.cardinality == catalog::relationship_cardinality_t::many, true, linkData.isValueLinked, loc);
            addField(&Context.Idents.get(link.first()), type, RD, loc);
            links_target_tables[linkData.targetTable]++;
            links_cardinality[linkData.targetTable] = linkData.cardinality == catalog::relationship_cardinality_t::many;
            links_from_parent[linkData.targetTable] = true;
            value_linked_link[linkData.targetTable] = linkData.isValueLinked;
        }
        else
        {
            int linkCount = 0;
            const auto& targetLinks = catalogData.find(linkData.targetTable)->second.linkData;
            for (const auto& targetLink : targetLinks)
            {
                if (targetLink.second.targetTable == typeName)
                {
                    ++linkCount;
                }
            }

            if (linkCount == 1)
            {
                QualType type = getLinkType(link.first(), tableName, linkData.targetTable
                    , linkData.cardinality == catalog::relationship_cardinality_t::many, false, linkData.isValueLinked, loc);
                addField(&Context.Idents.get(link.first()), type, RD, loc);
                links_target_tables[linkData.targetTable]++;
                links_cardinality[linkData.targetTable] = linkData.cardinality == catalog::relationship_cardinality_t::many;
                links_from_parent[linkData.targetTable] = false;
                value_linked_link[linkData.targetTable] = linkData.isValueLinked;
            }
        }
    }

    // Insert fields and methods that are not part of the schema.  Note that we use the keyword 'remove' to
    // avoid conflicting with the C++ 'delete' keyword.
    addMethod(&Context.Idents.get("insert"), DeclSpec::TST_typename, {}, attrFactory, attrs, RD, loc, true, ParsedType::make(Context.getTagDeclType(RD)));
    addMethod(&Context.Idents.get("remove"), DeclSpec::TST_void, {}, attrFactory, attrs, RD, loc);
    addMethod(&Context.Idents.get("remove"), DeclSpec::TST_void, {Context.BoolTy}, attrFactory, attrs, RD, loc);
    addMethod(&Context.Idents.get("gaia_id"), DeclSpec::TST_int, {}, attrFactory, attrs, RD, loc);

    // connect and disconnect can be present only if the table has outgoing relationships.
    if (!links_target_tables.empty())
    {
        // For each outgoing relationship creates an overload connect/disconnect to the target types. eg:
        // incubator__type
        //   bool connect(actuator_type&);
        //   bool disconnect(actuator_type&);
        //   bool connect(farmer_type&);
        //   bool disconnect(farmer_type&);
        //   ....
        for (const auto& targetTablePair : links_target_tables)
        {
            // connect/disconnect are not appended to the table if there is more than one
            // link pointing to a target type.
            // The user will need to full qualify the relationship to use connect/disconnect:
            // incubator.actuators.connect(actuator);
            if (targetTablePair.second > 1 || value_linked_link[targetTablePair.first()])
            {
                continue;
            }

            addConnectDisconnect(RD, targetTablePair.first(), links_cardinality[targetTablePair.first()], links_from_parent[targetTablePair.first()], loc, attrFactory, attrs);
        }
    }
    RD->completeDefinition();

    return Context.getTagDeclType(RD);
}

QualType Sema::getFieldType(const std::string& fieldOrTagName, SourceLocation loc)
{
    DeclContext* context = getCurFunctionDecl();
    const llvm::StringMap<llvm::StringMap<QualType>>& tableData = getTableData();
    if (tableData.empty())
    {
        return Context.VoidTy;
    }

    const llvm::StringMap<std::string>& tagMapping = getTagMapping(getCurFunctionDecl(), loc);

    // Check if the fieldOrTagName is a reference to a table or to a tag.
    const auto& tagMappingIterator = tagMapping.find(fieldOrTagName);
    const auto& tableDataIterator = tableData.find(fieldOrTagName);
    if (tableDataIterator != tableData.end() || tagMappingIterator != tagMapping.end())
    {
        for (const auto& iterator : tableData)
        {
            if (iterator.second.find(fieldOrTagName) != iterator.second.end())
            {
                // TODO add information about the source of ambiguity.
                Diag(loc, diag::err_ambiguous_field_reference) << fieldOrTagName;
                return Context.VoidTy;
            }
        }

        if (tableDataIterator != tableData.end())
        {
            return getTableType(fieldOrTagName, loc);
        }
        else
        {
            return getTableType(tagMappingIterator->second, loc);
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
    SmallVector<string, 8> tables;
    RulesetDecl* rulesetDecl = dyn_cast<RulesetDecl>(context);
    RulesetTablesAttr* attr = rulesetDecl->getAttr<RulesetTablesAttr>();

    if (attr != nullptr)
    {
        for (const IdentifierInfo* id : attr->tables())
        {
            tables.push_back(id->getName());
        }
    }
    else
    {
        for (const auto& it : tableData)
        {
            tables.push_back(it.first());
        }
    }

    QualType result = Context.VoidTy;
    llvm::StringMap<QualType> foundTables;
    for (const string& tableName : tables)
    {
        // Search if there is a match in the table fields.

        const auto& tableDescription = tableData.find(tableName);
        if (tableDescription == tableData.end())
        {
            Diag(loc, diag::err_table_not_found) << tableName;
            return Context.VoidTy;
        }

        const auto& fieldDescription = tableDescription->second.find(fieldOrTagName);

        if (fieldDescription != tableDescription->second.end())
        {
            if (fieldDescription->second->isVoidType())
            {
                Diag(loc, diag::err_invalid_field_type) << fieldOrTagName;
                return Context.VoidTy;
            }
            foundTables[tableName] = fieldDescription->second;
        }
    }

    if (foundTables.empty())
    {
        Diag(loc, diag::err_unknown_field) << fieldOrTagName;
        return Context.VoidTy;
    }

    if (foundTables.size() == 1)
    {
        fieldTableName = foundTables.begin()->first();
        result = foundTables.begin()->second;
    }
    else
    {
        for (auto search_context_iterator = searchContextStack.rbegin();
            search_context_iterator != searchContextStack.rend(); ++search_context_iterator)
        {
            for(auto table_iterator = foundTables.begin(); table_iterator != foundTables.end(); ++table_iterator)
            {
                if (search_context_iterator->find(table_iterator->first()) != search_context_iterator->end())
                {
                    if (result == Context.VoidTy)
                    {
                        fieldTableName =table_iterator->first();
                        result = table_iterator->second;
                    }
                    else
                    {
                        Diag(loc, diag::err_duplicate_field) << fieldOrTagName;
                        return Context.VoidTy;
                    }
                }
            }
        }
        if (result == Context.VoidTy)
        {
            Diag(loc, diag::err_duplicate_field) << fieldOrTagName;
            return Context.VoidTy;
        }
    }

    if (IsInExtendedExplicitPathScope())
    {
        AddTableSearchAnchor(fieldTableName, fieldTableName);
    }

    return result;
}

// This method is a simplified version of getFieldType() used only to determine if a string
// is the name of a field in the database.
bool Sema::findFieldType(const std::string& fieldOrTagName, SourceLocation loc)
{

    const llvm::StringMap<llvm::StringMap<QualType>>& tableData = getTableData();
    if (tableData.empty())
    {
        return false;
    }

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
        return false;
    }

    SmallVector<string, 8> tables;
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
        for (const auto& it : tableData)
        {
            tables.push_back(it.first());
        }
    }

    for (const string& tableName : tables)
    {
        // Search if there is a match in the table fields.
        const auto& tableDescription = tableData.find(tableName);
        if (tableDescription == tableData.end())
        {
            return false;
        }
        const auto& fieldDescription = tableDescription->second.find(fieldOrTagName);

        if (fieldDescription != tableDescription->second.end())
        {
            if (fieldDescription->second->isVoidType())
            {
                Diag(loc, diag::err_invalid_field_type) << fieldOrTagName;
                return false;
            }
            return true;
        }
    }

    return false;
}

static bool parseTaggedAttribute(StringRef attribute, StringRef& table, StringRef& tag)
{
    size_t tag_position = attribute.find(':');
    if (tag_position != StringRef::npos)
    {
        tag = attribute.substr(0, tag_position);
    }
    else
    {
        return false;
    }
    size_t dot_position = attribute.find('.', tag_position + 1);
    // Handle fully qualified reference.
    if (dot_position != StringRef::npos)
    {
        table = attribute.substr(tag_position + 1, dot_position - tag_position - 1);
        return true;
    }

    table = attribute.substr(tag_position + 1);
    return true;
}

llvm::StringMap<std::string> Sema::getTagMapping(const DeclContext* context, SourceLocation loc)
{
    llvm::StringMap<std::string> result;
    const FunctionDecl* ruleContext = dyn_cast<FunctionDecl>(context);
    if (ruleContext == nullptr)
    {
        return result;
    }

    const GaiaOnUpdateAttr* update_attribute = ruleContext->getAttr<GaiaOnUpdateAttr>();
    const GaiaOnInsertAttr* insert_attribute = ruleContext->getAttr<GaiaOnInsertAttr>();
    const GaiaOnChangeAttr* change_attribute = ruleContext->getAttr<GaiaOnChangeAttr>();

    if (update_attribute != nullptr)
    {
        for (const auto& table_iterator : update_attribute->tables())
        {
            StringRef table, tag;
            if (parseTaggedAttribute(table_iterator, table, tag))
            {
                if (result.find(tag) != result.end())
                {
                    Diag(loc, diag::err_tag_redefined) << tag;
                    return llvm::StringMap<std::string>();
                }
                else
                {
                    result[tag] = table;
                }
            }
        }
    }

    if (insert_attribute != nullptr)
    {
        for (const auto& table_iterator : insert_attribute->tables())
        {
            StringRef table, tag;
            if (parseTaggedAttribute(table_iterator, table, tag))
            {
                if (result.find(tag) != result.end())
                {
                    Diag(loc, diag::err_tag_redefined) << tag;
                    return llvm::StringMap<std::string>();
                }
                else
                {
                    result[tag] = table;
                }
            }
        }
    }

    if (change_attribute != nullptr)
    {
        for (const auto& table_iterator : change_attribute->tables())
        {
            StringRef table, tag;
            if (parseTaggedAttribute(table_iterator, table, tag))
            {
                if (result.find(tag) != result.end())
                {
                    Diag(loc, diag::err_tag_redefined) << tag;
                    return llvm::StringMap<std::string>();
                }
                else
                {
                    result[tag] = table;
                }
            }
        }
    }

    for (const auto& explicitPathTagMapIterator : explicitPathTagMapping)
    {
        const auto& tagMap = explicitPathTagMapIterator.second;
        for (const auto& tagMapIterator : tagMap)
        {
            if (result.find(tagMapIterator.first()) != result.end())
            {
                Diag(loc, diag::err_tag_redefined) << tagMapIterator.first();
                return llvm::StringMap<std::string>();
            }
            else
            {
                result[tagMapIterator.first()] = tagMapIterator.second;
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
                if (result.find(tagMapIterator.first()) != result.end())
                {
                    Diag(loc, diag::err_tag_redefined) << tagMapIterator.first();
                    return llvm::StringMap<std::string>();
                }
                else
                {
                    result[tagMapIterator.first()] = tagMapIterator.second;
                }
            }
        }
    }

    return result;
}

NamedDecl* Sema::injectVariableDefinition(IdentifierInfo* II, SourceLocation loc, const string& explicitPath)
{
    QualType qualType = Context.VoidTy;
    StringRef firstComponent;

    string table = ParseExplicitPath(explicitPath, loc, firstComponent);
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

    if (firstComponent.empty())
    {
        firstComponent = fieldTableName;
    }
    const llvm::StringMap<std::string>& tagMapping = getTagMapping(getCurFunctionDecl(), loc);
    bool isReferenceDefined = false;
    StringRef anchorTable;
    StringRef anchorVariable;
    if (explicitPath.front() != '/' && explicitPath.rfind("@/") != 0 && !searchContextStack.empty())
    {
        bool isFirstComponentTagged = false;
        for (const auto& tagIterator : tagMapping)
        {
            if (tagIterator.second == firstComponent)
            {
                isFirstComponentTagged = true;
                break;
            }
        }

        if (!isFirstComponentTagged)
        {
            bool isFirstComponentTagDefinition = false;
            const auto& definedTagIterator = extendedExplicitPathTagMapping.find(loc);
            if (definedTagIterator != extendedExplicitPathTagMapping.end())
            {
                for (const auto& tagIterator : definedTagIterator->second)
                {
                    if (tagIterator.second == firstComponent)
                    {
                        isFirstComponentTagDefinition = true;
                        break;
                    }
                }
            }

            bool isAnchorFound = false;

            // Check if first component is a field. It should be the only component in the path.
            const llvm::StringMap<gaia::catalog::CatalogTableData>& catalogData = gaia::catalog::getCatalogTableData();
            StringRef firstComponentTable = firstComponent;
            if (catalogData.find(firstComponentTable) == catalogData.end())
            {
                firstComponentTable = fieldTableName;
            }
            bool skipTopSearchContext = !IsInExtendedExplicitPathScope();
            for (auto searchContextIterator = searchContextStack.rbegin();
                searchContextIterator != searchContextStack.rend(); ++searchContextIterator)
            {
                if (!skipTopSearchContext)
                {
                    skipTopSearchContext = true;
                    continue;
                }

                if (searchContextIterator->empty())
                {
                    continue;
                }

                int pathLength = INT_MAX;
                for (auto anchorTableIterator = searchContextIterator->begin();
                    anchorTableIterator != searchContextIterator->end(); ++anchorTableIterator)
                {
                    if (!isReferenceDefined && !isFirstComponentTagDefinition)
                    {
                        if (anchorTableIterator->second == firstComponent)
                        {
                            anchorTable = anchorTableIterator->first();
                            anchorVariable = anchorTableIterator->second;
                            isReferenceDefined = true;
                            break;
                        }
                    }

                    if (!isAnchorFound && !isReferenceDefined)
                    {
                        llvm::SmallVector<string, 8> path;
                        StringRef source_table = anchorTableIterator->first();
                        const auto& tagIterator = tagMapping.find(source_table);
                        if (tagIterator != tagMapping.end())
                        {
                            source_table = tagIterator->second;
                        }
                        // Find topographically shortest path between anchor table and destination table.
                        if (gaia::catalog::findNavigationPath(source_table, firstComponentTable, path, false))
                        {
                            if (path.size() < pathLength)
                            {
                                pathLength = path.size();
                                anchorTable = anchorTableIterator->first();
                                anchorVariable = anchorTableIterator->second;
                            }
                            else if (pathLength == path.size())
                            {
                                anchorTable = StringRef();
                            }
                        }
                    }
                }

                if (isReferenceDefined)
                {
                    break;
                }
                else if (!anchorTable.empty())
                {
                    isAnchorFound = true;
                }
            }
        }
    }

    DeclContext* context = getCurFunctionDecl();
    VarDecl* varDecl = VarDecl::Create(Context, context, loc, loc, II, qualType, Context.getTrivialTypeSourceInfo(qualType, loc), SC_None);
    varDecl->setLexicalDeclContext(context);
    varDecl->setImplicit();

    varDecl->addAttr(GaiaFieldAttr::CreateImplicit(Context));
    varDecl->addAttr(FieldTableAttr::CreateImplicit(Context, &Context.Idents.get(fieldTableName)));

    SourceLocation startLocation, endLocation;
    std::string path;

    if (!anchorTable.empty())
    {
        varDecl->addAttr(GaiaAnchorAttr::CreateImplicit(Context,
            &Context.Idents.get(anchorTable),
            &Context.Idents.get(anchorVariable),
            IsInExtendedExplicitPathScope(),
            isReferenceDefined));
    }

    if (GetExplicitPathData(loc, startLocation, endLocation, path))
    {
        llvm::StringMap<std::string> tagMapping = getTagMapping(getCurFunctionDecl(), loc);
        SmallVector<StringRef, 4> argPathComponents, argTagKeys, argTagTables,
            argDefinedTagKeys, argDefinedTagTables;
        for (const auto& pathComponentsIterator : explicitPathData[loc].path)
        {
            argPathComponents.push_back(ConvertString(pathComponentsIterator, loc));
        }

        for (const auto& tagsIterator : explicitPathData[loc].tagMap)
        {
            argTagKeys.push_back(ConvertString(tagsIterator.first(), loc));
            argTagTables.push_back(ConvertString(tagsIterator.second, loc));
            argDefinedTagKeys.push_back(ConvertString(tagsIterator.second, loc));
            argDefinedTagTables.push_back(ConvertString(tagsIterator.first(), loc));
        }

        for (const auto& tagsIterator : explicitPathTagMapping[loc])
        {
            argTagKeys.push_back(ConvertString(tagsIterator.first(), loc));
            argTagTables.push_back(ConvertString(tagsIterator.second, loc));
        }
        for (const auto& tagsIterator : extendedExplicitPathTagMapping[loc])
        {
            argTagKeys.push_back(ConvertString(tagsIterator.first(), loc));
            argTagTables.push_back(ConvertString(tagsIterator.second, loc));
        }
        for (const auto& tagsIterator : tagMapping)
        {
            argTagKeys.push_back(ConvertString(tagsIterator.first(), loc));
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
    explicitPathData[location] = {startLocation, endLocation, explicitPath, SmallVector<std::string, 8>(), llvm::StringMap<std::string>()};
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
    bool returnValue = true;
    if (range.isValid())
    {
        returnValue = false;
        for (auto tableIterator = extendedExplicitPathTagMapping.begin(); tableIterator != extendedExplicitPathTagMapping.end();)
        {
            SourceLocation tagLocation = tableIterator->first;
            if (tagLocation == range.getBegin() || (range.getBegin() < tagLocation && tagLocation < range.getEnd()))
            {
                auto toErase = tableIterator;
                extendedExplicitPathTagMapping.erase(toErase);
                ++tableIterator;
                returnValue = true;
            }
            else
            {
                ++tableIterator;
            }
        }
    }
    return returnValue;
}

bool Sema::IsExplicitPathInRange(SourceRange range) const
{
    if (range.isValid())
    {
        for (auto tableIterator = extendedExplicitPathTagMapping.begin(); tableIterator != extendedExplicitPathTagMapping.end();)
        {
            SourceLocation tagLocation = tableIterator->first;
            if (tagLocation == range.getBegin() || (range.getBegin() < tagLocation && tagLocation < range.getEnd()))
            {
                return true;
            }
            else
            {
                ++tableIterator;
            }
        }
    }
    return false;
}

void Sema::ActOnStartDeclarativeLabel(StringRef label)
{
    declarativeLabelsInProcess.insert(label);
}

bool Sema::ActOnStartLabel(StringRef label)
{
    if (declarativeLabelsInProcess.find(label) != declarativeLabelsInProcess.end())
    {
        return false;
    }
    labelsInProcess.insert(label);
    return true;
}

bool Sema::ValidateLabel(const LabelDecl* label)
{
    StringRef labelName = label->getName();

    // Check if there is a declarative label which is not currently processed as a regular label.
    for (const auto& declarativeLabel : declarativeLabelsInProcess)
    {
        if (labelsInProcess.find(declarativeLabel.first()) == labelsInProcess.end())
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

void Sema::ActOnRuleStart()
{
    ResetTableSearchContextStack();
    labelsInProcess.clear();
    declarativeLabelsInProcess.clear();
    explicitPathData.clear();
    explicitPathTagMapping.clear();
    extendedExplicitPathTagMapping.clear();
    injectedVariablesLocation.clear();
}
