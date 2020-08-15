/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>

#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "gaia_catalog.h"
#include "gaia_catalog.hpp"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;
using namespace clang::ast_matchers;
using namespace gaia;

cl::OptionCategory ASTGeneratorCategory("Use ASTGenerator options");
cl::opt<string> ASTGeneratorOutputOption("output", cl::init(""), 
    cl::desc("output file name"), cl::cat(ASTGeneratorCategory));
cl::opt<bool> ASTGeneratorVerboseOption("v",
    cl::desc("print parse tokens"), cl::cat(ASTGeneratorCategory));

std::string curRuleset;
bool g_verbose = false;
bool generationError = false;

vector<string> rulesets;
unordered_map<string, unordered_set<string>>  active_Fields;
unordered_set<string>  used_Tables;
const FunctionDecl *current_Rule_Declaration = nullptr;
struct FieldData
{
    bool isActive;
    int position;
};
unordered_map<string, unordered_map<string, FieldData>> field_Data;
string current_Ruleset_Subscription;
string generated_Subscription_Code;
string current_Ruleset_UnSubscription;
struct TableLinkData
{
    string table;
    string field;
};
unordered_multimap<string, TableLinkData> table_Relationship_1;
unordered_multimap<string, TableLinkData> table_Relationship_N;

struct NavigationData
{
    string name;
    string linkingField;
    bool isParent;
};

struct NavigationCodeData
{
    string prefix;
    string postfix;
};

string generateGeneralSubscriptionCode()
{
    string retVal = "namespace gaia {\n namespace rules{\nextern \"C\" void subscribe_ruleset(const char* ruleset_name)\n{\n";

    for (string ruleset : rulesets)
    {
        retVal += "if (strcmp(ruleset_name, \"" + ruleset + "\") == 0)\n" \
            "{\n::" + ruleset + "::subscribeRuleset_" + ruleset + "();\nreturn;\n}\n";
    }

    retVal += "throw ruleset_not_found(ruleset_name);\n}\n" \
        "extern \"C\" void unsubscribe_ruleset(const char* ruleset_name)\n{\n";

    for (string ruleset : rulesets)
    {
        retVal += "if (strcmp(ruleset_name, \"" + ruleset + "\") == 0)\n" \
            "{\n::" + ruleset + "::unsubscribeRuleset_" + ruleset + "();\nreturn;\n}\n";
    }

    retVal += "throw ruleset_not_found(ruleset_name);\n}\n" \
        "extern \"C\" void initialize_rules()\n{\n";
    for (string ruleset : rulesets)
    {
        retVal += "::" + ruleset + "::subscribeRuleset_" + ruleset + "();\n" ;
    }
    retVal += "}\n}\n}";

    return retVal;
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

unordered_map<string, unordered_map<string, FieldData>> getTableData()
{
    unordered_map<string, unordered_map<string, FieldData>> retVal;
    try 
    {
        DBMonitor monitor;
    
        for (catalog::gaia_table_t table = catalog::gaia_table_t::get_first(); 
            table; table = table.get_next())
        {
            unordered_map<string, FieldData> fields;
            retVal[table.name()] = fields;
        }

        for (catalog::gaia_field_t field = catalog::gaia_field_t::get_first(); 
            field; field = field.get_next())
        {
            if (static_cast<gaia::catalog::data_type_t>(field.type()) != gaia::catalog::data_type_t::e_references)
            {
                catalog::gaia_table_t tbl = catalog::gaia_table_t::get(field.table_id());
                if (!tbl)
                {
                    llvm::errs() << "Incorrect table for field " << field.name() << "\n";
                    generationError = true;
                    return unordered_map<string, unordered_map<string, FieldData>>();
                }
                unordered_map<string, FieldData> fields = retVal[tbl.name()];
                if (fields.find(field.name()) != fields.end())
                {
                    llvm::errs() << "Duplicate field " << field.name() << "\n";
                    generationError = true;
                    return unordered_map<string, unordered_map<string, FieldData>>();
                }
                FieldData fieldData;
                fieldData.isActive = field.active();
                fieldData.position = field.position();
                fields[field.name()] = fieldData;
                retVal[tbl.name()] = fields;
            }
            else
            {
                gaia_id_t parentTableId = field.table_id();
                catalog::gaia_table_t parentTable = catalog::gaia_table_t::get(parentTableId);
                if (!parentTable)
                {
                    llvm::errs() << "Incorrect table for field " << field.name() << "\n";
                    generationError = true;
                    return unordered_map<string, unordered_map<string, FieldData>>();
                }

                gaia_id_t childTableId = field.type_id();
                catalog::gaia_table_t childTable = catalog::gaia_table_t::get(childTableId);
                if (!childTable)
                {
                    llvm::errs() << "Incorrect table referenced by field " << field.name() << "\n";
                    generationError = true;
                    return unordered_map<string, unordered_map<string, FieldData>>();
                }
                TableLinkData linkData1;
                linkData1.table = childTable.name();
                linkData1.field = field.name();
                TableLinkData linkDataN;
                linkDataN.table = parentTable.name();
                linkDataN.field = field.name();

                table_Relationship_1.emplace(parentTable.name(), linkData1);
                table_Relationship_N.emplace(childTable.name(), linkDataN);
            }
            
        }
    }
    catch (exception e)
    {
        llvm::errs() << "Exception while processing the catalog " << e.what() << "\n";
        generationError = true;
        return unordered_map<string, unordered_map<string, FieldData>>();
    }
    return retVal;
}

void log(const char* message, const char* value)
{
    if (g_verbose)
    {
        printf(message, value);
    }
}

void log(const char* message, const std::string& value)
{
    log(message, value.c_str());
}

// Trim from start.
string &ltrim(string &s) 
{
    s.erase(s.begin(), 
        find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
    return s;
}

// Trim from end.
string &rtrim(string &s) 
{
    s.erase(
        find_if(s.rbegin(), s.rend(), 
            not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}

// Trim from both ends.
string &trim(string &s) 
{
    return ltrim(rtrim(s));
}

string removeSemicolon(string s)
{
    if (!s.empty() && *s.rbegin() == ';')
    {
        return s.substr(0, s.size()-1);
    }
    else
    {
        return s;
    }
    
}

void split(const string &text, vector<string>& tokens, char separator) 
{
    size_t start = 0, end = 0;
    string token;   
    
    while ((end = text.find(separator, start)) != string::npos) 
    {
        token = text.substr(start, end - start);
        tokens.push_back(trim(token));
        log("token: %s\n", tokens.back());
        start = end + 1;
    }
    token  = text.substr(start);
    tokens.push_back(trim(token));
    log("token: %s\n", tokens.back());
}

string getTableName(const Decl *decl)
{
    const FieldTableAttr *tableAttr = decl->getAttr<FieldTableAttr>();
    if (tableAttr != nullptr)
    {
        return tableAttr->getTable()->getName().str();
    }
    return "";
}

string insertRulePreamble(string rule, string preamble)
{
    size_t ruleCodeStart = rule.find('{');
    return "{" + preamble + rule.substr(ruleCodeStart + 1);
}

bool checkCircularLink(const string& table, vector<NavigationData> &currentPath)
{
    for (auto p : currentPath)
    {
        if (p.name == table)
        {
            return true;
        }
    }

    return false;
}

bool findNavigationPath(const string& src, const string& dst, vector<NavigationData> &currentPath)
{
    if (src == dst)
    {
        return true;
    }

    auto parentItr = table_Relationship_1.equal_range(src);
    for (auto it = parentItr.first; it != parentItr.second; ++it)
    {      
        if (it != table_Relationship_1.end())
        {
            if (!checkCircularLink(it->second.table,currentPath))
            {
                NavigationData data = { it->second.table, it->second.field, true };
                currentPath.push_back(data);
                if (it->second.table == dst)
                {              
                    return true;
                }
                if (findNavigationPath(it->second.table, dst, currentPath))
                {
                    return true;
                }
                else
                {
                    currentPath.pop_back();
                }
            }
        }
    }

    auto childItr = table_Relationship_N.equal_range(src);
    for (auto it = childItr.first; it != childItr.second; ++it)
    {
        if (it != table_Relationship_N.end())
        {
            if (!checkCircularLink(it->second.table,currentPath))
            {
                NavigationData data = { it->second.table, it->second.field, false };
                currentPath.push_back(data);
                if (it->second.table == dst)
                {              
                    return true;
                }
                if (findNavigationPath(it->second.table, dst, currentPath))
                {
                    return true;
                }
                else
                {
                    currentPath.pop_back();
                }
            }
        }
    }
    return false;
}


NavigationCodeData generateNavigationCode(string anchorTable)
{
    NavigationCodeData retVal;
    retVal.prefix = "\ngaia::" + curRuleset+ "::" + anchorTable + "_t " + anchorTable + " = " + 
        "gaia::" + curRuleset+ "::" + anchorTable + "_t::get(context->record);\n";
    //single table used in the rule
    if (used_Tables.size() == 1 && used_Tables.find(anchorTable) != used_Tables.end())
    {
        return retVal;
    }

    if (used_Tables.empty())
    {
        generationError = true;
        llvm::errs() << "No tables are used in the rule \n";
        return NavigationCodeData();
    }
    if (used_Tables.find(anchorTable) == used_Tables.end())
    {
        generationError = true;
        llvm::errs() << "Table " << anchorTable <<" is not used in the rule \n";
        return NavigationCodeData();
    }

    if (table_Relationship_1.find(anchorTable) == table_Relationship_1.end() &&
        table_Relationship_N.find(anchorTable) == table_Relationship_N.end())
    {
        generationError = true;
        llvm::errs() << "Table " << anchorTable << " doesn't reference any table and not referenced by any other tables";
        return NavigationCodeData();
    }
    auto parentItr = table_Relationship_1.equal_range(anchorTable);
    auto childItr = table_Relationship_N.equal_range(anchorTable);    
    for (string table : used_Tables)
    {
        bool is1Relationship = false, isNRelationship = false;
        if (table == anchorTable)
        {
            continue;
        }
        string linkingField;
        for (auto it = parentItr.first; it != parentItr.second; ++it)
        {
            if (it != table_Relationship_1.end() && it->second.table == table)
            {
                if (is1Relationship)
                {
                    generationError = true;
                    llvm::errs() << "More then one field that links " << anchorTable << " and " << table << "\n";
                    return NavigationCodeData();
                }
                is1Relationship = true;
                linkingField = it->second.field;
            }
        }

        for (auto it = childItr.first; it != childItr.second; ++it)
        {
            if (it != table_Relationship_N.end() && it->second.table == table)
            {
                if (isNRelationship)
                {
                    generationError = true;
                    llvm::errs() << "More then one field that links " << anchorTable << " and " << table << "\n";
                    return NavigationCodeData();
                }
                isNRelationship = true;
                linkingField = it->second.field;
            }
        }

        if (is1Relationship && isNRelationship)
        {
            generationError = true;
            llvm::errs() << "Both relationships exist between tables " << anchorTable << " and " << table << "\n";
            return NavigationCodeData();
        }


        if (!is1Relationship && !isNRelationship)
        {
            vector<NavigationData> path;
            if (findNavigationPath(anchorTable, table, path))
            {
                string srcTbl = anchorTable;
                for (auto p : path)
                {
                    if (p.isParent)
                    {
                        retVal.prefix += p.name + "_t " + p.name + " = " + srcTbl + "." + p.linkingField + p.name + "_owner());\n";
                    }
                    else
                    {
                        retVal.prefix += "for (auto " + p.name + " : " + srcTbl + "." + p.linkingField + p.name + "_list)\n{\n";
                        retVal.postfix += "}\n";
                    }
                    srcTbl = p.name;
                }
            }
            else
            {
                generationError = true;
                llvm::errs() << "No path between tables " << anchorTable << " and " << table << "\n";
                return NavigationCodeData();
            }
        }
        else
        {
            if (is1Relationship)
            {
                retVal.prefix += table + "_t " + table + " = " + anchorTable + "." + linkingField + table + "_owner());\n";
            }
            else
            {
                retVal.prefix += "for (auto " + table + " : " + anchorTable + "." + linkingField + table + "_list)\n{\n";
                retVal.postfix += "}\n";
            }
        }
    }

    return retVal;
}

void generateRules(Rewriter &rewriter)
{
    if (current_Rule_Declaration == nullptr)
    {
        return;
    }
    if (active_Fields.empty())
    {
        llvm::errs() << "No active fields for the rule\n";
        generationError = true;
        return;
    }

    string ruleCode = rewriter.getRewrittenText(current_Rule_Declaration->getSourceRange());
    int ruleCnt = 1;
    for (auto fd : active_Fields)
    {
        string table = fd.first;
        bool containsLastOperation = false;
        bool containsFields = false;
        string fieldSubscriptionCode;
        string commonSubscriptionCode;
        if (field_Data.find(table) == field_Data.end())
        {
            llvm::errs() << "No table " << table << " found in the catalog\n";
            generationError = true;
            return;
        }
        string ruleName = curRuleset + "_" + current_Rule_Declaration->getName().str() + "_" + to_string(ruleCnt);
        commonSubscriptionCode = "rule_binding_t " + ruleName + "binding(" +
            "\"" + curRuleset + "\",\"" + ruleName + "\"," + curRuleset + "::" + ruleName + ");\n";
        fieldSubscriptionCode =  "field_position_list_t fields_" + ruleName + ";\n";

        auto fields = field_Data[table];
        for (auto field : fd.second)
        {
            bool isLastOperation = field == "LastOperation";
            if (!containsLastOperation && isLastOperation )
            {
                containsLastOperation = true;
            }
            if (!containsFields && !isLastOperation)
            {
                containsFields = true;
            }
            if (!isLastOperation && fields.find(field) == fields.end())
            {
                llvm::errs() << "No field " << field << " found in the catalog\n";
                generationError = true;
                return;
            }
            FieldData fieldData = fields[field];
           /* if (!fieldData.isActive)
            {
                llvm::errs() << "Field " << field << " is not marked as active in the catalog\n";
                generationError = true;
                return;
            }*/

            if (!isLastOperation)
            {
                fieldSubscriptionCode += "fields_" + ruleName + ".insert(" + to_string(fieldData.position) +");\n";
            }            
        }

        if (!containsFields && !containsLastOperation)
        {
            llvm::errs() << "No fields referred by table " + table +"\n";
            generationError = true;
            return;
        }

        current_Ruleset_Subscription += commonSubscriptionCode;
        current_Ruleset_UnSubscription += commonSubscriptionCode;

        if (containsFields)
        {
            current_Ruleset_Subscription += fieldSubscriptionCode + "subscribe_rule(gaia::" + curRuleset + "::" + table +
             "_t::s_gaia_type, event_type_t::row_update, fields_" + ruleName + 
                "," + ruleName + "binding);\n";
            current_Ruleset_UnSubscription += fieldSubscriptionCode + "unsubscribe_rule(gaia::" + curRuleset + "::" + table +
             "_t::s_gaia_type, event_type_t::row_update, fields_" + ruleName + 
                "," + ruleName + "binding);\n";
        }
                

        if (containsLastOperation)
        {
            current_Ruleset_Subscription += "subscribe_rule(gaia::" + curRuleset + "::" + table +
             "_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields," + 
                ruleName + "binding);\n";
            current_Ruleset_Subscription += "subscribe_rule(gaia::" + curRuleset + "::" + table +
             "_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields," + 
                ruleName + "binding);\n";
            current_Ruleset_Subscription += "subscribe_rule(gaia::" + curRuleset + "::" + table +
             "_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields," + 
                ruleName + "binding);\n";

            current_Ruleset_UnSubscription += "unsubscribe_rule(gaia::" + curRuleset + "::" + table +
             "_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields," + 
                ruleName + "binding);\n";
            current_Ruleset_UnSubscription += "unsubscribe_rule(gaia::" + curRuleset + "::" + table +
             "_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields," + 
                ruleName + "binding);\n";
            current_Ruleset_UnSubscription += "unsubscribe_rule(gaia::" + curRuleset + "::" + table +
             "_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields," + 
                ruleName + "binding);\n";
        }
        
        NavigationCodeData navigationCode = generateNavigationCode(table);
       
        if(ruleCnt == 1)
        {
            rewriter.InsertText(current_Rule_Declaration->getLocation(),"\nvoid " + ruleName + "(const rule_context_t* context)\n");
            rewriter.InsertTextAfterToken(current_Rule_Declaration->getLocation(), navigationCode.prefix);
            rewriter.InsertText(current_Rule_Declaration->getEndLoc(),navigationCode.postfix);
        }
        else
        {
            rewriter.InsertTextBefore(current_Rule_Declaration->getLocation(),"\nvoid " + ruleName + "(const rule_context_t* context)\n"
               + insertRulePreamble (ruleCode + navigationCode.postfix, navigationCode.prefix));
        }
        
        ruleCnt++;
    }
}

class Field_Get_Match_Handler : public MatchFinder::MatchCallback
{
public:
    Field_Get_Match_Handler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        if (generationError)
        {
            return;
        }
        const DeclRefExpr *exp = Result.Nodes.getNodeAs<DeclRefExpr>("fieldGet");
        const MemberExpr  *memberExpr = Result.Nodes.getNodeAs<MemberExpr>("tableFieldGet");
        string tableName;
        string fieldName;
        SourceRange expSourceRange;
        bool isLastOperation = false;
        if (exp != nullptr)
        {
            const ValueDecl *decl = exp->getDecl();
            if (decl->getType()->isStructureType())
            {
                return;
            }

            tableName = getTableName(decl);
            fieldName = decl->getName().str();
            used_Tables.insert(tableName);

            
            if (decl->hasAttr<GaiaFieldAttr>())
            {
                expSourceRange = SourceRange(exp->getLocation(),exp->getEndLoc());
                active_Fields[tableName].insert(fieldName);
            }
            else if (decl->hasAttr<GaiaFieldValueAttr>())
            {
                expSourceRange = SourceRange(exp->getLocation().getLocWithOffset(-1),exp->getEndLoc());
            }
        }
        else if (memberExpr != nullptr)
        {
            DeclRefExpr *declExpr = dyn_cast<DeclRefExpr>(memberExpr->getBase());
            if (declExpr != nullptr)
            {
                fieldName = memberExpr->getMemberNameInfo().getName().getAsString();
                tableName = declExpr->getDecl()->getName().str();
                
                isLastOperation = declExpr->getDecl()->hasAttr<GaiaLastOperationAttr>();

                used_Tables.insert(tableName);

                if (declExpr->getDecl()->hasAttr<GaiaFieldValueAttr>())
                {
                    expSourceRange = SourceRange(memberExpr->getBeginLoc().getLocWithOffset(-1), 
                        memberExpr->getEndLoc());
                }
                else
                {
                    expSourceRange = SourceRange(memberExpr->getBeginLoc(),
                        memberExpr->getEndLoc());
                    active_Fields[tableName].insert(fieldName);
                }   
            }
            else
            {
                llvm::errs() << "Incorrect Base Type of generated type\n";
                generationError = true;
            }            
        }
        else
        {
            llvm::errs() << "Incorrect matched expression\n";
            generationError = true;
        }
        
        if (expSourceRange.isValid())
        {
            if (isLastOperation)
            {
                rewriter.ReplaceText(expSourceRange, "context->last_operation(gaia::" + curRuleset + "::" + tableName + "_t::s_gaia_type)");
            }
            else
            {
                rewriter.ReplaceText(expSourceRange, tableName + "." + fieldName + "()");
            }
        }
    }

private:
    Rewriter &rewriter;
};

class Field_Set_Match_Handler : public MatchFinder::MatchCallback
{
public:
    Field_Set_Match_Handler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        if (generationError)
        {
            return;
        }
        const BinaryOperator *op = Result.Nodes.getNodeAs<BinaryOperator>("fieldSet");
        if (op != nullptr)
        {
            const Expr *opExp = op->getLHS();
            if (opExp != nullptr)
            {
                const DeclRefExpr *leftDeclExpr = dyn_cast<DeclRefExpr>(opExp);
                const MemberExpr  *memberExpr = dyn_cast<MemberExpr>(opExp);
        
                string tableName;
                string fieldName;
                SourceLocation startLocation;
                SourceLocation setLocEnd;
                if (leftDeclExpr != nullptr || memberExpr != nullptr)
                {
                    if (leftDeclExpr != nullptr)
                    {
                        const ValueDecl *opDecl = leftDeclExpr->getDecl();
                        if (opDecl->getType()->isStructureType())
                        {
                            return;
                        }
                        tableName = getTableName(opDecl);
                        fieldName = opDecl->getName().str();
                        startLocation = leftDeclExpr->getLocation();
                    }
                    else
                    {
                        DeclRefExpr *declExpr = dyn_cast<DeclRefExpr>(memberExpr->getBase());
                        if (declExpr == nullptr)
                        {
                            llvm::errs() << "Incorrect Base Type of generated type\n";
                            generationError = true;
                            return;
                        }
                        fieldName = memberExpr->getMemberNameInfo().getName().getAsString();
                        tableName = declExpr->getDecl()->getName().str();
                        startLocation = memberExpr->getBeginLoc();
                        
                    }
                    used_Tables.insert(tableName);
                
                    tok::TokenKind tokenKind;
                    std::string replacementText = "[&]() mutable {" + 
                        tableName + "_writer w = " + tableName + ".writer(); w." + 
                        fieldName;

                    switch(op->getOpcode())
                    {
                        case BO_Assign:
                        {
                            tokenKind = tok::equal;
                            break;
                        }
                        case BO_MulAssign:
                        {
                            tokenKind = tok::starequal;
                            break;
                        }
                        case BO_DivAssign:
                        {
                            tokenKind = tok::slashequal;
                            break;
                        }
                        case BO_RemAssign:
                        {
                            tokenKind = tok::percentequal;
                            break;
                        }
                        case BO_AddAssign:
                        {
                            tokenKind = tok::plusequal;
                            break;
                        }
                        case BO_SubAssign:
                        {
                            tokenKind = tok::minusequal;
                            break;
                        }
                        case BO_ShlAssign:
                        {
                            tokenKind = tok::lesslessequal;
                            break;
                        }
                        case BO_ShrAssign:
                        {
                            tokenKind = tok::greatergreaterequal;
                            break;
                        }
                        case BO_AndAssign:
                        {
                            tokenKind = tok::ampequal;
                            break;
                        }
                        case BO_XorAssign:
                        {
                            tokenKind = tok::caretequal;
                            break;
                        }
                        case BO_OrAssign:
                        {
                            tokenKind = tok::pipeequal;
                            break;
                        }
                        default:
                            llvm::errs() << "Incorrect Operator type\n";
                            generationError = true;
                            return;
                    }

                    if (op->getOpcode() != BO_Assign)
                    {
                        replacementText += ConvertCompoundBinaryOpcode(op->getOpcode());
                        active_Fields[tableName].insert(fieldName);
                    }
                    else
                    {
                        replacementText += "=";
                    }
                                        
                    
                    if (leftDeclExpr != nullptr)
                    {
                        setLocEnd = Lexer::findLocationAfterToken(
                            startLocation,tokenKind,rewriter.getSourceMgr(),
                            rewriter.getLangOpts(),true);
                    }
                    else
                    {
                        setLocEnd = Lexer::findLocationAfterToken(
                            memberExpr->getExprLoc(),tokenKind,rewriter.getSourceMgr(),
                            rewriter.getLangOpts(),true);
                    }
                    
                    rewriter.ReplaceText(
                        SourceRange(startLocation,setLocEnd.getLocWithOffset(-1)), 
                        replacementText);
                    //rewriter.InsertTextAfterToken(op->getEndLoc(),")");
                    if (op->getOpcode() != BO_Assign)
                    {
                        rewriter.InsertTextAfterToken(op->getEndLoc(),
                            "; w.update_row();return " + 
                            tableName + "." + fieldName + "();}() ");

                    }
                    else
                    {
                        rewriter.InsertTextAfterToken(op->getEndLoc(),
                            "; w.update_row();return " +  
                            tableName + "." + fieldName + "();}()");
                    }
                }
                else
                {
                    llvm::errs() << "Incorrect Operator Expression Type\n";
                    generationError = true;
                }
            }
            else
            {
                llvm::errs() << "Incorrect Operator Expression\n";
                generationError = true;
            }
        }
        else
        {
            llvm::errs() << "Incorrect Matched operator\n";
            generationError = true;
        }
    }

private:
    std::string ConvertCompoundBinaryOpcode(BinaryOperator::Opcode opcode)
    {
        switch(opcode)
        {
            case BO_MulAssign:
                return  "*=";
            case BO_DivAssign:
                return  "/=";
            case BO_RemAssign:
                return  "%=";
            case BO_AddAssign:
                return  "+=";
            case BO_SubAssign:
                return  "-=";
            case BO_ShlAssign:
                return  "<<=";
            case BO_ShrAssign:
                return  ">>=";
            case BO_AndAssign:
                return  "&=";
            case BO_XorAssign:
                return  "^=";
            case BO_OrAssign:
                return  "|=";
            default:
                llvm::errs() << "Incorrect Operator Code " << opcode <<"\n";
                generationError = true;
                return "";
        }
    }

    Rewriter &rewriter;
};

class Field_Unary_Operator_Match_Handler : public MatchFinder::MatchCallback
{
public:
    Field_Unary_Operator_Match_Handler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        if (generationError)
        {
            return;
        }
        const UnaryOperator *op = Result.Nodes.getNodeAs<UnaryOperator>("fieldUnaryOp");
        if (op != nullptr)
        {
            const Expr *opExp = op->getSubExpr();
            if (opExp != nullptr)
            {
                const DeclRefExpr *declExpr = dyn_cast<DeclRefExpr>(opExp);
                const MemberExpr  *memberExpr = dyn_cast<MemberExpr>(opExp);
        
                if (declExpr != nullptr || memberExpr != nullptr)
                {
                    std::string replaceStr;
                    string tableName;
                    string fieldName;

                    if (declExpr != nullptr)
                    {
                        
                        const ValueDecl *opDecl = declExpr->getDecl();
                        if (opDecl->getType()->isStructureType())
                        {
                            return;
                        }
                    
                        tableName = getTableName(opDecl);
                        fieldName = opDecl->getName().str();
                    }
                    else
                    {
                        DeclRefExpr *declExpr = dyn_cast<DeclRefExpr>(memberExpr->getBase());
                        if (declExpr == nullptr)
                        {
                            llvm::errs() << "Incorrect Base Type of generated type\n";
                            generationError = true;
                            return;
                        }
                        fieldName = memberExpr->getMemberNameInfo().getName().getAsString();
                        tableName = declExpr->getDecl()->getName().str();
                    }

                    used_Tables.insert(tableName);
                    active_Fields[tableName].insert(fieldName);
                                    
                    if (op->isPostfix())
                    {
                        if (op->isIncrementOp())
                        {
                            replaceStr = "[&]() mutable {auto t=" + 
                                tableName + "." + fieldName + "();" + 
                                tableName + "_writer w = " + tableName + ".writer(); w." + 
                                fieldName + "++; w.update_row();return t;}()";

                        }
                        else if(op->isDecrementOp())
                        {
                            replaceStr = "[&]() mutable {auto t=" + 
                                tableName + "." + fieldName + "();" + 
                                tableName + "_writer w = " + tableName + ".writer(); w." + 
                                fieldName + "--; w.update_row();return t;}()";
                        }
                    }
                    else
                    {
                        if (op->isIncrementOp())
                        {
                            replaceStr = "[&]() mutable {" + 
                                tableName + "_writer w = " + tableName + ".writer(); ++ w." + 
                                fieldName + ";w.update_row(); return w." +
                                fieldName + ";}()";
                        }
                        else if(op->isDecrementOp())
                        {
                            replaceStr = "[&]() mutable {" + 
                                tableName + "_writer w = " + tableName + ".writer(); -- w." + 
                                fieldName + ";w.update_row(); return w." +
                                fieldName + ";}()";
                        }
                    }

                    rewriter.ReplaceText(
                        SourceRange(op->getBeginLoc().getLocWithOffset(-1),op->getEndLoc().getLocWithOffset(1)), 
                        replaceStr);
                }
                else
                {
                    llvm::errs() << "Incorrect Operator Expression Type\n";
                    generationError = true;
                }
            }
            else
            {
                llvm::errs() << "Incorrect Operator Expression\n";
                generationError = true;
            }
        }
        else
        {
            llvm::errs() << "Incorrect Matched Operator\n";
            generationError = true;
        }
    }

private:
    Rewriter &rewriter;
};

class Rule_Match_Handler : public MatchFinder::MatchCallback
{
public:
    Rule_Match_Handler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        if (generationError)
        {
            return;
        }
        const FunctionDecl * ruleDecl = Result.Nodes.getNodeAs<FunctionDecl>("ruleDecl");
        generateRules(rewriter);
        if (generationError)
        {
            return;
        }
        current_Rule_Declaration = ruleDecl;
        used_Tables.clear();
        active_Fields.clear();
    }

private:   
    Rewriter &rewriter;
};

class Ruleset_Match_Handler : public MatchFinder::MatchCallback
{
public:
    Ruleset_Match_Handler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        if (generationError)
        {
            return;
        }
        generateRules(rewriter);
        if (generationError)
        {
            return;
        }
        current_Rule_Declaration = nullptr;
        used_Tables.clear();
        active_Fields.clear();

        const RulesetDecl * rulesetDecl = Result.Nodes.getNodeAs<RulesetDecl>("rulesetDecl");
        if (rulesetDecl != nullptr)
        {
            if (!curRuleset.empty())
            {
                generated_Subscription_Code += "namespace " + curRuleset +
                    "{\nvoid subscribeRuleset_" + 
                    rulesetDecl->getName().str() + "()\n{\n" + current_Ruleset_Subscription + 
                    "}\nvoid unsubscribeRuleset_" + 
                    rulesetDecl->getName().str() + "()\n{\n" + current_Ruleset_UnSubscription + 
                    "}\n}\n";
            }
            curRuleset = rulesetDecl->getName().str();   
            rulesets.push_back(curRuleset);
            current_Ruleset_Subscription.clear(); 
            current_Ruleset_UnSubscription.clear();       
            rewriter.ReplaceText(
                SourceRange(rulesetDecl->getBeginLoc(),rulesetDecl->decls_begin()->getBeginLoc().getLocWithOffset(-2)),
                "namespace " + curRuleset + "\n{\n");
        }
    }

private:
    Rewriter &rewriter;
};

class Update_Match_Handler : public MatchFinder::MatchCallback
{
public:
    Update_Match_Handler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        const DeclRefExpr *exp = Result.Nodes.getNodeAs<DeclRefExpr>("UPDATE");
        if (exp != nullptr)
        {
            rewriter.ReplaceText(
                SourceRange(exp->getLocation(),exp->getEndLoc()),
                "gaia::rules::last_operation_t::row_update");
        }
    }

private:
    Rewriter &rewriter;
};

class Insert_Match_Handler : public MatchFinder::MatchCallback
{
public:
    Insert_Match_Handler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        const DeclRefExpr *exp = Result.Nodes.getNodeAs<DeclRefExpr>("INSERT");
        if (exp != nullptr)
        {
            rewriter.ReplaceText(
                SourceRange(exp->getLocation(),exp->getEndLoc()),
                "gaia::rules::last_operation_t::row_insert");
        }
    }

private:
    Rewriter &rewriter;
};

class Delete_Match_Handler : public MatchFinder::MatchCallback
{
public:
    Delete_Match_Handler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        const DeclRefExpr *exp = Result.Nodes.getNodeAs<DeclRefExpr>("DELETE");
        if (exp != nullptr)
        {
            rewriter.ReplaceText(
                SourceRange(exp->getLocation(),exp->getEndLoc()),
                "gaia::rules::last_operation_t::row_delete");
        }
    }

private:
    Rewriter &rewriter;
};

class None_Match_Handler : public MatchFinder::MatchCallback
{
public:
    None_Match_Handler(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        const DeclRefExpr *exp = Result.Nodes.getNodeAs<DeclRefExpr>("NONE");
        if (exp != nullptr)
        {
            rewriter.ReplaceText(
                SourceRange(exp->getLocation(),exp->getEndLoc()),
                "gaia::rules::last_operation_t::row_none");
        }
    }

private:
    Rewriter &rewriter;
};

class ASTGenerator_Consumer : public clang::ASTConsumer 
{
public:
    explicit ASTGenerator_Consumer(ASTContext *context, Rewriter &r)
        : fieldGetMatcherHandler(r), fieldSetMatcherHandler(r), ruleMatcherHandler(r),
        rulesetMatcherHandler(r), fieldUnaryOperatorMatchHandler(r), 
        updateMatchHandler(r),insertMatchHandler(r),deleteMatchHandler(r),noneMatchHandler(r)

    {
        StatementMatcher fieldGetMatcher = 
            declRefExpr(to(varDecl(anyOf(hasAttr(attr::GaiaField),hasAttr(attr::GaiaFieldValue)),
            unless(hasAttr(attr::GaiaFieldLValue))))).bind("fieldGet");  
        StatementMatcher fieldSetMatcher = binaryOperator(allOf(
            isAssignmentOperator(),
            hasLHS(declRefExpr(to(varDecl(hasAttr(attr::GaiaFieldLValue))))))).bind("fieldSet");
        DeclarationMatcher ruleMatcher = functionDecl(hasAttr(attr::Rule)).bind("ruleDecl");
        DeclarationMatcher rulesetMatcher = rulesetDecl().bind("rulesetDecl");
        StatementMatcher fieldUnaryOperatorMatcher = unaryOperator(allOf(anyOf(
            hasOperatorName("++"), hasOperatorName("--")),
            hasUnaryOperand(declRefExpr(to(varDecl(hasAttr(attr::GaiaFieldLValue)))))
            )).bind("fieldUnaryOp");

        StatementMatcher tableFieldGetMatcher = 
            memberExpr(member(allOf(hasAttr(attr::GaiaField),unless(hasAttr(attr::GaiaFieldLValue)))),
            hasDescendant(declRefExpr(to(varDecl(anyOf(
                hasAttr(attr::GaiaField),
                hasAttr(attr::FieldTable),
                hasAttr(attr::GaiaFieldValue), 
                hasAttr(attr::GaiaLastOperation)))))))
            .bind("tableFieldGet");

        StatementMatcher tableFieldSetMatcher = binaryOperator(allOf(
            isAssignmentOperator(),
            hasLHS(memberExpr(member(hasAttr(attr::GaiaFieldLValue))))))
            .bind("fieldSet");

        StatementMatcher tableFieldUnaryOperatorMatcher = unaryOperator(allOf(anyOf(
            hasOperatorName("++"), hasOperatorName("--")),
            hasUnaryOperand(memberExpr(member(hasAttr(attr::GaiaFieldLValue)))))
            ).bind("fieldUnaryOp");

        StatementMatcher updateMatcher = 
            declRefExpr(to(varDecl(hasAttr(attr::GaiaLastOperationUPDATE)))).bind("UPDATE"); 
        StatementMatcher insertMatcher = 
            declRefExpr(to(varDecl(hasAttr(attr::GaiaLastOperationINSERT)))).bind("INSERT"); 
        StatementMatcher deleteMatcher = 
            declRefExpr(to(varDecl(hasAttr(attr::GaiaLastOperationDELETE)))).bind("DELETE"); 
        StatementMatcher noneMatcher = 
            declRefExpr(to(varDecl(hasAttr(attr::GaiaLastOperationNONE)))).bind("NONE"); 
        
        
        matcher.addMatcher(fieldGetMatcher, &fieldGetMatcherHandler);
        matcher.addMatcher(tableFieldGetMatcher, &fieldGetMatcherHandler);

        matcher.addMatcher(fieldSetMatcher, &fieldSetMatcherHandler);
        matcher.addMatcher(tableFieldSetMatcher, &fieldSetMatcherHandler);
             
        matcher.addMatcher(ruleMatcher, &ruleMatcherHandler);
        matcher.addMatcher(rulesetMatcher, &rulesetMatcherHandler);
        matcher.addMatcher(fieldUnaryOperatorMatcher, &fieldUnaryOperatorMatchHandler);
              
        matcher.addMatcher(tableFieldUnaryOperatorMatcher, &fieldUnaryOperatorMatchHandler);

        matcher.addMatcher(updateMatcher,&updateMatchHandler);
        matcher.addMatcher(insertMatcher,&insertMatchHandler);
        matcher.addMatcher(deleteMatcher,&deleteMatchHandler);
        matcher.addMatcher(noneMatcher,&noneMatchHandler);
    }

    virtual void HandleTranslationUnit(clang::ASTContext &context) 
    {
        matcher.matchAST(context);
    }
private:
    MatchFinder matcher;
    Field_Get_Match_Handler fieldGetMatcherHandler;
    Field_Set_Match_Handler fieldSetMatcherHandler;
    Rule_Match_Handler     ruleMatcherHandler;
    Ruleset_Match_Handler  rulesetMatcherHandler;
    Field_Unary_Operator_Match_Handler fieldUnaryOperatorMatchHandler;
    Update_Match_Handler updateMatchHandler;
    Insert_Match_Handler insertMatchHandler;
    Delete_Match_Handler deleteMatchHandler;
    None_Match_Handler noneMatchHandler;
    

};

class ASTGenerator_Action : public clang::ASTFrontendAction 
{
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &compiler, llvm::StringRef inFile) 
    {
        rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
        return std::unique_ptr<clang::ASTConsumer>(
            new ASTGenerator_Consumer(&compiler.getASTContext(), rewriter));
    }
    void EndSourceFileAction()
    {
        generateRules(rewriter);
        if (generationError)
        {
            return;
        }
 
        generated_Subscription_Code += "namespace " + curRuleset + "{\nvoid subscribeRuleset_" + 
                    curRuleset + "()\n{\n" + current_Ruleset_Subscription + 
                    "}\n" + "void unsubscribeRuleset_" + 
                    curRuleset + "()\n{\n" + current_Ruleset_UnSubscription + 
                    "}\n}\n" + generateGeneralSubscriptionCode();

        if (!shouldEraseOutputFiles() && !generationError && !ASTGeneratorOutputOption.empty())
        {
            std::error_code error_code;
            llvm::raw_fd_ostream outFile(ASTGeneratorOutputOption, error_code, llvm::sys::fs::F_None);

            if (!outFile.has_error())
            {
                rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID())
                    .write(outFile); 
                outFile << generated_Subscription_Code;
            }

            outFile.close();
        }
    }
private: 
    Rewriter rewriter;
};

int main(int argc, const char **argv) 
{
    field_Data = getTableData();
    // Parse the command-line args passed to your code.
    CommonOptionsParser op(argc, argv, ASTGeneratorCategory);
    if (ASTGeneratorVerboseOption)
    {
        g_verbose = true;
    }
    // Create a new Clang Tool instance (a LibTooling environment).
    ClangTool tool(op.getCompilations(), op.getSourcePathList());
    tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-fgaia-extensions"));
    tool.run(newFrontendActionFactory<ASTGenerator_Action>().get());    
}
