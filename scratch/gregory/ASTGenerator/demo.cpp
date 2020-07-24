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
#include "storage_engine.hpp"
#include "catalog_gaia_generated.h"

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
unordered_map<string, unordered_set<string>>  activeFields;
unordered_set<string>  usedTables;
const FunctionDecl *curRuleDecl = nullptr;
unordered_map<string, unordered_map<string, bool>> fieldData;
string curRulesetSubscription;
string generatedSubscriptionCode;
string curRulesetUnSubscription;
struct TableLinkData
{
    string table;
    string field;
};
unordered_multimap<string, TableLinkData> tableRelationship1;
unordered_multimap<string, TableLinkData> tableRelationshipN;

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
            "{\n" + ruleset + "::subscribeRuleset_" + ruleset + "();\nreturn;\n}\n";
    }

    retVal += "throw ruleset_not_found(ruleset_name);\n}\n" \
        "extern \"C\" void unsubscribe_ruleset(const char* ruleset_name)\n{\n";

    for (string ruleset : rulesets)
    {
        retVal += "if (strcmp(ruleset_name, \"" + ruleset + "\") == 0)\n" \
            "{\n" + ruleset + "::unsubscribeRuleset_" + ruleset + "();\nreturn;\n}\n";
    }

    retVal += "throw ruleset_not_found(ruleset_name);\n}\n" \
        "extern \"C\" void initialize_rules()\n{\n";
    for (string ruleset : rulesets)
    {
        retVal +=  ruleset + "::subscribeRuleset_" + ruleset + "();\n" ;
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

unordered_map<string, unordered_map<string, bool>> getTableData()
{
    unordered_map<string, unordered_map<string, bool>> retVal;
    try 
    {
        DBMonitor monitor;
    
        for(catalog::Gaia_table table = catalog::Gaia_table::get_first(); 
            table; table = table.get_next())
        {
            unordered_map<string, bool> fields;
            retVal[table.name()] = fields;
        }

        for(catalog::Gaia_field field = catalog::Gaia_field::get_first(); 
            field; field = field.get_next())
        {
            if (field.type() != gaia::catalog::gaia_data_type_REFERENCES)
            {
                gaia_id_t tableId = field.table_id();
                catalog::Gaia_table tbl = catalog::Gaia_table::get(tableId);
                if (!tbl)
                {
                    llvm::errs() << "Incorrect table for field " << field.name() << "\n";
                    generationError = true;
                    return unordered_map<string, unordered_map<string, bool>>();
                }
                unordered_map<string, bool> fields = retVal[tbl.name()];
                if (fields.find(field.name()) != fields.end())
                {
                    llvm::errs() << "Duplicate field " << field.name() << "\n";
                    generationError = true;
                    return unordered_map<string, unordered_map<string, bool>>();
                }
                fields[field.name()] = field.active();

                retVal[tbl.name()] = fields;
            }
            else
            {
                gaia_id_t parentTableId = field.table_id();
                catalog::Gaia_table parentTable = catalog::Gaia_table::get(parentTableId);
                if (!parentTable)
                {
                    llvm::errs() << "Incorrect table for field " << field.name() << "\n";
                    generationError = true;
                    return unordered_map<string, unordered_map<string, bool>>();
                }

                gaia_id_t childTableId = field.type_id();
                catalog::Gaia_table childTable = catalog::Gaia_table::get(childTableId);
                if (!childTable)
                {
                    llvm::errs() << "Incorrect table referenced by field " << field.name() << "\n";
                    generationError = true;
                    return unordered_map<string, unordered_map<string, bool>>();
                }
                TableLinkData linkData1;
                linkData1.table = childTable.name();
                linkData1.field = field.name();
                TableLinkData linkDataN;
                linkDataN.table = parentTable.name();
                linkDataN.field = field.name();

                tableRelationship1.emplace(parentTable.name(), linkData1);
                tableRelationshipN.emplace(childTable.name(), linkDataN);
            }
            
        }
    }
    catch (exception e)
    {
        llvm::errs() << "Exception while processing the catalog " << e.what() << "\n";
        generationError = true;
        return unordered_map<string, unordered_map<string, bool>>();
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

NavigationCodeData generateNavigationCode(string anchorTable)
{
    NavigationCodeData retVal;
    retVal.prefix = "\n" + anchorTable + "_t " + anchorTable + " = " + 
        anchorTable + "_t::get(context->record);\n";
    //single table used in the rule
    if (usedTables.size() == 1 && usedTables.find(anchorTable) != usedTables.end())
    {
        return retVal;
    }

    if (usedTables.empty())
    {
        generationError = true;
        llvm::errs() << "No tables are used in the rule \n";
        return NavigationCodeData();
    }
    if (usedTables.find(anchorTable) == usedTables.end())
    {
        generationError = true;
        llvm::errs() << "Table " << anchorTable <<" is not used in the rule \n";
        return NavigationCodeData();
    }

    if (tableRelationship1.find(anchorTable) == tableRelationship1.end() &&
        tableRelationshipN.find(anchorTable) == tableRelationshipN.end())
    {
        generationError = true;
        llvm::errs() << "Table " << anchorTable << " doesn't reference any table and not referenced by any other tables";
        return NavigationCodeData();
    }
    auto parentItr = tableRelationship1.equal_range(anchorTable);
    auto childItr = tableRelationshipN.equal_range(anchorTable);    
    for (string table : usedTables)
    {
        bool is1Relationship = false, isNRelationship = false;
        if (table == anchorTable)
        {
            continue;
        }
        string linkingField;
        for (auto it = parentItr.first; it != parentItr.second; ++it)
        {
            if (it != tableRelationship1.end() && it->second.table == table)
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
            if (it != tableRelationshipN.end() && it->second.table == table)
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

        if (!is1Relationship && !isNRelationship)
        {
            generationError = true;
            llvm::errs() << "No relationship between tables " << anchorTable << " and " << table << "\n";
            return NavigationCodeData();
        }

        if (is1Relationship && isNRelationship)
        {
            generationError = true;
            llvm::errs() << "Both relationships exist between tables " << anchorTable << " and " << table << "\n";
            return NavigationCodeData();
        }

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

    return retVal;
}

void generateRules(Rewriter &rewriter)
{
    if (curRuleDecl == nullptr)
    {
        return;
    }
    if (activeFields.empty())
    {
        llvm::errs() << "No active fields for the rule\n";
        generationError = true;
        return;
    }

    string ruleCode = rewriter.getRewrittenText(curRuleDecl->getSourceRange());
    int ruleCnt = 1;
    for (auto fd : activeFields)
    {
        string table = fd.first;
        bool containsLastOperation = false;
        bool containsFields = false;
        string fieldSubscriptionCode;
        if (fieldData.find(table) == fieldData.end())
        {
            llvm::errs() << "No table " << table << " found in the catalog\n";
            generationError = true;
            return;
        }
        string ruleName = curRuleset + "_" + curRuleDecl->getName().str() + "_" + to_string(ruleCnt);
        fieldSubscriptionCode =  "rule_binding_t " + ruleName + "binding(" +
            "\"" + curRuleset + "\",\"" + ruleName + "\"," + curRuleset + "::" + ruleName + ");\n" + 
            "field_list_t fields_" + to_string(ruleCnt) + ";\n";

        auto fields = fieldData[table];
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
           /* if (!fields[field])
            {
                llvm::errs() << "Field " << field << " is not marked as active in the catalog\n";
                generationError = true;
                return;
            }*/

            if (!isLastOperation)
            {
                fieldSubscriptionCode += "fields_" + to_string(ruleCnt) + ".insert(" + table + 
                "::get_field_offset(\"" + field + "\"));\n";
            }            
        }

        if (!containsFields && !containsLastOperation)
        {
            llvm::errs() << "No fields referred by table " + table +"\n";
            generationError = true;
            return;
        }

        if (containsFields)
        {
            curRulesetSubscription += fieldSubscriptionCode + "subscribe_rule(" + table + 
                "::s_gaia_type, event_type_t::row_update, fields_" + to_string(ruleCnt) + 
                "," + ruleName + "binding);\n";
            curRulesetUnSubscription += fieldSubscriptionCode + "unsubscribe_rule(" + table + 
                "::s_gaia_type, event_type_t::row_update, fields_" + to_string(ruleCnt) + 
                "," + ruleName + "binding);\n";
        }

        if (containsLastOperation)
        {
            curRulesetSubscription += "subscribe_rule(" + table + 
                "::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields," + 
                ruleName + "binding);\n";
            curRulesetSubscription += "subscribe_rule(" + table + 
                "::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields," + 
                ruleName + "binding);\n";
            curRulesetSubscription += "subscribe_rule(" + table + 
                "::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields," + 
                ruleName + "binding);\n";

            curRulesetUnSubscription += "unsubscribe_rule(" + table + 
                "::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields," + 
                ruleName + "binding);\n";
            curRulesetUnSubscription += "unsubscribe_rule(" + table + 
                "::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields," + 
                ruleName + "binding);\n";
            curRulesetUnSubscription += "unsubscribe_rule(" + table + 
                "::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields," + 
                ruleName + "binding);\n";
        }
        
        NavigationCodeData navigationCode = generateNavigationCode(table);
       
        if(ruleCnt == 1)
        {
            rewriter.InsertText(curRuleDecl->getLocation(),"\nvoid " + ruleName + "(const rule_context_t* context)\n");
            rewriter.InsertTextAfterToken(curRuleDecl->getLocation(), navigationCode.prefix);
            rewriter.InsertText(curRuleDecl->getEndLoc(),navigationCode.postfix);
        }
        else
        {
            rewriter.InsertTextBefore(curRuleDecl->getLocation(),"\nvoid " + ruleName + "(const rule_context_t* context)\n"
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
            usedTables.insert(tableName);

            
            if (decl->hasAttr<GaiaFieldAttr>())
            {
                expSourceRange = SourceRange(exp->getLocation(),exp->getEndLoc());
                activeFields[tableName].insert(fieldName);
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

                usedTables.insert(tableName);

                if (declExpr->getDecl()->hasAttr<GaiaFieldValueAttr>())
                {
                    expSourceRange = SourceRange(memberExpr->getBeginLoc().getLocWithOffset(-1), 
                        memberExpr->getEndLoc());
                }
                else
                {
                    expSourceRange = SourceRange(memberExpr->getBeginLoc(),
                        memberExpr->getEndLoc());
                    activeFields[tableName].insert(fieldName);
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
                rewriter.ReplaceText(expSourceRange, "context->last_operation(" + tableName + "::s_gaia_type)");
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
                    usedTables.insert(tableName);
                
                    tok::TokenKind tokenKind;
                    std::string replacementText = "[&]() mutable {" + 
                        tableName + "_writer w=" + tableName + "::writer(" +tableName + "); w->" + fieldName;

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
                        activeFields[tableName].insert(fieldName);
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
                        rewriter.InsertTextAfterToken(op->getEndLoc(),";" +
                            tableName + "::update_row(w);return " + 
                            tableName + "." + fieldName + "();}() ");

                    }
                    else
                    {
                        rewriter.InsertTextAfterToken(op->getEndLoc(),";" +
                            tableName + "::update_row(w);return " +  
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

                    usedTables.insert(tableName);
                    activeFields[tableName].insert(fieldName);
                                    
                    if (op->isPostfix())
                    {
                        if (op->isIncrementOp())
                        {
                            replaceStr = "[&]() mutable {auto t=" + 
                                tableName + "." + fieldName + "();" + 
                                tableName + "_writer w=" + tableName + "::writer(" + 
                                tableName + ");w->" + fieldName +"++;" +  
                                tableName + "::update_row(w);return t;}()";

                        }
                        else if(op->isDecrementOp())
                        {
                            replaceStr = "[&]() mutable {auto t=" + 
                                tableName + "." + fieldName + "();" + 
                                tableName + "_writer w=" + tableName + "::writer(" + 
                                tableName + ");w->" + fieldName + "--;" +  
                                tableName + "::update_row(w);return t;}()";
                        }
                    }
                    else
                    {
                        if (op->isIncrementOp())
                        {
                            replaceStr = "[&]() mutable {" + 
                                tableName + "_writer w=" + tableName + "::writer(" +  
                                tableName + ");++ w->" + fieldName +  ";" +
                                tableName + "::update_row(w); return w->" +
                                fieldName + ";}()";
                        }
                        else if(op->isDecrementOp())
                        {
                            replaceStr = "[&]() mutable {" + 
                                tableName + "_writer w = " + tableName + "::writer(" +  
                                tableName + ");-- w->" + fieldName +  ";" +
                                tableName + "::update_row(w); return w->" +
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
        curRuleDecl = ruleDecl;
        usedTables.clear();
        activeFields.clear();
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
        curRuleDecl = nullptr;
        usedTables.clear();
        activeFields.clear();

        const RulesetDecl * rulesetDecl = Result.Nodes.getNodeAs<RulesetDecl>("rulesetDecl");
        if (rulesetDecl != nullptr)
        {
            if (!curRuleset.empty())
            {
                generatedSubscriptionCode += "void subscribeRuleset_" + 
                    rulesetDecl->getName().str() + "()\n{\n" + curRulesetSubscription + 
                    "}\n";
                generatedSubscriptionCode += "void unsubscribeRuleset_" + 
                    rulesetDecl->getName().str() + "()\n{\n" + curRulesetUnSubscription + 
                    "}\n";
            }
            curRuleset = rulesetDecl->getName().str();   
            rulesets.push_back(curRuleset);
            curRulesetSubscription.clear(); 
            curRulesetUnSubscription.clear();       
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
                "last_operation_t::update");
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
                "last_operation_t::insert");
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
                "last_operation_t::delete");
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
                "last_operation_t::none");
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
 
        generatedSubscriptionCode += "void subscribeRuleset_" + 
                    curRuleset + "()\n{\n" + curRulesetSubscription + 
                    "}\n" + "void unsubscribeRuleset_" + 
                    curRuleset + "()\n{\n" + curRulesetUnSubscription + 
                    "}\n" + generateGeneralSubscriptionCode();

        if (!shouldEraseOutputFiles() && !generationError && !ASTGeneratorOutputOption.empty())
        {
            std::error_code error_code;
            llvm::raw_fd_ostream outFile(ASTGeneratorOutputOption, error_code, llvm::sys::fs::F_None);

            if (!outFile.has_error())
            {
                rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID())
                    .write(outFile); 
                outFile << generatedSubscriptionCode;
            }

            outFile.close();
        }
    }
private: 
    Rewriter rewriter;
};

int main(int argc, const char **argv) 
{
    fieldData = getTableData();
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
