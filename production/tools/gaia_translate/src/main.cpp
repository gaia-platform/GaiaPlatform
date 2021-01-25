/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "gaia_catalog.h"
#include "gaia_version.hpp"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;
using namespace clang::ast_matchers;
using namespace gaia;

cl::OptionCategory g_translation_engine_category("Use translation engine options");
cl::opt<string> g_translation_engine_output_option(
    "output", cl::init(""), cl::desc("output file name"), cl::cat(g_translation_engine_category));

std::string g_current_ruleset;
bool g_generation_error = false;
int g_current_ruleset_rule_number = 1;
unsigned int g_current_ruleset_rule_line_number = 1;
bool g_delete_operation_in_rule = false;
const int c_declaration_to_ruleset_offset = -2;
bool g_function_call_in_rule = false;
vector<SourceRange> g_gaia_id_call_locations;

vector<string> g_rulesets;
unordered_map<string, unordered_set<string>> g_active_fields;
unordered_set<string> g_used_tables;

unordered_set<string> g_used_dbs;
unordered_map<string, string> g_table_db_data;

const FunctionDecl* g_current_rule_declaration = nullptr;
struct field_data_t
{
    bool is_active;
    int position;
};
unordered_map<string, unordered_map<string, field_data_t>> g_field_data;
string g_current_ruleset_subscription;
string g_generated_subscription_code;
string g_current_ruleset_unsubscription;
struct table_link_data_t
{
    string table;
    string field;
};
unordered_multimap<string, table_link_data_t> g_table_relationship_1;
unordered_multimap<string, table_link_data_t> g_table_relationship_n;

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

// Suppress these clang-tidy warnings for now.
const char c_nolint_identifier_naming[] = "// NOLINTNEXTLINE(readability-identifier-naming)";
const char c_nolint_range_copy[] = "// NOLINTNEXTLINE(performance-for-range-copy)";

const char* c_last_operation = "LastOperation";

static void print_version(raw_ostream& stream)
{
    stream << "Gaia Translation Engine " << gaia_full_version() << "\nCopyright (c) Gaia Platform LLC\n";
}

string generate_general_subscription_code()
{
    string return_value;
    return_value
        .append("namespace gaia\n")
        .append("{\n")
        .append("namespace rules\n")
        .append("{\n")
        .append("extern \"C\" void subscribe_ruleset(const char* ruleset_name)\n")
        .append("{\n");

    for (const string& ruleset : g_rulesets)
    {
        return_value
            .append("    if (strcmp(ruleset_name, \"" + ruleset + "\") == 0)\n")
            .append("    {\n")
            .append("        ::" + ruleset + "::subscribe_ruleset_" + ruleset + "();\n") //NOLINT(performance-inefficient-string-concatenation)
            .append("        return;\n")
            .append("    }\n");
    }

    return_value
        .append("    throw ruleset_not_found(ruleset_name);\n")
        .append("}\n")
        .append("extern \"C\" void unsubscribe_ruleset(const char* ruleset_name)\n")
        .append("{\n");

    for (const string& ruleset : g_rulesets)
    {
        return_value
            .append("    if (strcmp(ruleset_name, \"" + ruleset + "\") == 0)\n")
            .append("    {\n")
            .append("        ::" + ruleset + "::unsubscribe_ruleset_" + ruleset + "();\n") //NOLINT(performance-inefficient-string-concatenation)
            .append("        return;\n")
            .append("    }\n");
    }

    return_value
        .append("    throw ruleset_not_found(ruleset_name);\n")
        .append("}\n")
        .append("extern \"C\" void initialize_rules()\n")
        .append("{\n");

    for (const string& ruleset : g_rulesets)
    {
        return_value
            .append("    ::" + ruleset)
            .append("::subscribe_ruleset_" + ruleset + "();\n");
    }
    return_value
        .append("}\n")
        .append("} // namespace rules\n")
        .append("} // namespace gaia\n");

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

void fill_table_db_data(catalog::gaia_table_t& table)
{
    auto db = table.gaia_database();
    g_table_db_data[table.name()] = db.name();
}

unordered_map<string, unordered_map<string, field_data_t>> get_table_data()
{
    unordered_map<string, unordered_map<string, field_data_t>> return_value;
    if (g_generation_error)
    {
        return return_value;
    }
    try
    {
        db_monitor monitor;

        for (catalog::gaia_field_t field : catalog::gaia_field_t::list())
        {
            catalog::gaia_table_t tbl = field.gaia_table();
            if (!tbl)
            {
                cerr << "Incorrect table for field '" << field.name() << "'." << endl;
                g_generation_error = true;
                return unordered_map<string, unordered_map<string, field_data_t>>();
            }

            unordered_map<string, field_data_t> fields = return_value[tbl.name()];
            if (fields.find(field.name()) != fields.end())
            {
                cerr << "Duplicate field '" << field.name() << "'." << endl;
                g_generation_error = true;
                return unordered_map<string, unordered_map<string, field_data_t>>();
            }
            field_data_t field_data;
            field_data.is_active = field.active();
            field_data.position = field.position();
            fields[field.name()] = field_data;
            return_value[tbl.name()] = fields;
            fill_table_db_data(tbl);
        }

        for (catalog::gaia_relationship_t relationship : catalog::gaia_relationship_t::list())
        {
            catalog::gaia_table_t child_table = relationship.child_gaia_table();
            if (!child_table)
            {
                cerr << "Incorrect child table in the relationship '" << relationship.name() << "'." << endl;
                g_generation_error = true;
                return unordered_map<string, unordered_map<string, field_data_t>>();
            }

            catalog::gaia_table_t parent_table = relationship.parent_gaia_table();
            if (!parent_table)
            {
                cerr << "Incorrect parent table in the relationship " << relationship.name() << "." << endl;
                g_generation_error = true;
                return unordered_map<string, unordered_map<string, field_data_t>>();
            }
            table_link_data_t link_data_1;
            link_data_1.table = parent_table.name();
            link_data_1.field = relationship.name();
            table_link_data_t link_data_n;
            link_data_n.table = child_table.name();
            link_data_n.field = relationship.name();

            g_table_relationship_1.emplace(child_table.name(), link_data_1);
            g_table_relationship_n.emplace(parent_table.name(), link_data_n);

            fill_table_db_data(child_table);
            fill_table_db_data(parent_table);
        }
    }
    catch (const exception& e)
    {
        cerr << "An exception has occurred while processing the catalog: '" << e.what() << "'." << endl;
        g_generation_error = true;
        return unordered_map<string, unordered_map<string, field_data_t>>();
    }
    return return_value;
}

string get_table_name(const Decl* decl)
{
    const FieldTableAttr* table_attr = decl->getAttr<FieldTableAttr>();
    if (table_attr != nullptr)
    {
        return table_attr->getTable()->getName().str();
    }
    return "";
}

// This function adds a field to active fields list if it is marked as active in the catalog;
// it returns true if there was no error and false otherwise.
bool validate_and_add_active_field(const string& table_name, const string& field_name)
{
    if (g_field_data.empty())
    {
        g_field_data = get_table_data();
    }

    if (g_generation_error)
    {
        return false;
    }

    if (g_field_data.find(table_name) == g_field_data.end())
    {
        cerr << "Table '" << table_name << "' was not found in the catalog." << endl;
        g_generation_error = true;
        return false;
    }

    if (field_name == c_last_operation)
    {
        g_active_fields[table_name].insert(c_last_operation);
        return true;
    }

    auto fields = g_field_data[table_name];

    if (fields.find(field_name) == fields.end())
    {
        cerr << "Field '" << field_name << "' of table '" << table_name << "' was not found in the catalog." << endl;
        g_generation_error = true;
        return false;
    }

    field_data_t field_data = fields[field_name];
    if (field_data.is_active)
    {
        g_active_fields[table_name].insert(field_name);
    }
    return true;
}

string insert_rule_preamble(const string& rule, const string& preamble)
{
    size_t rule_code_start = rule.find('{');
    return "{" + preamble + rule.substr(rule_code_start + 1);
}

string get_closest_table(const unordered_map<string, int>& table_distance)
{
    int min_distance = INT_MAX;
    string return_value;
    for (const auto& d : table_distance)
    {
        if (d.second < min_distance)
        {
            min_distance = d.second;
            return_value = d.first;
        }
    }

    return return_value;
}

bool find_navigation_path(const string& src, const string& dst, vector<navigation_data_t>& current_path)
{
    if (src == dst)
    {
        return true;
    }

    unordered_map<string, int> table_distance;
    unordered_map<string, string> table_prev;
    unordered_map<string, navigation_data_t> table_navigation;

    for (const auto& fd : g_field_data)
    {
        table_distance[fd.first] = INT_MAX;
    }
    table_distance[src] = 0;

    string closest_table;

    while (closest_table != dst)
    {
        closest_table = get_closest_table(table_distance);
        if (closest_table == "")
        {
            return false;
        }

        if (closest_table == dst)
        {
            break;
        }

        int distance = table_distance[closest_table];

        table_distance.erase(closest_table);

        auto parent_itr = g_table_relationship_1.equal_range(closest_table);
        for (auto it = parent_itr.first; it != parent_itr.second; ++it)
        {
            if (it != g_table_relationship_1.end())
            {
                string tbl = it->second.table;
                if (table_distance.find(tbl) != table_distance.end())
                {
                    if (table_distance[tbl] > distance + 1)
                    {
                        table_distance[tbl] = distance + 1;
                        table_prev[tbl] = closest_table;
                        navigation_data_t data = {tbl, it->second.field, true};
                        table_navigation[tbl] = data;
                    }
                }
            }
        }

        auto child_itr = g_table_relationship_n.equal_range(closest_table);
        for (auto it = child_itr.first; it != child_itr.second; ++it)
        {
            if (it != g_table_relationship_n.end())
            {
                string tbl = it->second.table;
                if (table_distance.find(tbl) != table_distance.end())
                {
                    if (table_distance[tbl] > distance + 1)
                    {
                        table_distance[tbl] = distance + 1;
                        table_prev[tbl] = closest_table;
                        navigation_data_t data = {tbl, it->second.field, false};
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

navigation_code_data_t generate_navigation_code(const string& anchor_table)
{
    navigation_code_data_t return_value;
    // no need to generate navigation code for a rule with LastOperation DELETE
    if (!g_delete_operation_in_rule)
    {
        return_value.prefix = "\nauto " + anchor_table
            + " = " + "gaia::" + g_table_db_data[anchor_table] + "::" + anchor_table + "_t::get(context->record);\n";
    }

    //single table used in the rule
    if (g_used_tables.size() == 1 && g_used_tables.find(anchor_table) != g_used_tables.end())
    {
        return return_value;
    }

    if (g_delete_operation_in_rule)
    {
        g_generation_error = true;
        cerr
            << "Navigation from a record that has been deleted is currently not supported."
            << " This condition occurs when a rule is subscribed to a delete operation"
            << " and is referencing data related to the deleted record." << endl;
        return navigation_code_data_t();
    }

    if (g_used_tables.empty())
    {
        g_generation_error = true;
        cerr << "No tables are used in the rule." << endl;
        return navigation_code_data_t();
    }
    if (g_used_tables.find(anchor_table) == g_used_tables.end())
    {
        g_generation_error = true;
        cerr << "Table '" << anchor_table << "' is not used in the rule." << endl;
        return navigation_code_data_t();
    }

    if (g_table_relationship_1.find(anchor_table) == g_table_relationship_1.end()
        && g_table_relationship_n.find(anchor_table) == g_table_relationship_n.end())
    {
        g_generation_error = true;
        cerr << "No path between '" << anchor_table << "' and other tables." << endl;
        return navigation_code_data_t();
    }
    auto parent_itr = g_table_relationship_1.equal_range(anchor_table);
    auto child_itr = g_table_relationship_n.equal_range(anchor_table);
    unordered_set<string> processed_tables;
    for (const string& table : g_used_tables)
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
            if (it != g_table_relationship_1.end() && it->second.table == table)
            {
                if (is_1_relationship)
                {
                    g_generation_error = true;
                    cerr << "There is more than one field that links '" << anchor_table << "' and '" << table << "'." << endl;
                    return navigation_code_data_t();
                }
                is_1_relationship = true;
                linking_field = it->second.field;
            }
        }

        for (auto it = child_itr.first; it != child_itr.second; ++it)
        {
            if (it != g_table_relationship_n.end() && it->second.table == table)
            {
                if (is_n_relationship)
                {
                    g_generation_error = true;
                    cerr << "There is more than one field that links '" << anchor_table << "' and '" << table << "'." << endl;
                    return navigation_code_data_t();
                }
                is_n_relationship = true;
                linking_field = it->second.field;
            }
        }

        if (is_1_relationship && is_n_relationship)
        {
            g_generation_error = true;
            cerr << "Both relationships exist between tables '" << anchor_table << "' and '" << table << "'." << endl;
            return navigation_code_data_t();
        }

        if (!is_1_relationship && !is_n_relationship)
        {
            vector<navigation_data_t> path;
            if (find_navigation_path(anchor_table, table, path))
            {
                string source_table = anchor_table;
                for (const auto& p : path)
                {
                    if (processed_tables.find(p.name) == processed_tables.end())
                    {
                        processed_tables.insert(p.name);
                        if (p.is_parent)
                        {
                            if (p.linking_field.empty())
                            {
                                return_value.prefix
                                    += "auto " + p.name + " = "
                                    + source_table + "." + p.name + "();\n";
                            }
                            else
                            {
                                return_value.prefix
                                    += "auto " + p.name + " = "
                                    + source_table + "." + p.linking_field + "_" + p.name + "();\n";
                            }
                        }
                        else
                        {
                            if (p.linking_field.empty())
                            {
                                return_value.prefix
                                    .append(c_nolint_range_copy)
                                    .append("\nfor (auto ")
                                    .append(p.name)
                                    .append(" : ")
                                    .append(source_table)
                                    .append(".")
                                    .append(p.name)
                                    .append("_list())\n{\n");
                            }
                            else
                            {
                                return_value.prefix
                                    .append(c_nolint_range_copy)
                                    .append("\nfor (auto ")
                                    .append(p.name)
                                    .append(" : ")
                                    .append(source_table)
                                    .append(".")
                                    .append(p.linking_field)
                                    .append("_")
                                    .append(p.name)
                                    .append("_list())\n{\n");
                            }

                            return_value.postfix += "}\n";
                        }
                    }
                    source_table = p.name;
                }
            }
            else
            {
                g_generation_error = true;
                cerr << "No path between tables '" << anchor_table << "' and '" << table << "'." << endl;
                return navigation_code_data_t();
            }
        }
        else
        {
            if (is_1_relationship)
            {
                if (linking_field.empty())
                {
                    return_value.prefix
                        .append("auto ")
                        .append(table)
                        .append(" = ")
                        .append(anchor_table)
                        .append(".")
                        .append(table)
                        .append("();\n");
                }
                else
                {
                    return_value.prefix
                        .append("auto ")
                        .append(table)
                        .append(" = ")
                        .append(anchor_table)
                        .append(".")
                        .append(linking_field)
                        .append("_")
                        .append(table)
                        .append("();\n");
                }
            }
            else
            {
                if (linking_field.empty())
                {
                    return_value.prefix
                        .append(c_nolint_range_copy)
                        .append("\nfor (auto ")
                        .append(table)
                        .append(" : ")
                        .append(anchor_table)
                        .append(".")
                        .append(table)
                        .append("_list())\n{\n");
                }
                else
                {
                    return_value.prefix
                        .append(c_nolint_range_copy)
                        .append("\nfor (auto ")
                        .append(table)
                        .append(" : ")
                        .append(anchor_table)
                        .append(".")
                        .append(linking_field)
                        .append("_")
                        .append(table)
                        .append("_list())\n{\n");
                }

                return_value.postfix += "}\n";
            }
        }
        processed_tables.insert(table);
    }

    return return_value;
}

void generate_rules(Rewriter& rewriter)
{
    if (g_field_data.empty())
    {
        g_field_data = get_table_data();
    }

    if (g_generation_error)
    {
        return;
    }

    if (g_current_rule_declaration == nullptr)
    {
        return;
    }
    if (g_active_fields.empty())
    {
        cerr << "No active fields for the rule." << endl;
        g_generation_error = true;
        return;
    }

    if (g_delete_operation_in_rule)
    {
        if (g_function_call_in_rule)
        {
            cerr
                << "Calling extended data class methods of a record that has been deleted is currently not supported."
                << " This condition occurs when a rule is subscribed to a delete operation"
                << " and is referencing data related to the deleted record." << endl;
            g_generation_error = true;
            return;
        }

        for (const auto& location : g_gaia_id_call_locations)
        {
            rewriter.ReplaceText(location, "context->record");
        }
    }

    string rule_code = rewriter.getRewrittenText(g_current_rule_declaration->getSourceRange());
    int rule_count = 1;
    unordered_map<uint32_t, string> rule_line_numbers;

    for (auto fd : g_active_fields)
    {
        if (g_generation_error)
        {
            return;
        }
        string table = fd.first;
        bool contains_last_operation = false;
        bool contains_fields = false;
        string field_subscription_code;
        string common_subscription_code;
        if (g_field_data.find(table) == g_field_data.end())
        {
            cerr << "Table '" << table << "' was not found in the catalog." << endl;
            g_generation_error = true;
            return;
        }
        string rule_name
            = g_current_ruleset + "_" + g_current_rule_declaration->getName().str() + "_" + to_string(rule_count);

        string rule_line_var = rule_line_numbers[g_current_ruleset_rule_number];

        // Declare a constant for the line number of the rule if this is the first
        // time we've seen this rule.  Note that we may see a rule multiple times if
        // the rule has multiple anchr rows.
        if (rule_line_var.empty())
        {
            rule_line_var = "c_rule_line_";
            rule_line_var.append(to_string(g_current_ruleset_rule_number));
            rule_line_numbers[g_current_ruleset_rule_number] = rule_line_var;

            common_subscription_code
                .append("    const uint32_t ")
                .append(rule_line_var)
                .append(" = ")
                .append(to_string(g_current_ruleset_rule_line_number))
                .append(";\n");
        }

        common_subscription_code
            .append("    ")
            .append(c_nolint_identifier_naming)
            .append("\n")
            .append("    rule_binding_t ")
            .append(rule_name)
            .append("binding(\"")
            .append(g_current_ruleset)
            .append("\",\"")
            .append(to_string(g_current_ruleset_rule_number));
        if (g_active_fields.size() > 1)
        {
            common_subscription_code.append("_").append(table);
        }
        common_subscription_code
            .append("\",")
            .append(g_current_ruleset)
            .append("::")
            .append(rule_name)
            .append(",")
            .append(rule_line_var)
            .append(");\n");

        field_subscription_code
            .append("    ")
            .append(c_nolint_identifier_naming)
            .append("\n")
            .append("    field_position_list_t fields_" + rule_name + ";\n");

        if (fd.second.find(c_last_operation) != fd.second.end())
        {
            contains_last_operation = true;
        }
        else
        {
            auto fields = g_field_data[table];
            contains_fields = !fd.second.empty();
            for (const auto& field : fd.second)
            {
                if (fields.find(field) == fields.end())
                {
                    cerr << "Field '" << field << "' of table '" << table << "' was not found in the catalog." << endl;
                    g_generation_error = true;
                    return;
                }

                field_data_t field_data = fields[field];
                if (field_data.is_active)
                {
                    field_subscription_code += "    fields_" + rule_name + ".push_back(" + to_string(field_data.position) + ");\n";
                }
            }
        }

        if (!contains_fields && !contains_last_operation)
        {
            cerr << "No fields referenced by table '" << table << "'." << endl;
            g_generation_error = true;
            return;
        }

        g_current_ruleset_subscription += common_subscription_code;
        g_current_ruleset_unsubscription += common_subscription_code;

        if (g_delete_operation_in_rule && fd.second.size() > 1)
        {
            cerr << "Referencing fields of a record that has been deleted is currently not supported.";
            cerr << " This condition occurs when a rule is subscribed to a delete operation and";
            cerr << " is referencing data related to the deleted record.";
            cerr << endl;
            g_generation_error = true;
            return;
        }

        if (contains_fields)
        {
            g_current_ruleset_subscription
                .append(field_subscription_code)
                .append("    subscribe_rule(gaia::")
                .append(g_table_db_data[table])
                .append("::")
                .append(table)
                .append("_t::s_gaia_type, event_type_t::row_update, fields_")
                .append(rule_name)
                .append(",")
                .append(rule_name)
                .append("binding);\n");
            g_current_ruleset_unsubscription
                .append(field_subscription_code)
                .append("    unsubscribe_rule(gaia::")
                .append(g_table_db_data[table])
                .append("::")
                .append(table)
                .append("_t::s_gaia_type, event_type_t::row_update, fields_")
                .append(rule_name)
                .append(",")
                .append(rule_name)
                .append("binding);\n");
        }

        if (contains_last_operation)
        {
            g_current_ruleset_subscription
                .append("    subscribe_rule(gaia::")
                .append(g_table_db_data[table])
                .append("::")
                .append(table)
                .append("_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,")
                .append(rule_name)
                .append("binding);\n")
                .append("    subscribe_rule(gaia::")
                .append(g_table_db_data[table])
                .append("::")
                .append(table)
                .append("_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,")
                .append(rule_name)
                .append("binding);\n");

            if (g_delete_operation_in_rule)
            {
                g_current_ruleset_subscription
                    .append("    subscribe_rule(gaia::")
                    .append(g_table_db_data[table])
                    .append("::")
                    .append(table)
                    .append("_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,")
                    .append(rule_name)
                    .append("binding);\n");
            }

            g_current_ruleset_unsubscription
                .append("    unsubscribe_rule(gaia::")
                .append(g_table_db_data[table])
                .append("::")
                .append(table)
                .append("_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,")
                .append(rule_name)
                .append("binding);\n")
                .append("    unsubscribe_rule(gaia::")
                .append(g_table_db_data[table])
                .append("::")
                .append(table)
                .append("_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,")
                .append(rule_name)
                .append("binding);\n");
            if (g_delete_operation_in_rule)
            {
                g_current_ruleset_unsubscription
                    .append("    unsubscribe_rule(gaia::")
                    .append(g_table_db_data[table])
                    .append("::")
                    .append(table)
                    .append("_t::s_gaia_type, event_type_t::row_delete, gaia::rules::empty_fields,")
                    .append(rule_name)
                    .append("binding);\n");
            }
        }

        g_current_ruleset_subscription.append("\n");
        g_current_ruleset_unsubscription.append("\n");

        navigation_code_data_t navigation_code = generate_navigation_code(table);
        string function_header = "\n";
        function_header
            .append(c_nolint_identifier_naming)
            .append("\nvoid ")
            .append(rule_name)
            .append("(const rule_context_t* context)\n");

        if (rule_count == 1)
        {
            rewriter.InsertText(g_current_rule_declaration->getLocation(), function_header);
            rewriter.InsertTextAfterToken(
                g_current_rule_declaration->getLocation(),
                navigation_code.prefix);
            rewriter.InsertText(
                g_current_rule_declaration->getEndLoc(),
                navigation_code.postfix);
        }
        else
        {
            function_header.append(insert_rule_preamble(rule_code + navigation_code.postfix, navigation_code.prefix));
            rewriter.InsertTextBefore(g_current_rule_declaration->getLocation(), function_header);
        }

        rule_count++;
    }
}

class field_get_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit field_get_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_generation_error)
        {
            return;
        }
        const auto* expression = result.Nodes.getNodeAs<DeclRefExpr>("fieldGet");
        const auto* member_expression = result.Nodes.getNodeAs<MemberExpr>("tableFieldGet");
        string table_name;
        string field_name;
        SourceRange expression_source_range;
        bool is_last_operation = false;
        if (expression != nullptr)
        {
            const ValueDecl* decl = expression->getDecl();
            if (decl->getType()->isStructureType())
            {
                return;
            }

            table_name = get_table_name(decl);
            field_name = decl->getName().str();
            g_used_tables.insert(table_name);
            g_used_dbs.insert(g_table_db_data[table_name]);

            if (decl->hasAttr<GaiaFieldAttr>())
            {
                expression_source_range = SourceRange(expression->getLocation(), expression->getEndLoc());
                if (!validate_and_add_active_field(table_name, field_name))
                {
                    return;
                }
            }
            else if (decl->hasAttr<GaiaFieldValueAttr>())
            {
                expression_source_range
                    = SourceRange(expression->getLocation().getLocWithOffset(-1), expression->getEndLoc());
            }
        }
        else if (member_expression != nullptr)
        {
            auto* declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
            if (declaration_expression != nullptr)
            {
                field_name = member_expression->getMemberNameInfo().getName().getAsString();
                table_name = declaration_expression->getDecl()->getName().str();

                is_last_operation = declaration_expression->getDecl()->hasAttr<GaiaLastOperationAttr>();

                g_used_tables.insert(table_name);
                g_used_dbs.insert(g_table_db_data[table_name]);

                if (declaration_expression->getDecl()->hasAttr<GaiaFieldValueAttr>())
                {
                    expression_source_range
                        = SourceRange(
                            member_expression->getBeginLoc().getLocWithOffset(-1),
                            member_expression->getEndLoc());
                }
                else
                {
                    expression_source_range
                        = SourceRange(
                            member_expression->getBeginLoc(),
                            member_expression->getEndLoc());
                    if (!validate_and_add_active_field(table_name, field_name))
                    {
                        return;
                    }
                }
            }
            else
            {
                cerr << "Incorrect base type of generated type." << endl;
                g_generation_error = true;
            }
        }
        else
        {
            cerr << "Incorrect matched expression." << endl;
            g_generation_error = true;
        }

        if (expression_source_range.isValid())
        {
            if (is_last_operation)
            {
                m_rewriter.ReplaceText(
                    expression_source_range,
                    "context->last_operation(gaia::"
                        + g_table_db_data[table_name] + "::"
                        + table_name + "_t::s_gaia_type)");
            }
            else
            {
                m_rewriter.ReplaceText(expression_source_range, table_name + "." + field_name + "()");
            }
        }
    }

private:
    Rewriter& m_rewriter;
};

class field_set_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit field_set_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_generation_error)
        {
            return;
        }
        const auto* op = result.Nodes.getNodeAs<BinaryOperator>("fieldSet");
        if (op != nullptr)
        {
            const Expr* operator_expression = op->getLHS();
            if (operator_expression != nullptr)
            {
                const auto* left_declaration_expression = dyn_cast<DeclRefExpr>(operator_expression);
                const auto* member_expression = dyn_cast<MemberExpr>(operator_expression);

                string table_name;
                string field_name;
                SourceLocation set_start_location;
                SourceLocation set_end_location;
                if (left_declaration_expression != nullptr || member_expression != nullptr)
                {
                    if (left_declaration_expression != nullptr)
                    {
                        const ValueDecl* operator_declaration = left_declaration_expression->getDecl();
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
                        auto* declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
                        if (declaration_expression == nullptr)
                        {
                            cerr << "Incorrect base type of generated type." << endl;
                            g_generation_error = true;
                            return;
                        }
                        field_name = member_expression->getMemberNameInfo().getName().getAsString();
                        table_name = declaration_expression->getDecl()->getName().str();
                        set_start_location = member_expression->getBeginLoc();
                    }
                    g_used_tables.insert(table_name);
                    g_used_dbs.insert(g_table_db_data[table_name]);

                    tok::TokenKind token_kind;
                    std::string replacement_text
                        = "[&]() mutable {auto w = " + table_name + ".writer(); w." + field_name;

                    switch (op->getOpcode())
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
                        cerr << "Incorrect operator type." << endl;
                        g_generation_error = true;
                        return;
                    }

                    replacement_text += convert_compound_binary_opcode(op->getOpcode());

                    if (left_declaration_expression != nullptr)
                    {
                        set_end_location = Lexer::findLocationAfterToken(
                            set_start_location, token_kind, m_rewriter.getSourceMgr(),
                            m_rewriter.getLangOpts(), true);
                    }
                    else
                    {
                        set_end_location = Lexer::findLocationAfterToken(
                            member_expression->getExprLoc(), token_kind, m_rewriter.getSourceMgr(),
                            m_rewriter.getLangOpts(), true);
                    }

                    m_rewriter.ReplaceText(
                        SourceRange(set_start_location, set_end_location.getLocWithOffset(-1)),
                        replacement_text);

                    if (op->getOpcode() != BO_Assign)
                    {
                        if (!validate_and_add_active_field(table_name, field_name))
                        {
                            return;
                        }
                        m_rewriter.InsertTextAfterToken(
                            op->getEndLoc(), "; w.update_row();return w." + field_name + ";}() ");
                    }
                    else
                    {
                        m_rewriter.InsertTextAfterToken(
                            op->getEndLoc(), "; w.update_row();return w." + field_name + ";}()");
                    }
                }
                else
                {
                    cerr << "Incorrect operator expression type." << endl;
                    g_generation_error = true;
                }
            }
            else
            {
                cerr << "Incorrect operator expression" << endl;
                g_generation_error = true;
            }
        }
        else
        {
            cerr << "Incorrect matched operator." << endl;
            g_generation_error = true;
        }
    }

private:
    std::string convert_compound_binary_opcode(BinaryOperator::Opcode op_code)
    {
        switch (op_code)
        {
        case BO_Assign:
            return "=";
        case BO_MulAssign:
            return "*=";
        case BO_DivAssign:
            return "/=";
        case BO_RemAssign:
            return "%=";
        case BO_AddAssign:
            return "+=";
        case BO_SubAssign:
            return "-=";
        case BO_ShlAssign:
            return "<<=";
        case BO_ShrAssign:
            return ">>=";
        case BO_AndAssign:
            return "&=";
        case BO_XorAssign:
            return "^=";
        case BO_OrAssign:
            return "|=";
        default:
            cerr << "Incorrect operator code " << op_code << "." << endl;
            g_generation_error = true;
            return "";
        }
    }

    Rewriter& m_rewriter;
};

class field_unary_operator_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit field_unary_operator_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_generation_error)
        {
            return;
        }
        const auto* op = result.Nodes.getNodeAs<UnaryOperator>("fieldUnaryOp");
        if (op != nullptr)
        {
            const Expr* operator_expression = op->getSubExpr();
            if (operator_expression != nullptr)
            {
                const auto* declaration_expression = dyn_cast<DeclRefExpr>(operator_expression);
                const auto* member_expression = dyn_cast<MemberExpr>(operator_expression);

                if (declaration_expression != nullptr || member_expression != nullptr)
                {
                    string replace_string;
                    string table_name;
                    string field_name;

                    if (declaration_expression != nullptr)
                    {

                        const ValueDecl* operator_declaration = declaration_expression->getDecl();
                        if (operator_declaration->getType()->isStructureType())
                        {
                            return;
                        }

                        table_name = get_table_name(operator_declaration);
                        field_name = operator_declaration->getName().str();
                    }
                    else
                    {
                        auto* declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
                        if (declaration_expression == nullptr)
                        {
                            cerr << "Incorrect base type of generated type." << endl;
                            g_generation_error = true;
                            return;
                        }
                        field_name = member_expression->getMemberNameInfo().getName().getAsString();
                        table_name = declaration_expression->getDecl()->getName().str();
                    }

                    g_used_tables.insert(table_name);
                    g_used_dbs.insert(g_table_db_data[table_name]);
                    if (!validate_and_add_active_field(table_name, field_name))
                    {
                        return;
                    }

                    if (op->isPostfix())
                    {
                        if (op->isIncrementOp())
                        {
                            replace_string
                                = "[&]() mutable {auto t="
                                + table_name + "." + field_name + "();auto w = "
                                + table_name + ".writer(); w." + field_name + "++; w.update_row();return t;}()";
                        }
                        else if (op->isDecrementOp())
                        {
                            replace_string
                                = "[&]() mutable {auto t=" + table_name + "." + field_name + "();auto w = "
                                + table_name + ".writer(); w." + field_name + "--; w.update_row();return t;}()";
                        }
                    }
                    else
                    {
                        if (op->isIncrementOp())
                        {
                            replace_string
                                = "[&]() mutable {auto w = " + table_name + ".writer(); ++ w." + field_name
                                + ";w.update_row(); return w." + field_name + ";}()";
                        }
                        else if (op->isDecrementOp())
                        {
                            replace_string
                                = "[&]() mutable {auto w = " + table_name + ".writer(); -- w." + field_name
                                + ";w.update_row(); return w." + field_name + ";}()";
                        }
                    }

                    m_rewriter.ReplaceText(
                        SourceRange(op->getBeginLoc().getLocWithOffset(-1), op->getEndLoc().getLocWithOffset(1)),
                        replace_string);
                }
                else
                {
                    cerr << "Incorrect operator expression type." << endl;
                    g_generation_error = true;
                }
            }
            else
            {
                cerr << "Incorrect operator expression." << endl;
                g_generation_error = true;
            }
        }
        else
        {
            cerr << "Incorrect matched operator." << endl;
            g_generation_error = true;
        }
    }

private:
    Rewriter& m_rewriter;
};

class rule_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit rule_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_generation_error)
        {
            return;
        }

        const auto* rule_declaration = result.Nodes.getNodeAs<FunctionDecl>("ruleDecl");
        generate_rules(m_rewriter);
        if (g_generation_error)
        {
            return;
        }

        if (g_current_rule_declaration)
        {
            g_current_ruleset_rule_number++;
        }

        g_current_rule_declaration = rule_declaration;

        SourceRange rule_range = g_current_rule_declaration->getSourceRange();
        g_current_ruleset_rule_line_number = m_rewriter.getSourceMgr().getSpellingLineNumber(rule_range.getBegin());
        g_used_tables.clear();
        g_active_fields.clear();
        g_delete_operation_in_rule = false;
        g_function_call_in_rule = false;
        g_gaia_id_call_locations.clear();
    }

private:
    Rewriter& m_rewriter;
};

class ruleset_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit ruleset_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_generation_error)
        {
            return;
        }
        generate_rules(m_rewriter);
        if (g_generation_error)
        {
            return;
        }
        g_current_rule_declaration = nullptr;
        g_used_tables.clear();
        g_active_fields.clear();

        const auto* ruleset_declaration = result.Nodes.getNodeAs<RulesetDecl>("rulesetDecl");
        if (ruleset_declaration != nullptr)
        {
            if (!g_current_ruleset.empty())
            {
                g_generated_subscription_code
                    += "namespace " + g_current_ruleset
                    + "{\nvoid subscribe_ruleset_" + g_current_ruleset
                    + "()\n{\n" + g_current_ruleset_subscription
                    + "}\nvoid unsubscribe_ruleset_" + g_current_ruleset
                    + "()\n{\n" + g_current_ruleset_unsubscription + "}\n}\n";
            }
            g_current_ruleset = ruleset_declaration->getName().str();
            g_rulesets.push_back(g_current_ruleset);
            g_current_ruleset_subscription.clear();
            g_current_ruleset_unsubscription.clear();
            g_current_ruleset_rule_number = 1;
            if (*(ruleset_declaration->decls_begin()) == nullptr)
            {
                // Empty ruleset so it doesn't make sense to process any possible attributes
                m_rewriter.ReplaceText(
                    SourceRange(
                        ruleset_declaration->getBeginLoc(),
                        ruleset_declaration->getEndLoc()),
                    "namespace " + g_current_ruleset + "\n{\n} // namespace " + g_current_ruleset + "\n");
            }
            else
            {
                // Replace ruleset declaration that may include attributes with namespace declaration
                m_rewriter.ReplaceText(
                    SourceRange(
                        ruleset_declaration->getBeginLoc(),
                        ruleset_declaration->decls_begin()->getBeginLoc().getLocWithOffset(c_declaration_to_ruleset_offset)),
                    "namespace " + g_current_ruleset + "\n{\n");

                // Replace closing brace with namespace comment.
                m_rewriter.ReplaceText(SourceRange(ruleset_declaration->getEndLoc()), "}// namespace " + g_current_ruleset);
            }
        }
    }

private:
    Rewriter& m_rewriter;
};

class update_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit update_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_generation_error)
        {
            return;
        }
        const auto* exp = result.Nodes.getNodeAs<DeclRefExpr>("UPDATE");
        if (exp != nullptr)
        {
            m_rewriter.ReplaceText(
                SourceRange(exp->getLocation(), exp->getEndLoc()),
                "gaia::rules::last_operation_t::row_update");
        }
    }

private:
    Rewriter& m_rewriter;
};

class insert_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit insert_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_generation_error)
        {
            return;
        }
        const auto* exp = result.Nodes.getNodeAs<DeclRefExpr>("INSERT");
        if (exp != nullptr)
        {
            m_rewriter.ReplaceText(
                SourceRange(exp->getLocation(), exp->getEndLoc()),
                "gaia::rules::last_operation_t::row_insert");
        }
    }

private:
    Rewriter& m_rewriter;
};

class delete_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit delete_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_generation_error)
        {
            return;
        }
        const auto* exp = result.Nodes.getNodeAs<DeclRefExpr>("DELETE");
        if (exp != nullptr)
        {
            m_rewriter.ReplaceText(
                SourceRange(exp->getLocation(), exp->getEndLoc()),
                "gaia::rules::last_operation_t::row_delete");
        }
    }

private:
    Rewriter& m_rewriter;
};

class none_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit none_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_generation_error)
        {
            return;
        }
        const auto* exp = result.Nodes.getNodeAs<DeclRefExpr>("NONE");
        if (exp != nullptr)
        {
            m_rewriter.ReplaceText(
                SourceRange(exp->getLocation(), exp->getEndLoc()),
                "gaia::rules::last_operation_t::row_none");
        }
    }

private:
    Rewriter& m_rewriter;
};

class last_operation_comparison_handler_t : public MatchFinder::MatchCallback
{
public:
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_generation_error)
        {
            return;
        }
        if (g_delete_operation_in_rule)
        {
            return;
        }
        const auto* op = result.Nodes.getNodeAs<BinaryOperator>("LastOperationComparison");
        if (op != nullptr)
        {
            const auto* lhs = op->getLHS();
            const auto* rhs = op->getRHS();
            if (lhs != nullptr && rhs != nullptr)
            {
                const auto* lhs_expression = lhs;
                const auto* rhs_expression = rhs;
                if (dyn_cast<ImplicitCastExpr>(lhs) != nullptr)
                {
                    lhs_expression = dyn_cast<ImplicitCastExpr>(lhs)->getSubExpr();
                }
                if (dyn_cast<ImplicitCastExpr>(rhs) != nullptr)
                {
                    rhs_expression = dyn_cast<ImplicitCastExpr>(rhs)->getSubExpr();
                }
                bool lhs_has_delete = false;
                bool rhs_has_delete = false;
                if (dyn_cast<MemberExpr>(lhs_expression) == nullptr)
                {
                    if (dyn_cast<DeclRefExpr>(lhs_expression) != nullptr)
                    {
                        lhs_has_delete = dyn_cast<DeclRefExpr>(lhs_expression)->getDecl()->hasAttr<GaiaLastOperationDELETEAttr>();
                    }
                }
                else if (dyn_cast<MemberExpr>(rhs_expression) == nullptr)
                {
                    if (dyn_cast<DeclRefExpr>(rhs_expression) != nullptr)
                    {
                        rhs_has_delete = dyn_cast<DeclRefExpr>(rhs_expression)->getDecl()->hasAttr<GaiaLastOperationDELETEAttr>();
                    }
                }
                if (op->getOpcode() == BO_EQ)
                {
                    if (lhs_has_delete || rhs_has_delete)
                    {
                        g_delete_operation_in_rule = true;
                    }
                }
                else if (op->getOpcode() == BO_NE)
                {
                    if (!lhs_has_delete && !rhs_has_delete)
                    {
                        g_delete_operation_in_rule = true;
                    }
                }
            }
        }
    }
};

class last_operation_switch_handler_t : public MatchFinder::MatchCallback
{
public:
    void run(const MatchFinder::MatchResult& result) override
    {
        g_delete_operation_in_rule = true;
    }
};

class last_operation_if_handler_t : public MatchFinder::MatchCallback
{
public:
    void run(const MatchFinder::MatchResult& result) override
    {
        g_delete_operation_in_rule = true;
    }
};

class function_call_handler_t : public MatchFinder::MatchCallback
{
public:
    void run(const MatchFinder::MatchResult& result) override
    {
        g_function_call_in_rule = true;
    }
};

class gaia_id_call_handler_t : public MatchFinder::MatchCallback
{
public:
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_generation_error)
        {
            return;
        }
        const auto* exp = result.Nodes.getNodeAs<CXXMemberCallExpr>("gaia_id_call");
        if (exp != nullptr)
        {
            g_gaia_id_call_locations.emplace_back(SourceRange(exp->getBeginLoc(), exp->getEndLoc()));
        }
    }
};

class variable_declaration_match_handler_t : public MatchFinder::MatchCallback
{
public:
    void run(const MatchFinder::MatchResult& result) override
    {
        const auto* variable_declaration = result.Nodes.getNodeAs<VarDecl>("varDeclaration");
        if (variable_declaration != nullptr)
        {
            const auto variable_name = variable_declaration->getNameAsString();
            if (variable_name != "")
            {
                if (g_field_data.empty())
                {
                    g_field_data = get_table_data();
                }

                if (g_generation_error)
                {
                    return;
                }

                if (g_field_data.find(variable_name) != g_field_data.end())
                {
                    cerr << "Local variable declaration '" << variable_name
                         << "' hides database table of the same name." << endl;
                    return;
                }

                for (auto table_data : g_field_data)
                {
                    if (table_data.second.find(variable_name) != table_data.second.end())
                    {
                        cerr << "Local variable declaration '" << variable_name
                             << "' hides catalog field entity of the same name." << endl;
                        return;
                    }
                }
            }
        }
    }
};

class translation_engine_consumer_t : public clang::ASTConsumer
{
public:
    explicit translation_engine_consumer_t(ASTContext* context, Rewriter& r)
        : m_field_get_match_handler(r)
        , m_field_set_match_handler(r)
        , m_rule_match_handler(r)
        , m_ruleset_match_handler(r)
        , m_field_unary_operator_match_handler(r)
        , m_update_match_handler(r)
        , m_insert_match_handler(r)
        , m_delete_match_handler(r)
        , m_none_match_handler(r)
    {
        DeclarationMatcher ruleset_matcher = rulesetDecl().bind("rulesetDecl");
        DeclarationMatcher rule_matcher
            = functionDecl(allOf(hasAncestor(ruleset_matcher), hasAttr(attr::Rule))).bind("ruleDecl");
        DeclarationMatcher variable_declaration_matcher = varDecl(hasAncestor(rule_matcher)).bind("varDeclaration");
        StatementMatcher gaia_id_call_matcher
            = cxxMemberCallExpr(
                  on(declRefExpr(
                      to(varDecl(
                          hasAttr(attr::FieldTable))))),
                  callee(cxxMethodDecl(hasName("gaia_id"))))
                  .bind("gaia_id_call");
        StatementMatcher function_call_matcher
            = cxxMemberCallExpr(
                  on(declRefExpr(
                      to(varDecl(
                          hasAttr(attr::FieldTable))))),
                  callee(cxxMethodDecl(hasName("delete_row"))))
                  .bind("delete_row_call");

        StatementMatcher last_operation_switch_matcher
            = switchStmt(allOf(
                             hasCondition(expr(ignoringParenImpCasts(memberExpr(
                                 hasDescendant(declRefExpr(to(varDecl(
                                     hasAttr(attr::GaiaLastOperation))))))))),
                             forEachSwitchCase(anyOf(
                                 defaultStmt(),
                                 caseStmt(has(ignoringParenImpCasts(declRefExpr(to(varDecl(
                                     hasAttr(attr::GaiaLastOperationDELETE)))))))))))
                  .bind("LastOperationSwitch");
        StatementMatcher last_operation_comparison_matcher
            = binaryOperator(allOf(
                                 anyOf(hasOperatorName("=="), hasOperatorName("!=")),
                                 anyOf(
                                     allOf(
                                         hasLHS(ignoringParenImpCasts(memberExpr(
                                             hasDescendant(declRefExpr(to(varDecl(
                                                 hasAttr(attr::GaiaLastOperation)))))))),
                                         hasRHS(ignoringParenImpCasts(declRefExpr(to(varDecl(
                                             anyOf(
                                                 hasAttr(attr::GaiaLastOperationUPDATE),
                                                 hasAttr(attr::GaiaLastOperationINSERT),
                                                 hasAttr(attr::GaiaLastOperationDELETE),
                                                 hasAttr(attr::GaiaLastOperationNONE)))))))),
                                     allOf(
                                         hasRHS(ignoringParenImpCasts(memberExpr(
                                             hasDescendant(declRefExpr(to(varDecl(
                                                 hasAttr(attr::GaiaLastOperation)))))))),
                                         hasLHS(ignoringParenImpCasts(declRefExpr(to(varDecl(
                                             anyOf(
                                                 hasAttr(attr::GaiaLastOperationUPDATE),
                                                 hasAttr(attr::GaiaLastOperationINSERT),
                                                 hasAttr(attr::GaiaLastOperationDELETE),
                                                 hasAttr(attr::GaiaLastOperationNONE)))))))))))
                  .bind("LastOperationComparison");
        StatementMatcher last_operation_if_statement_matcher
            = ifStmt(allOf(
                         hasCondition(last_operation_comparison_matcher),
                         hasElse(unless(ifStmt(hasCondition(last_operation_comparison_matcher))))))
                  .bind("ifLastOperation");
        StatementMatcher field_get_matcher
            = declRefExpr(to(varDecl(
                              anyOf(
                                  hasAttr(attr::GaiaField),
                                  hasAttr(attr::GaiaFieldValue)),
                              unless(hasAttr(attr::GaiaFieldLValue)))))
                  .bind("fieldGet");
        StatementMatcher field_set_matcher
            = binaryOperator(
                  allOf(
                      isAssignmentOperator(),
                      hasLHS(declRefExpr(to(varDecl(hasAttr(attr::GaiaFieldLValue)))))))
                  .bind("fieldSet");
        StatementMatcher field_unary_operator_matcher
            = unaryOperator(
                  allOf(
                      anyOf(
                          hasOperatorName("++"),
                          hasOperatorName("--")),
                      hasUnaryOperand(declRefExpr(to(varDecl(hasAttr(attr::GaiaFieldLValue)))))))
                  .bind("fieldUnaryOp");
        StatementMatcher table_field_get_matcher
            = memberExpr(
                  member(
                      allOf(
                          hasAttr(attr::GaiaField),
                          unless(hasAttr(attr::GaiaFieldLValue)))),
                  hasDescendant(declRefExpr(
                      to(varDecl(anyOf(
                          hasAttr(attr::GaiaField),
                          hasAttr(attr::FieldTable),
                          hasAttr(attr::GaiaFieldValue),
                          hasAttr(attr::GaiaLastOperation)))))))
                  .bind("tableFieldGet");
        StatementMatcher table_field_set_matcher
            = binaryOperator(allOf(
                                 isAssignmentOperator(),
                                 hasLHS(memberExpr(member(hasAttr(attr::GaiaFieldLValue))))))
                  .bind("fieldSet");
        StatementMatcher table_field_unary_operator_matcher
            = unaryOperator(allOf(
                                anyOf(
                                    hasOperatorName("++"),
                                    hasOperatorName("--")),
                                hasUnaryOperand(memberExpr(member(hasAttr(attr::GaiaFieldLValue))))))
                  .bind("fieldUnaryOp");
        StatementMatcher update_matcher
            = declRefExpr(to(varDecl(hasAttr(attr::GaiaLastOperationUPDATE)))).bind("UPDATE");
        StatementMatcher insert_matcher
            = declRefExpr(to(varDecl(hasAttr(attr::GaiaLastOperationINSERT)))).bind("INSERT");
        StatementMatcher delete_matcher
            = declRefExpr(to(varDecl(hasAttr(attr::GaiaLastOperationDELETE)))).bind("DELETE");
        StatementMatcher none_matcher
            = declRefExpr(to(varDecl(hasAttr(attr::GaiaLastOperationNONE)))).bind("NONE");

        m_matcher.addMatcher(last_operation_if_statement_matcher, &m_last_operation_if_handler);
        m_matcher.addMatcher(last_operation_comparison_matcher, &m_last_operation_comparison_handler);
        m_matcher.addMatcher(last_operation_switch_matcher, &m_last_operation_switch_handler);
        m_matcher.addMatcher(field_get_matcher, &m_field_get_match_handler);
        m_matcher.addMatcher(table_field_get_matcher, &m_field_get_match_handler);

        m_matcher.addMatcher(field_set_matcher, &m_field_set_match_handler);
        m_matcher.addMatcher(table_field_set_matcher, &m_field_set_match_handler);

        m_matcher.addMatcher(rule_matcher, &m_rule_match_handler);
        m_matcher.addMatcher(ruleset_matcher, &m_ruleset_match_handler);
        m_matcher.addMatcher(field_unary_operator_matcher, &m_field_unary_operator_match_handler);

        m_matcher.addMatcher(table_field_unary_operator_matcher, &m_field_unary_operator_match_handler);

        m_matcher.addMatcher(update_matcher, &m_update_match_handler);
        m_matcher.addMatcher(insert_matcher, &m_insert_match_handler);
        m_matcher.addMatcher(delete_matcher, &m_delete_match_handler);
        m_matcher.addMatcher(none_matcher, &m_none_match_handler);

        m_matcher.addMatcher(function_call_matcher, &m_delete_row_function_call_handler);
        m_matcher.addMatcher(gaia_id_call_matcher, &m_gaia_id_call_handler);
        m_matcher.addMatcher(variable_declaration_matcher, &m_variable_declaration_match_handler);
    }

    void HandleTranslationUnit(clang::ASTContext& context) override
    {
        m_matcher.matchAST(context);
    }

private:
    MatchFinder m_matcher;
    field_get_match_handler_t m_field_get_match_handler;
    field_set_match_handler_t m_field_set_match_handler;
    rule_match_handler_t m_rule_match_handler;
    ruleset_match_handler_t m_ruleset_match_handler;
    field_unary_operator_match_handler_t m_field_unary_operator_match_handler;
    update_match_handler_t m_update_match_handler;
    insert_match_handler_t m_insert_match_handler;
    delete_match_handler_t m_delete_match_handler;
    none_match_handler_t m_none_match_handler;
    last_operation_comparison_handler_t m_last_operation_comparison_handler;
    last_operation_switch_handler_t m_last_operation_switch_handler;
    last_operation_if_handler_t m_last_operation_if_handler;
    function_call_handler_t m_delete_row_function_call_handler;
    gaia_id_call_handler_t m_gaia_id_call_handler;
    variable_declaration_match_handler_t m_variable_declaration_match_handler;
};

class translation_engine_action_t : public clang::ASTFrontendAction
{
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& compiler, llvm::StringRef in_file) override
    {
        m_rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
        return std::unique_ptr<clang::ASTConsumer>(
            new translation_engine_consumer_t(&compiler.getASTContext(), m_rewriter));
    }
    void EndSourceFileAction() override
    {
        if (!g_translation_engine_output_option.empty())
        {
            std::remove(g_translation_engine_output_option.c_str());
        }
        generate_rules(m_rewriter);
        if (g_generation_error)
        {
            return;
        }

        g_generated_subscription_code
            += "namespace " + g_current_ruleset
            + "{\nvoid subscribe_ruleset_" + g_current_ruleset + "()\n{\n" + g_current_ruleset_subscription + "}\n"
            + "void unsubscribe_ruleset_" + g_current_ruleset + "()\n{\n" + g_current_ruleset_unsubscription + "}\n"
            + "} // namespace " + g_current_ruleset + "\n"
            + generate_general_subscription_code();

        if (!shouldEraseOutputFiles() && !g_generation_error && !g_translation_engine_output_option.empty())
        {
            std::error_code error_code;
            llvm::raw_fd_ostream output_file(g_translation_engine_output_option, error_code, llvm::sys::fs::F_None);

            if (!output_file.has_error())
            {
                output_file << "#include <cstring>\n";
                for (const string& db : g_used_dbs)
                {
                    output_file << "#include \"gaia_" << db << ".h\"\n";
                }

                output_file << "#include \"gaia/rules/rules.hpp\"\n";
                output_file << "using namespace gaia::common;\n";
                output_file << "using namespace gaia::db::triggers;\n";
                output_file << "using namespace gaia::rules;\n";

                m_rewriter.getEditBuffer(m_rewriter.getSourceMgr().getMainFileID())
                    .write(output_file);
                output_file << g_generated_subscription_code;
            }

            output_file.close();
        }
    }

private:
    Rewriter m_rewriter;
};

int main(int argc, const char** argv)
{
    cl::opt<bool> help("h", cl::desc("Alias for -help"), cl::Hidden);
    cl::list<std::string> source_files(
        cl::Positional, cl::desc("<sourceFile>"), cl::ZeroOrMore,
        cl::cat(g_translation_engine_category), cl::sub(*cl::AllSubCommands));

    cl::SetVersionPrinter(print_version);
    cl::ResetAllOptionOccurrences();
    cl::HideUnrelatedOptions(g_translation_engine_category);
    std::string error_message;
    llvm::raw_string_ostream stream(error_message);
    std::unique_ptr<CompilationDatabase> compilation_database
        = FixedCompilationDatabase::loadFromCommandLine(argc, argv, error_message);

    if (!cl::ParseCommandLineOptions(argc, argv, "A tool to generate C++ rule and rule subscription code from declarative rulesets", &stream))
    {
        stream.flush();
        return EXIT_FAILURE;
    }

    cl::PrintOptionValues();

    if (source_files.empty())
    {
        cl::PrintHelpMessage();
        return EXIT_SUCCESS;
    }

    if (source_files.size() > 1)
    {
        cerr << "Translation Engine does not support more than one source ruleset." << endl;
        return EXIT_FAILURE;
    }

    if (!compilation_database)
    {
        compilation_database = llvm::make_unique<clang::tooling::FixedCompilationDatabase>(
            ".", std::vector<std::string>());
    }

    // Create a new Clang Tool instance (a LibTooling environment).
    ClangTool tool(*compilation_database, source_files);

    tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-fgaia-extensions"));
    int result = tool.run(newFrontendActionFactory<translation_engine_action_t>().get());
    if (result == 0 && !g_generation_error)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}
