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

cl::OptionCategory translation_engine_category("Use translation engine options");
cl::opt<string> translation_engine_output_option("output", cl::init(""),
    cl::desc("output file name"), cl::cat(translation_engine_category));
cl::opt<bool> translation_engine_verbose_option("v",
    cl::desc("print parse tokens"), cl::cat(translation_engine_category));

std::string current_ruleset;
bool g_verbose = false;
bool generation_error = false;

vector<string> rulesets;
unordered_map<string, unordered_set<string>> active_fields;
unordered_set<string> used_tables;

unordered_set<string> used_dbs;
unordered_map<string, string> table_db_data;

const FunctionDecl *current_rule_declaration = nullptr;
struct field_data_t
{
    bool is_active;
    int position;
};
unordered_map<string, unordered_map<string, field_data_t>> field_data;
string current_ruleset_subscription;
string generated_subscription_code;
string current_ruleset_unsubscription;
struct table_link_data_t
{
    string table;
    string field;
};
unordered_multimap<string, table_link_data_t> table_relationship_1;
unordered_multimap<string, table_link_data_t> table_relationship_n;

struct navigation_data_t
{
    string name;
    string linking_field;
    bool is_parent;
};

struct navigation_code_data_t
{
    string prefix;
    string postfix;
};

string generate_general_subscription_code()
{
    string return_value = "namespace gaia {\n namespace rules{\nextern \"C\" void subscribe_ruleset(const char* ruleset_name)\n{\n";

    for (string ruleset : rulesets)
    {
        return_value += "if (strcmp(ruleset_name, \"" + ruleset + "\") == 0)\n" \
            "{\n::" + ruleset + "::subscribeRuleset_" + ruleset + "();\nreturn;\n}\n";
    }

    return_value += "throw ruleset_not_found(ruleset_name);\n}\n" \
        "extern \"C\" void unsubscribe_ruleset(const char* ruleset_name)\n{\n";

    for (string ruleset : rulesets)
    {
        return_value += "if (strcmp(ruleset_name, \"" + ruleset + "\") == 0)\n" \
            "{\n::" + ruleset + "::unsubscribeRuleset_" + ruleset + "();\nreturn;\n}\n";
    }

    return_value += "throw ruleset_not_found(ruleset_name);\n}\n" \
        "extern \"C\" void initialize_rules()\n{\n";
    for (string ruleset : rulesets)
    {
        return_value += "::" + ruleset + "::subscribeRuleset_" + ruleset + "();\n" ;
    }
    return_value += "}\n}\n}";

    return return_value;
}

class db_monitor
{
    public:
        db_monitor()
        {
            gaia::db::begin_session();
            gaia::db::begin_transaction();
        }

        ~db_monitor()
        {
            gaia::db::commit_transaction();
            gaia::db::end_session();
        }
};

void fill_table_db_data(catalog::gaia_table_t &table)
{
    auto db = table.gaia_database();
    table_db_data[table.name()] = db.name();
}

unordered_map<string, unordered_map<string, field_data_t>> get_table_data()
{
    unordered_map<string, unordered_map<string, field_data_t>> return_value;
    try
    {
        db_monitor monitor;

        for (catalog::gaia_field_t field : catalog::gaia_field_t::list())
        {
            if (static_cast<gaia::catalog::data_type_t>(field.type()) != gaia::catalog::data_type_t::e_references)
            {
                catalog::gaia_table_t tbl = field.gaia_table();
                if (!tbl)
                {
                    llvm::errs() << "Incorrect table for field " << field.name() << "\n";
                    generation_error = true;
                    return unordered_map<string, unordered_map<string, field_data_t>>();
                }

                unordered_map<string, field_data_t> fields = return_value[tbl.name()];
                if (fields.find(field.name()) != fields.end())
                {
                    llvm::errs() << "Duplicate field " << field.name() << "\n";
                    generation_error = true;
                    return unordered_map<string, unordered_map<string, field_data_t>>();
                }
                field_data_t field_data;
                field_data.is_active = field.active();
                field_data.position = field.position();
                fields[field.name()] = field_data;
                return_value[tbl.name()] = fields;
                fill_table_db_data(tbl);
            }
            else
            {
                catalog::gaia_table_t parent_table = field.gaia_table();
                if (!parent_table)
                {
                    llvm::errs() << "Incorrect table for field " << field.name() << "\n";
                    generation_error = true;
                    return unordered_map<string, unordered_map<string, field_data_t>>();
                }

                catalog::gaia_table_t child_table = field.ref_gaia_table();
                if (!child_table)
                {
                    llvm::errs() << "Incorrect table referenced by field " << field.name() << "\n";
                    generation_error = true;
                    return unordered_map<string, unordered_map<string, field_data_t>>();
                }
                table_link_data_t link_data_1;
                link_data_1.table = child_table.name();
                link_data_1.field = field.name();
                table_link_data_t link_data_n;
                link_data_n.table = parent_table.name();
                link_data_n.field = field.name();

                table_relationship_1.emplace(parent_table.name(), link_data_1);
                table_relationship_n.emplace(child_table.name(), link_data_n);

                fill_table_db_data(parent_table);
                fill_table_db_data(child_table);
            }
        }
    }
    catch (exception e)
    {
        llvm::errs() << "Exception while processing the catalog " << e.what() << "\n";
        generation_error = true;
        return unordered_map<string, unordered_map<string, field_data_t>>();
    }
    return return_value;
}

string get_table_name(const Decl *decl)
{
    const FieldTableAttr *table_attr = decl->getAttr<FieldTableAttr>();
    if (table_attr != nullptr)
    {
        return table_attr->getTable()->getName().str();
    }
    return "";
}

string insert_rule_preamble(string rule, string preamble)
{
    size_t rule_code_start = rule.find('{');
    return "{" + preamble + rule.substr(rule_code_start + 1);
}

string get_closest_table(const unordered_map<string, int>& table_distance)
{
    int min_distance = INT_MAX;
    string return_value;
    for (auto d : table_distance)
    {
        if (d.second < min_distance)
        {
            min_distance = d.second;
            return_value = d.first;
        }
    }

    return return_value;
}

bool find_navigation_path(const string& src, const string& dst, vector<navigation_data_t> &current_path)
{
    if (src == dst)
    {
        return true;
    }

    unordered_map<string, int> table_distance;
    unordered_map<string, string> table_prev;
    unordered_map<string, navigation_data_t> table_navigation;

    for (auto fd : field_data)
    {
        table_distance[fd.first] = INT_MAX;
    }
    table_distance[src] = 0;

    string closest_table;

    while (closest_table != dst)
    {
        closest_table = get_closest_table(table_distance);
        if (closest_table == "" )
        {
            return false;
        }

        if (closest_table == dst)
        {
            break;
        }

        int distance = table_distance[closest_table];

        table_distance.erase(closest_table);

        auto parent_itr = table_relationship_1.equal_range(closest_table);
        for (auto it = parent_itr.first; it != parent_itr.second; ++it)
        {
            if (it != table_relationship_1.end())
            {
                string tbl = it->second.table;
                if (table_distance.find(tbl) != table_distance.end())
                {
                    if (table_distance[tbl] > distance + 1)
                    {
                        table_distance[tbl] = distance + 1;
                        table_prev[tbl] = closest_table;
                        navigation_data_t data = { tbl, it->second.field, true };
                        table_navigation[tbl] = data;
                    }
                }
            }
        }

        auto child_itr = table_relationship_n.equal_range(closest_table);
        for (auto it = child_itr.first; it != child_itr.second; ++it)
        {
            if (it != table_relationship_n.end())
            {
                string tbl = it->second.table;
                if (table_distance.find(tbl) != table_distance.end())
                {
                    if (table_distance[tbl] > distance + 1)
                    {
                        table_distance[tbl] = distance + 1;
                        table_prev[tbl] = closest_table;
                        navigation_data_t data = { tbl, it->second.field, false };
                        table_navigation[tbl] = data;
                    }
                }
            }
        }
    }

    string tbl = dst;
    while (table_prev[tbl] != "")
    {
        current_path.insert(current_path.begin(), table_navigation[tbl]);
        tbl = table_prev[tbl];
    }
    return true;
}

navigation_code_data_t generate_navigation_code(string anchor_table)
{
    navigation_code_data_t return_value;
    return_value.prefix = "\nauto " + anchor_table + " = " +
        "gaia::" + table_db_data[anchor_table] + "::" + anchor_table + "_t::get(context->record);\n";
    //single table used in the rule
    if (used_tables.size() == 1 && used_tables.find(anchor_table) != used_tables.end())
    {
        return return_value;
    }

    if (used_tables.empty())
    {
        generation_error = true;
        llvm::errs() << "No tables are used in the rule \n";
        return navigation_code_data_t();
    }
    if (used_tables.find(anchor_table) == used_tables.end())
    {
        generation_error = true;
        llvm::errs() << "Table " << anchor_table <<" is not used in the rule \n";
        return navigation_code_data_t();
    }

    if (table_relationship_1.find(anchor_table) == table_relationship_1.end() &&
        table_relationship_n.find(anchor_table) == table_relationship_n.end())
    {
        generation_error = true;
        llvm::errs() << "Table " << anchor_table << " doesn't reference any table and not referenced by any other tables";
        return navigation_code_data_t();
    }
    auto parent_itr = table_relationship_1.equal_range(anchor_table);
    auto child_itr = table_relationship_n.equal_range(anchor_table);
    unordered_set<string> processed_tables;
    for (string table : used_tables)
    {
        if (processed_tables.find(table) != processed_tables.end())
        {
            continue;
        }

        bool is_1_relationship = false, is_n_relationship = false;
        if (table == anchor_table)
        {
            continue;
        }

        string linking_field;
        for (auto it = parent_itr.first; it != parent_itr.second; ++it)
        {
            if (it != table_relationship_1.end() && it->second.table == table)
            {
                if (is_1_relationship)
                {
                    generation_error = true;
                    llvm::errs() << "More then one field that links " << anchor_table << " and " << table << "\n";
                    return navigation_code_data_t();
                }
                is_1_relationship = true;
                linking_field = it->second.field;
            }
        }

        for (auto it = child_itr.first; it != child_itr.second; ++it)
        {
            if (it != table_relationship_n.end() && it->second.table == table)
            {
                if (is_n_relationship)
                {
                    generation_error = true;
                    llvm::errs() << "More then one field that links " << anchor_table << " and " << table << "\n";
                    return navigation_code_data_t();
                }
                is_n_relationship = true;
                linking_field = it->second.field;
            }
        }

        if (is_1_relationship && is_n_relationship)
        {
            generation_error = true;
            llvm::errs() << "Both relationships exist between tables " << anchor_table << " and " << table << "\n";
            return navigation_code_data_t();
        }

        if (!is_1_relationship && !is_n_relationship)
        {
            vector<navigation_data_t> path;
            if (find_navigation_path(anchor_table, table, path))
            {
                string source_table = anchor_table;
                for (auto p : path)
                {
                    if (processed_tables.find(p.name) == processed_tables.end())
                    {
                        processed_tables.insert(p.name);
                        if (p.is_parent)
                        {
                            if (p.linking_field.empty())
                            {
                                return_value.prefix += "auto " + p.name + " = " + source_table + "." + p.name + "();\n";
                            }
                            else
                            {
                                return_value.prefix += "auto " + p.name + " = " + source_table + "." + p.linking_field + "_" + p.name + "();\n";
                            }
                        }
                        else
                        {
                            if (p.linking_field.empty())
                            {
                                return_value.prefix += "for (auto " + p.name + " : " + source_table + "." + p.name + "_list())\n{\n";
                            }
                            else
                            {
                                return_value.prefix += "for (auto " + p.name + " : " + source_table + "." + p.linking_field + "_" + p.name +  "_list())\n{\n";
                            }

                            return_value.postfix += "}\n";
                        }
                    }
                    source_table = p.name;
                }
            }
            else
            {
                generation_error = true;
                llvm::errs() << "No path between tables " << anchor_table << " and " << table << "\n";
                return navigation_code_data_t();
            }
        }
        else
        {
            if (is_1_relationship)
            {
                if (linking_field.empty())
                {
                    return_value.prefix += "auto " + table + " = " + anchor_table + "." + table + "();\n";
                }
                else
                {
                    return_value.prefix += "auto " + table + " = " + anchor_table + "." + linking_field + "_" + table + "();\n";
                }

            }
            else
            {
                if (linking_field.empty())
                {
                    return_value.prefix += "for (auto " + table + " : " + anchor_table + "." + table + "_list())\n{\n";
                }
                else
                {
                    return_value.prefix += "for (auto " + table + " : " + anchor_table + "." + linking_field + "_" + table + "_list())\n{\n";
                }

                return_value.postfix += "}\n";
            }
        }
        processed_tables.insert(table);
    }

    return return_value;
}

void generate_rules(Rewriter &rewriter)
{
    if (current_rule_declaration == nullptr)
    {
        return;
    }
    if (active_fields.empty())
    {
        llvm::errs() << "No active fields for the rule\n";
        generation_error = true;
        return;
    }

    string rule_code = rewriter.getRewrittenText(current_rule_declaration->getSourceRange());
    int rule_count = 1;
    for (auto fd : active_fields)
    {
        string table = fd.first;
        bool contains_last_operation = false;
        bool contains_fields = false;
        string field_subscription_code;
        string common_subscription_code;
        if (field_data.find(table) == field_data.end())
        {
            llvm::errs() << "No table " << table << " found in the catalog\n";
            generation_error = true;
            return;
        }
        string rule_name = current_ruleset + "_" + current_rule_declaration->getName().str() + "_" + to_string(rule_count);
        common_subscription_code = "rule_binding_t " + rule_name + "binding(" +
            "\"" + current_ruleset + "\",\"" + rule_name + "\"," + current_ruleset + "::" + rule_name + ");\n";
        field_subscription_code =  "field_position_list_t fields_" + rule_name + ";\n";

        if (fd.second.find("LastOperation") != fd.second.end())
        {
            contains_last_operation = true;
        }
        else
        {
            auto fields = field_data[table];
            contains_fields = !fd.second.empty();
            for (auto field : fd.second)
            {
                if (fields.find(field) == fields.end())
                {
                    llvm::errs() << "No field " << field << " found in the catalog\n";
                    generation_error = true;
                    return;
                }

                field_data_t field_data = fields[field];
                if (!field_data.is_active)
                {
                    llvm::errs() << "Field " << field << " is not marked as active in the catalog\n";
                    generation_error = true;
                    return;
                }

                field_subscription_code += "fields_" + rule_name + ".push_back(" + to_string(field_data.position) +");\n";
            }
        }

        if (!contains_fields && !contains_last_operation)
        {
            llvm::errs() << "No fields referred by table " + table +"\n";
            generation_error = true;
            return;
        }

        current_ruleset_subscription += common_subscription_code;
        current_ruleset_unsubscription += common_subscription_code;

        if (contains_fields)
        {
            current_ruleset_subscription += field_subscription_code + "subscribe_rule(gaia::" + table_db_data[table] + "::" + table +
             "_t::s_gaia_type, event_type_t::row_update, fields_" + rule_name +
                "," + rule_name + "binding);\n";
            current_ruleset_unsubscription += field_subscription_code + "unsubscribe_rule(gaia::" + table_db_data[table] + "::" + table +
             "_t::s_gaia_type, event_type_t::row_update, fields_" + rule_name +
                "," + rule_name + "binding);\n";
        }


        if (contains_last_operation)
        {
            current_ruleset_subscription += "subscribe_rule(gaia::" + table_db_data[table] + "::" + table +
             "_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields," +
                rule_name + "binding);\n";
            current_ruleset_subscription += "subscribe_rule(gaia::" + table_db_data[table] + "::" + table +
             "_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields," +
                rule_name + "binding);\n";
            current_ruleset_subscription += "subscribe_rule(gaia::" + table_db_data[table] + "::" + table +
             "_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields," +
                rule_name + "binding);\n";

            current_ruleset_unsubscription += "unsubscribe_rule(gaia::" + table_db_data[table] + "::" + table +
             "_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields," +
                rule_name + "binding);\n";
            current_ruleset_unsubscription += "unsubscribe_rule(gaia::" + table_db_data[table] + "::" + table +
             "_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields," +
                rule_name + "binding);\n";
            current_ruleset_unsubscription += "unsubscribe_rule(gaia::" + table_db_data[table] + "::" + table +
             "_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields," +
                rule_name + "binding);\n";
        }

        navigation_code_data_t navigation_code = generate_navigation_code(table);

        if(rule_count == 1)
        {
            rewriter.InsertText(current_rule_declaration->getLocation(), "\nvoid " + rule_name + "(const rule_context_t* context)\n");
            rewriter.InsertTextAfterToken(current_rule_declaration->getLocation(), navigation_code.prefix);
            rewriter.InsertText(current_rule_declaration->getEndLoc(), navigation_code.postfix);
        }
        else
        {
            rewriter.InsertTextBefore(current_rule_declaration->getLocation(), "\nvoid " + rule_name + "(const rule_context_t* context)\n"
               + insert_rule_preamble (rule_code + navigation_code.postfix, navigation_code.prefix));
        }

        rule_count++;
    }
}

class field_get_match_handler_t : public MatchFinder::MatchCallback
{
public:
    field_get_match_handler_t (Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        if (generation_error)
        {
            return;
        }
        const DeclRefExpr *expression = Result.Nodes.getNodeAs<DeclRefExpr>("fieldGet");
        const MemberExpr  *member_expression = Result.Nodes.getNodeAs<MemberExpr>("tableFieldGet");
        string table_name;
        string field_name;
        SourceRange expression_source_range;
        bool is_last_operation = false;
        if (expression != nullptr)
        {
            const ValueDecl *decl = expression->getDecl();
            if (decl->getType()->isStructureType())
            {
                return;
            }

            table_name = get_table_name(decl);
            field_name = decl->getName().str();
            used_tables.insert(table_name);
            used_dbs.insert(table_db_data[table_name]);

            if (decl->hasAttr<GaiaFieldAttr>())
            {
                expression_source_range = SourceRange(expression->getLocation(), expression->getEndLoc());
                active_fields[table_name].insert(field_name);
            }
            else if (decl->hasAttr<GaiaFieldValueAttr>())
            {
                expression_source_range = SourceRange(expression->getLocation().getLocWithOffset(-1), expression->getEndLoc());
            }
        }
        else if (member_expression != nullptr)
        {
            DeclRefExpr *declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
            if (declaration_expression != nullptr)
            {
                field_name = member_expression->getMemberNameInfo().getName().getAsString();
                table_name = declaration_expression->getDecl()->getName().str();

                is_last_operation = declaration_expression->getDecl()->hasAttr<GaiaLastOperationAttr>();

                used_tables.insert(table_name);
                used_dbs.insert(table_db_data[table_name]);

                if (declaration_expression->getDecl()->hasAttr<GaiaFieldValueAttr>())
                {
                    expression_source_range = SourceRange(member_expression->getBeginLoc().getLocWithOffset(-1),
                        member_expression->getEndLoc());
                }
                else
                {
                    expression_source_range = SourceRange(member_expression->getBeginLoc(),
                        member_expression->getEndLoc());
                    active_fields[table_name].insert(field_name);
                }
            }
            else
            {
                llvm::errs() << "Incorrect Base Type of generated type\n";
                generation_error = true;
            }
        }
        else
        {
            llvm::errs() << "Incorrect matched expression\n";
            generation_error = true;
        }

        if (expression_source_range.isValid())
        {
            if (is_last_operation)
            {
                rewriter.ReplaceText(expression_source_range, "context->last_operation(gaia::" + table_db_data[table_name] + "::" + table_name + "_t::s_gaia_type)");
            }
            else
            {
                rewriter.ReplaceText(expression_source_range, table_name + "." + field_name + "()");
            }
        }
    }

private:
    Rewriter &rewriter;
};

class field_set_match_handler_t: public MatchFinder::MatchCallback
{
public:
    field_set_match_handler_t (Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        if (generation_error)
        {
            return;
        }
        const BinaryOperator *op = Result.Nodes.getNodeAs<BinaryOperator>("fieldSet");
        if (op != nullptr)
        {
            const Expr *operator_expression = op->getLHS();
            if (operator_expression != nullptr)
            {
                const DeclRefExpr *left_declaration_expression = dyn_cast<DeclRefExpr>(operator_expression);
                const MemberExpr  *member_expression = dyn_cast<MemberExpr>(operator_expression);

                string table_name;
                string field_name;
                SourceLocation set_start_location;
                SourceLocation set_end_location;
                if (left_declaration_expression != nullptr || member_expression != nullptr)
                {
                    if (left_declaration_expression != nullptr)
                    {
                        const ValueDecl *operator_declaration = left_declaration_expression->getDecl();
                        if (operator_declaration->getType()->isStructureType())
                        {
                            return;
                        }
                        table_name = get_table_name(operator_declaration);
                        field_name = operator_declaration->getName().str();
                        set_start_location = left_declaration_expression->getLocation();
                    }
                    else
                    {
                        DeclRefExpr *declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
                        if (declaration_expression == nullptr)
                        {
                            llvm::errs() << "Incorrect Base Type of generated type\n";
                            generation_error = true;
                            return;
                        }
                        field_name = member_expression->getMemberNameInfo().getName().getAsString();
                        table_name = declaration_expression->getDecl()->getName().str();
                        set_start_location = member_expression->getBeginLoc();

                    }
                    used_tables.insert(table_name);
                    used_dbs.insert(table_db_data[table_name]);

                    tok::TokenKind token_kind;
                    std::string replacement_text = "[&]() mutable {auto w = " + table_name + ".writer(); w." +
                        field_name;

                    switch(op->getOpcode())
                    {
                        case BO_Assign:
                        {
                            token_kind = tok::equal;
                            break;
                        }
                        case BO_MulAssign:
                        {
                            token_kind = tok::starequal;
                            break;
                        }
                        case BO_DivAssign:
                        {
                            token_kind = tok::slashequal;
                            break;
                        }
                        case BO_RemAssign:
                        {
                            token_kind = tok::percentequal;
                            break;
                        }
                        case BO_AddAssign:
                        {
                            token_kind = tok::plusequal;
                            break;
                        }
                        case BO_SubAssign:
                        {
                            token_kind = tok::minusequal;
                            break;
                        }
                        case BO_ShlAssign:
                        {
                            token_kind = tok::lesslessequal;
                            break;
                        }
                        case BO_ShrAssign:
                        {
                            token_kind = tok::greatergreaterequal;
                            break;
                        }
                        case BO_AndAssign:
                        {
                            token_kind = tok::ampequal;
                            break;
                        }
                        case BO_XorAssign:
                        {
                            token_kind = tok::caretequal;
                            break;
                        }
                        case BO_OrAssign:
                        {
                            token_kind = tok::pipeequal;
                            break;
                        }
                        default:
                            llvm::errs() << "Incorrect Operator type\n";
                            generation_error = true;
                            return;
                    }

                    replacement_text += convert_compound_binary_opcode(op->getOpcode());

                    if (left_declaration_expression != nullptr)
                    {
                        set_end_location = Lexer::findLocationAfterToken(
                            set_start_location, token_kind, rewriter.getSourceMgr(),
                            rewriter.getLangOpts(), true);
                    }
                    else
                    {
                        set_end_location = Lexer::findLocationAfterToken(
                            member_expression->getExprLoc(), token_kind, rewriter.getSourceMgr(),
                            rewriter.getLangOpts(), true);
                    }

                    rewriter.ReplaceText(
                        SourceRange(set_start_location, set_end_location.getLocWithOffset(-1)),
                        replacement_text);

                    if (op->getOpcode() != BO_Assign)
                    {
                        rewriter.InsertTextAfterToken(op->getEndLoc(),
                            "; w.update_row();return w." + field_name + ";}() ");

                    }
                    else
                    {
                        rewriter.InsertTextAfterToken(op->getEndLoc(),
                            "; w.update_row();return w." + field_name + ";}()");
                    }
                }
                else
                {
                    llvm::errs() << "Incorrect Operator Expression Type\n";
                    generation_error = true;
                }
            }
            else
            {
                llvm::errs() << "Incorrect Operator Expression\n";
                generation_error = true;
            }
        }
        else
        {
            llvm::errs() << "Incorrect Matched operator\n";
            generation_error = true;
        }
    }

private:
    std::string convert_compound_binary_opcode(BinaryOperator::Opcode op_code)
    {
        switch(op_code)
        {
            case BO_Assign:
                return "=";
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
                llvm::errs() << "Incorrect Operator Code " << op_code <<"\n";
                generation_error = true;
                return "";
        }
    }

    Rewriter &rewriter;
};

class field_unary_operator_match_handler_t : public MatchFinder::MatchCallback
{
public:
    field_unary_operator_match_handler_t(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        if (generation_error)
        {
            return;
        }
        const UnaryOperator *op = Result.Nodes.getNodeAs<UnaryOperator>("fieldUnaryOp");
        if (op != nullptr)
        {
            const Expr *operator_expression = op->getSubExpr();
            if (operator_expression != nullptr)
            {
                const DeclRefExpr *declaration_expression = dyn_cast<DeclRefExpr>(operator_expression);
                const MemberExpr  *member_expression = dyn_cast<MemberExpr>(operator_expression);

                if (declaration_expression != nullptr || member_expression != nullptr)
                {
                    string replace_string;
                    string table_name;
                    string field_name;

                    if (declaration_expression != nullptr)
                    {

                        const ValueDecl *operator_declaration = declaration_expression->getDecl();
                        if (operator_declaration->getType()->isStructureType())
                        {
                            return;
                        }

                        table_name = get_table_name(operator_declaration);
                        field_name = operator_declaration->getName().str();
                    }
                    else
                    {
                        DeclRefExpr *declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
                        if (declaration_expression == nullptr)
                        {
                            llvm::errs() << "Incorrect Base Type of generated type\n";
                            generation_error = true;
                            return;
                        }
                        field_name = member_expression->getMemberNameInfo().getName().getAsString();
                        table_name = declaration_expression->getDecl()->getName().str();
                    }

                    used_tables.insert(table_name);
                    used_dbs.insert(table_db_data[table_name]);
                    active_fields[table_name].insert(field_name);

                    if (op->isPostfix())
                    {
                        if (op->isIncrementOp())
                        {
                            replace_string = "[&]() mutable {auto t=" +
                                table_name + "." + field_name + "();auto w = " + table_name + ".writer(); w." +
                                field_name + "++; w.update_row();return t;}()";

                        }
                        else if(op->isDecrementOp())
                        {
                            replace_string = "[&]() mutable {auto t=" +
                                table_name + "." + field_name + "();auto w = " + table_name + ".writer(); w." +
                                field_name + "--; w.update_row();return t;}()";
                        }
                    }
                    else
                    {
                        if (op->isIncrementOp())
                        {
                            replace_string = "[&]() mutable {auto w = " + table_name + ".writer(); ++ w." +
                                field_name + ";w.update_row(); return w." +
                                field_name + ";}()";
                        }
                        else if(op->isDecrementOp())
                        {
                            replace_string = "[&]() mutable {auto w = " + table_name + ".writer(); -- w." +
                                field_name + ";w.update_row(); return w." +
                                field_name + ";}()";
                        }
                    }

                    rewriter.ReplaceText(
                        SourceRange(op->getBeginLoc().getLocWithOffset(-1), op->getEndLoc().getLocWithOffset(1)),
                        replace_string);
                }
                else
                {
                    llvm::errs() << "Incorrect Operator Expression Type\n";
                    generation_error = true;
                }
            }
            else
            {
                llvm::errs() << "Incorrect Operator Expression\n";
                generation_error = true;
            }
        }
        else
        {
            llvm::errs() << "Incorrect Matched Operator\n";
            generation_error = true;
        }
    }

private:
    Rewriter &rewriter;
};

class rule_match_handler_t : public MatchFinder::MatchCallback
{
public:
    rule_match_handler_t(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        if (generation_error)
        {
            return;
        }
        const FunctionDecl * rule_declaration = Result.Nodes.getNodeAs<FunctionDecl>("ruleDecl");
        generate_rules(rewriter);
        if (generation_error)
        {
            return;
        }
        current_rule_declaration = rule_declaration;
        used_tables.clear();
        active_fields.clear();
    }

private:
    Rewriter &rewriter;
};

class ruleset_match_handler_t : public MatchFinder::MatchCallback
{
public:
    ruleset_match_handler_t(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        if (generation_error)
        {
            return;
        }
        generate_rules(rewriter);
        if (generation_error)
        {
            return;
        }
        current_rule_declaration = nullptr;
        used_tables.clear();
        active_fields.clear();

        const RulesetDecl * ruleset_declaration = Result.Nodes.getNodeAs<RulesetDecl>("rulesetDecl");
        if (ruleset_declaration != nullptr)
        {
            if (!current_ruleset.empty())
            {
                generated_subscription_code += "namespace " + current_ruleset +
                    "{\nvoid subscribeRuleset_" +
                    ruleset_declaration->getName().str() + "()\n{\n" + current_ruleset_subscription +
                    "}\nvoid unsubscribeRuleset_" +
                    ruleset_declaration->getName().str() + "()\n{\n" + current_ruleset_unsubscription +
                    "}\n}\n";
            }
            current_ruleset = ruleset_declaration->getName().str();
            rulesets.push_back(current_ruleset);
            current_ruleset_subscription.clear();
            current_ruleset_unsubscription.clear();
            rewriter.ReplaceText(
                SourceRange(ruleset_declaration->getBeginLoc(), ruleset_declaration->decls_begin()->getBeginLoc().getLocWithOffset(-2)),
                "namespace " + current_ruleset + "\n{\n");
        }
    }

private:
    Rewriter &rewriter;
};

class update_match_handler_t : public MatchFinder::MatchCallback
{
public:
    update_match_handler_t(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        const DeclRefExpr *exp = Result.Nodes.getNodeAs<DeclRefExpr>("UPDATE");
        if (exp != nullptr)
        {
            rewriter.ReplaceText(
                SourceRange(exp->getLocation(), exp->getEndLoc()),
                "gaia::rules::last_operation_t::row_update");
        }
    }

private:
    Rewriter &rewriter;
};

class insert_match_handler_t : public MatchFinder::MatchCallback
{
public:
    insert_match_handler_t(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        const DeclRefExpr *exp = Result.Nodes.getNodeAs<DeclRefExpr>("INSERT");
        if (exp != nullptr)
        {
            rewriter.ReplaceText(
                SourceRange(exp->getLocation(), exp->getEndLoc()),
                "gaia::rules::last_operation_t::row_insert");
        }
    }

private:
    Rewriter &rewriter;
};

class delete_match_handler_t : public MatchFinder::MatchCallback
{
public:
    delete_match_handler_t(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        const DeclRefExpr *exp = Result.Nodes.getNodeAs<DeclRefExpr>("DELETE");
        if (exp != nullptr)
        {
            rewriter.ReplaceText(
                SourceRange(exp->getLocation(), exp->getEndLoc()),
                "gaia::rules::last_operation_t::row_delete");
        }
    }

private:
    Rewriter &rewriter;
};

class none_match_handler_t : public MatchFinder::MatchCallback
{
public:
    none_match_handler_t(Rewriter &r) : rewriter (r){}
    virtual void run (const MatchFinder::MatchResult &Result)
    {
        const DeclRefExpr *exp = Result.Nodes.getNodeAs<DeclRefExpr>("NONE");
        if (exp != nullptr)
        {
            rewriter.ReplaceText(
                SourceRange(exp->getLocation(), exp->getEndLoc()),
                "gaia::rules::last_operation_t::row_none");
        }
    }

private:
    Rewriter &rewriter;
};

class translation_engine_consumer_t : public clang::ASTConsumer
{
public:
    explicit translation_engine_consumer_t(ASTContext *context, Rewriter &r)
        : field_get_match_handler(r), field_set_match_handler(r), rule_match_handler(r),
        ruleset_match_handler(r), field_unary_operator_match_handler(r),
        update_match_handler(r), insert_match_handler(r), delete_match_handler(r), none_match_handler(r)

    {
        StatementMatcher field_get_matcher =
            declRefExpr(to(varDecl(anyOf(hasAttr(attr::GaiaField), hasAttr(attr::GaiaFieldValue)),
            unless(hasAttr(attr::GaiaFieldLValue))))).bind("fieldGet");
        StatementMatcher field_set_matcher = binaryOperator(allOf(
            isAssignmentOperator(),
            hasLHS(declRefExpr(to(varDecl(hasAttr(attr::GaiaFieldLValue))))))).bind("fieldSet");
        DeclarationMatcher rule_matcher = functionDecl(hasAttr(attr::Rule)).bind("ruleDecl");
        DeclarationMatcher ruleset_matcher = rulesetDecl().bind("rulesetDecl");
        StatementMatcher field_unary_operator_matcher = unaryOperator(allOf(anyOf(
            hasOperatorName("++"), hasOperatorName("--")),
            hasUnaryOperand(declRefExpr(to(varDecl(hasAttr(attr::GaiaFieldLValue)))))
            )).bind("fieldUnaryOp");
        StatementMatcher table_field_get_matcher =
            memberExpr(member(allOf(hasAttr(attr::GaiaField), unless(hasAttr(attr::GaiaFieldLValue)))),
            hasDescendant(declRefExpr(to(varDecl(anyOf(
                hasAttr(attr::GaiaField),
                hasAttr(attr::FieldTable),
                hasAttr(attr::GaiaFieldValue),
                hasAttr(attr::GaiaLastOperation)))))))
            .bind("tableFieldGet");
        StatementMatcher table_field_set_matcher = binaryOperator(allOf(
            isAssignmentOperator(),
            hasLHS(memberExpr(member(hasAttr(attr::GaiaFieldLValue))))))
            .bind("fieldSet");
        StatementMatcher table_field_unary_operator_matcher = unaryOperator(allOf(anyOf(
            hasOperatorName("++"), hasOperatorName("--")),
            hasUnaryOperand(memberExpr(member(hasAttr(attr::GaiaFieldLValue)))))
            ).bind("fieldUnaryOp");
        StatementMatcher update_matcher =
            declRefExpr(to(varDecl(hasAttr(attr::GaiaLastOperationUPDATE)))).bind("UPDATE");
        StatementMatcher insert_matcher =
            declRefExpr(to(varDecl(hasAttr(attr::GaiaLastOperationINSERT)))).bind("INSERT");
        StatementMatcher delete_matcher =
            declRefExpr(to(varDecl(hasAttr(attr::GaiaLastOperationDELETE)))).bind("DELETE");
        StatementMatcher none_matcher =
            declRefExpr(to(varDecl(hasAttr(attr::GaiaLastOperationNONE)))).bind("NONE");

        matcher.addMatcher(field_get_matcher, &field_get_match_handler);
        matcher.addMatcher(table_field_get_matcher, &field_get_match_handler);

        matcher.addMatcher(field_set_matcher, &field_set_match_handler);
        matcher.addMatcher(table_field_set_matcher, &field_set_match_handler);

        matcher.addMatcher(rule_matcher, &rule_match_handler);
        matcher.addMatcher(ruleset_matcher, &ruleset_match_handler);
        matcher.addMatcher(field_unary_operator_matcher, &field_unary_operator_match_handler);

        matcher.addMatcher(table_field_unary_operator_matcher, &field_unary_operator_match_handler);

        matcher.addMatcher(update_matcher, &update_match_handler);
        matcher.addMatcher(insert_matcher, &insert_match_handler);
        matcher.addMatcher(delete_matcher, &delete_match_handler);
        matcher.addMatcher(none_matcher, &none_match_handler);
    }

    virtual void HandleTranslationUnit(clang::ASTContext &context)
    {
        matcher.matchAST(context);
    }
private:
    MatchFinder matcher;
    field_get_match_handler_t field_get_match_handler;
    field_set_match_handler_t field_set_match_handler;
    rule_match_handler_t     rule_match_handler;
    ruleset_match_handler_t  ruleset_match_handler;
    field_unary_operator_match_handler_t field_unary_operator_match_handler;
    update_match_handler_t update_match_handler;
    insert_match_handler_t insert_match_handler;
    delete_match_handler_t delete_match_handler;
    none_match_handler_t none_match_handler;
};

class translation_engine_action_t : public clang::ASTFrontendAction
{
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &compiler, llvm::StringRef inFile)
    {
        rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
        return std::unique_ptr<clang::ASTConsumer>(
            new translation_engine_consumer_t(&compiler.getASTContext(), rewriter));
    }
    void EndSourceFileAction()
    {
        generate_rules(rewriter);
        if (generation_error)
        {
            return;
        }

        generated_subscription_code += "namespace " + current_ruleset + "{\nvoid subscribeRuleset_" +
                    current_ruleset + "()\n{\n" + current_ruleset_subscription +
                    "}\n" + "void unsubscribeRuleset_" +
                    current_ruleset + "()\n{\n" + current_ruleset_unsubscription +
                    "}\n}\n" + generate_general_subscription_code();

        if (!shouldEraseOutputFiles() && !generation_error && !translation_engine_output_option.empty())
        {
            std::error_code error_code;
            llvm::raw_fd_ostream output_file(translation_engine_output_option, error_code, llvm::sys::fs::F_None);

            if (!output_file.has_error())
            {
                for (string db : used_dbs)
                {
                    output_file << "#include \"gaia_" << db << ".h\"\n";
                }

                output_file << "#include \"rules.hpp\"\n";
                output_file << "using namespace gaia::rules;\n";

                rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID())
                    .write(output_file);
                output_file << generated_subscription_code;
            }

            output_file.close();
        }
    }
private:
    Rewriter rewriter;
};

int main(int argc, const char **argv)
{
    field_data = get_table_data();
    // Parse the command-line args passed to your code.
    CommonOptionsParser op(argc, argv, translation_engine_category);
    if (translation_engine_verbose_option)
    {
        g_verbose = true;
    }
    // Create a new Clang Tool instance (a LibTooling environment).
    ClangTool tool(op.getCompilations(), op.getSourcePathList());
    tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-fgaia-extensions"));
    tool.run(newFrontendActionFactory<translation_engine_action_t>().get());
}
