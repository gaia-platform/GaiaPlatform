/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#pragma clang diagnostic pop

#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/gaia_version.hpp"
#include "gaia_internal/db/db_client_config.hpp"

#include "table_navigation.h"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;
using namespace clang::ast_matchers;
using namespace gaia;
using namespace gaia::common;
using namespace gaia::translation;

cl::OptionCategory g_translation_engine_category("Use translation engine options");
cl::opt<string> g_translation_engine_output_option(
    "output", cl::init(""), cl::desc("output file name"), cl::cat(g_translation_engine_category));

std::string g_current_ruleset;
bool g_is_generation_error = false;
int g_current_ruleset_rule_number = 1;
unsigned int g_current_ruleset_rule_line_number = 1;
constexpr int c_declaration_to_ruleset_offset = -2;
bool g_is_rule_context_rule_name_referenced = false;
SourceRange g_rule_attribute_source_range;
bool g_is_rule_prolog_specified = false;
constexpr int c_encoding_shift = 16;
constexpr int c_encoding_mask = 0xFFFF;
constexpr char c_if_stmt[] = "if";

vector<string> g_rulesets;
unordered_map<string, unordered_set<string>> g_active_fields;
unordered_set<string> g_insert_tables;
unordered_set<string> g_update_tables;
unordered_map<string, string> g_attribute_tag_map;
unordered_set<unsigned int> g_insert_call_locations;

namespace std
{
template <>
struct hash<SourceRange>
{
    std::size_t operator()(SourceRange const& range) const noexcept
    {
        return std::hash<unsigned int>{}(
            (range.getBegin().getRawEncoding() << c_encoding_shift) | (range.getEnd().getRawEncoding() & c_encoding_mask));
    }
};
} // namespace std

unordered_map<SourceRange, vector<explicit_path_data_t>> g_expression_explicit_path_data;

unordered_set<string> g_used_dbs;

const FunctionDecl* g_current_rule_declaration = nullptr;

string g_current_ruleset_subscription;
string g_generated_subscription_code;
string g_current_ruleset_unsubscription;

enum rewriter_operation_t
{
    replace_text,
    insert_text_after_token,
    remove_text,
    insert_text_before,
};

struct rewriter_history_t
{
    SourceRange range;
    string string_argument;
    rewriter_operation_t operation;
};

vector<rewriter_history_t> g_rewriter_history;
vector<SourceRange> g_nomatch_location;
unordered_map<SourceRange, string> g_variable_declaration_location;
unordered_set<SourceRange> g_variable_declaration_init_location;
unordered_map<SourceRange, SourceLocation> g_nomatch_location_map;

// Suppress these clang-tidy warnings for now.
static const char c_nolint_identifier_naming[] = "// NOLINTNEXTLINE(readability-identifier-naming)";
static const char c_ident[] = "    ";

static void print_version(raw_ostream& stream)
{
    stream << "Gaia Translation Engine " << gaia_full_version() << "\nCopyright (c) Gaia Platform LLC\n";
}

SourceRange get_statement_source_range(const Stmt* expression, const SourceManager& source_manager, const LangOptions& options)
{
    if (expression == nullptr)
    {
        return SourceRange();
    }
    SourceRange return_value = expression->getSourceRange();
    if (dyn_cast<CompoundStmt>(expression) == nullptr)
    {
        SourceLocation end_location = Lexer::findLocationAfterToken(return_value.getEnd(),
                tok::semi, source_manager, options, true);
        if (end_location.isValid())
        {
            return_value.setEnd(end_location.getLocWithOffset(-1));
        }
    }
    return return_value;
}

SourceRange get_if_statement_source_range(const IfStmt* expression, const SourceManager& source_manager, const LangOptions& options)
{
    if (expression == nullptr)
    {
        return SourceRange();
    }
    SourceRange return_value = expression->getSourceRange();
    SourceRange nomatch_source_range = get_statement_source_range(expression->getNoMatch(), source_manager, options);
    SourceRange else_source_range = get_statement_source_range(expression->getElse(), source_manager, options);
    SourceRange then_source_range = get_statement_source_range(expression->getThen(), source_manager, options);
    if (nomatch_source_range.isValid())
    {
        return_value.setEnd(nomatch_source_range.getEnd());
    }
    else if (else_source_range.isValid())
    {
        return_value.setEnd(else_source_range.getEnd());
    }
    else
    {
        return_value.setEnd(then_source_range.getEnd());
    }
    return return_value;
}

void get_variable_name(string& variable_name, string& table_name, explicit_path_data_t& explicit_path_data)
{
    const auto& table_iterator = explicit_path_data.tag_table_map.find(variable_name);
    if (table_iterator == explicit_path_data.tag_table_map.end())
    {
        table_name = variable_name;
    }
    else
    {
        table_name = table_iterator->second;
    }
    auto defined_tag_iterator = explicit_path_data.defined_tags.find(variable_name);
    if (defined_tag_iterator != explicit_path_data.defined_tags.end())
    {
        variable_name = defined_tag_iterator->second;
    }
    if (table_name != variable_name)
    {
        explicit_path_data.tag_table_map[variable_name] = table_name;
    }

    if (explicit_path_data.tag_table_map.find(variable_name) == explicit_path_data.tag_table_map.end())
    {
        variable_name = table_navigation_t::get_variable_name(table_name, explicit_path_data.tag_table_map);
        explicit_path_data.tag_table_map[variable_name] = table_name;
    }
    explicit_path_data.variable_name = variable_name;
}

bool is_range_contained_in_another_range(const SourceRange& range1, const SourceRange& range2)
{
    if (range1 == range2)
    {
        return true;
    }

    if (range1.getBegin() == range2.getBegin() && range2.getEnd() < range1.getEnd())
    {
        return true;
    }
    if (range1.getBegin() < range2.getBegin() && range1.getEnd() == range2.getEnd())
    {
        return true;
    }
    if (range1.getBegin() < range2.getBegin() && range2.getEnd() < range1.getEnd())
    {
        return true;
    }
    return false;
}

string get_table_from_expression(const string& expression)
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

bool is_tag_defined(const unordered_map<string, string>& tag_map, const string& tag)
{
    for (const auto& defined_tag_iterator : tag_map)
    {
        if (defined_tag_iterator.second == tag)
        {
            return true;
        }
    }
    return false;
}

bool optimize_path(vector<explicit_path_data_t>& path, explicit_path_data_t& path_segment)
{
    string first_table = get_table_from_expression(path_segment.path_components.front());
    for (auto& path_iterator : path)
    {
        if (is_tag_defined(path_iterator.defined_tags, first_table))
        {
            path_segment.skip_implicit_path_generation = true;
            path.insert(path.begin(), path_segment);
            return true;
        }
        if (is_tag_defined(path_segment.defined_tags, get_table_from_expression(path_iterator.path_components.front())))
        {
            path_iterator.skip_implicit_path_generation = true;
        }
    }
    return false;
}

bool is_path_segment_contained_in_another_path(
    const vector<explicit_path_data_t>& path,
    const explicit_path_data_t& path_segment)
{
    unordered_set<string> tag_container, table_container;

    if (!path_segment.defined_tags.empty())
    {
        return false;
    }

    for (const auto& path_iterator : path)
    {
        for (const auto& table_iterator : path_iterator.path_components)
        {
            string table_name = get_table_from_expression(table_iterator);
            auto tag_iterator = path_iterator.defined_tags.find(table_name);
            if (tag_iterator != path_iterator.defined_tags.end())
            {
                table_name = tag_iterator->second;
            }
            table_container.insert(table_name);
        }
        for (const auto& tag_iterator : path_iterator.tag_table_map)
        {
            tag_container.insert(tag_iterator.first);
        }
    }

    for (const auto& tag_map_iterator : path_segment.tag_table_map)
    {
        if (tag_container.find(tag_map_iterator.first) == tag_container.end())
        {
            return false;
        }
    }

    for (const auto& table_iterator : path_segment.path_components)
    {
        if (table_container.find(get_table_from_expression(table_iterator)) == table_container.end())
        {
            return false;
        }
    }

    return true;
}

void validate_table_data()
{
    if (table_navigation_t::get_table_data().empty())
    {
        g_is_generation_error = true;
        return;
    }
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
            .append(c_ident)
            .append("if (strcmp(ruleset_name, \"")
            .append(ruleset)
            .append("\") == 0)\n")
            .append(c_ident)
            .append("{\n")
            .append(c_ident)
            .append(c_ident)
            .append("::")
            .append(ruleset)
            .append("::subscribe_ruleset_")
            .append(ruleset)
            .append("();\n")
            .append(c_ident)
            .append(c_ident)
            .append("return;\n")
            .append(c_ident)
            .append("}\n");
    }

    return_value
        .append(c_ident)
        .append("throw gaia::rules::ruleset_not_found(ruleset_name);\n")
        .append("}\n")
        .append("extern \"C\" void unsubscribe_ruleset(const char* ruleset_name)\n")
        .append("{\n");

    for (const string& ruleset : g_rulesets)
    {
        return_value
            .append(c_ident)
            .append("if (strcmp(ruleset_name, \"")
            .append(ruleset)
            .append("\") == 0)\n")
            .append(c_ident)
            .append("{\n")
            .append(c_ident)
            .append(c_ident)
            .append("::")
            .append(ruleset)
            .append("::unsubscribe_ruleset_")
            .append(ruleset)
            .append("();\n")
            .append(c_ident)
            .append(c_ident)
            .append("return;\n")
            .append(c_ident)
            .append("}\n");
    }

    return_value
        .append(c_ident)
        .append("throw ruleset_not_found(ruleset_name);\n")
        .append("}\n")
        .append("extern \"C\" void initialize_rules()\n")
        .append("{\n");

    for (const string& ruleset : g_rulesets)
    {
        return_value
            .append(c_ident)
            .append("::" + ruleset)
            .append("::subscribe_ruleset_" + ruleset + "();\n");
    }
    return_value
        .append("}\n")
        .append("} // namespace rules\n")
        .append("} // namespace gaia\n");

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

// The function parses a rule  attribute e.g.
// Employee
// Employee.name_last
// E:Employee
// E:Employee.name_last
bool parse_attribute(const string& attribute, string& table, string& field, string& tag)
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
        tagless_attribute = attribute;
    }
    size_t dot_position = tagless_attribute.find('.');
    // Handle fully qualified reference.
    if (dot_position != string::npos)
    {
        table = tagless_attribute.substr(0, dot_position);
        field = tagless_attribute.substr(dot_position + 1);
        return true;
    }
    validate_table_data();

    if (table_navigation_t::get_table_data().find(tagless_attribute) == table_navigation_t::get_table_data().end())
    {
        // Might be a field.
        for (const auto& tbl : table_navigation_t::get_table_data())
        {
            if (tbl.second.field_data.find(tagless_attribute) != tbl.second.field_data.end())
            {
                table = tbl.first;
                field = tagless_attribute;
                return true;
            }
        }
        return false;
    }
    table = tagless_attribute;
    field.clear();
    return true;
}

// This function adds a field to active fields list if it is marked as active in the catalog;
// it returns true if there was no error and false otherwise.
bool validate_and_add_active_field(const string& table_name, const string& field_name, bool is_active_from_field = false)
{
    if (g_is_rule_prolog_specified && is_active_from_field)
    {
        cerr << "Since a rule attribute was provided, specifying active fields inside the rule is not supported." << endl;
        g_is_generation_error = true;
        return false;
    }

    validate_table_data();

    if (g_is_generation_error)
    {
        return false;
    }

    if (table_navigation_t::get_table_data().find(table_name) == table_navigation_t::get_table_data().end())
    {
        cerr << "Table '" << table_name << "' was not found in the catalog." << endl;
        g_is_generation_error = true;
        return false;
    }

    auto fields = table_navigation_t::get_table_data().find(table_name)->second.field_data;

    if (fields.find(field_name) == fields.end())
    {
        cerr << "Field '" << field_name << "' of table '" << table_name << "' was not found in the catalog." << endl;
        g_is_generation_error = true;
        return false;
    }

    if (fields[field_name].is_deprecated)
    {
        cerr << "Field '" << field_name << "' of table '" << table_name << "' is deprecated in the catalog." << endl;
        g_is_generation_error = true;
        return false;
    }

    // TODO[GAIAPLAT-622] If we ever add a "strict" mode to the database, then we
    // should reinstate checking for active fields.

    g_active_fields[table_name].insert(field_name);
    return true;
}

string get_table_name(const string& table, const unordered_map<string, string>& tag_map)
{
    auto tag_iterator = tag_map.find(table);
    if (tag_iterator == tag_map.end())
    {
        tag_iterator = g_attribute_tag_map.find(table);
        if (tag_iterator != g_attribute_tag_map.end())
        {
            return tag_iterator->second;
        }
    }
    else
    {
        return tag_iterator->second;
    }

    return table;
}

void generate_navigation(const string& anchor_table, Rewriter& rewriter)
{
    if (g_is_generation_error)
    {
        return;
    }

    for (const auto& explicit_path_data_iterator : g_expression_explicit_path_data)
    {
        for (const auto& data_iterator : explicit_path_data_iterator.second)
        {
            string anchor_table_name = get_table_name(
                get_table_from_expression(anchor_table), data_iterator.tag_table_map);
            SourceRange nomatch_range;
            for (const auto& nomatch_source_range : g_nomatch_location)
            {
                if (explicit_path_data_iterator.first.getEnd() == nomatch_source_range.getEnd())
                {
                    nomatch_range = nomatch_source_range;
                    break;
                }
            }

            SourceRange variable_declaration_range;
            for (const auto& variable_declaration_range_iterator : g_variable_declaration_location)
            {
                string variable_name = variable_declaration_range_iterator.second;
                if (g_attribute_tag_map.find(variable_name) != g_attribute_tag_map.end())
                {
                    cerr << "Local variable declaration '" << variable_name
                        << "' hides a tag of the same name." << endl;
                }
                if (is_range_contained_in_another_range(
                        explicit_path_data_iterator.first, variable_declaration_range_iterator.first))
                {
                    if (data_iterator.tag_table_map.find(variable_name) != data_iterator.tag_table_map.end()
                        || is_tag_defined(data_iterator.defined_tags, variable_name))
                    {
                        cerr << "Local variable declaration '" << variable_name
                             << "' hides a tag of the same name." << endl;
                    }

                    if (g_variable_declaration_init_location.find(variable_declaration_range_iterator.first)
                        != g_variable_declaration_init_location.end())
                    {
                        string table_name = get_table_name(
                            get_table_from_expression(
                                data_iterator.path_components.front()),
                            data_iterator.tag_table_map);

                        if (data_iterator.path_components.size() == 1
                            && table_name == anchor_table_name && !data_iterator.is_absolute_path)
                        {
                            auto declaration_source_range_size =
                                variable_declaration_range_iterator.first.getEnd().getRawEncoding() - variable_declaration_range_iterator.first.getBegin().getRawEncoding();
                            auto min_declaration_source_range_size =
                                variable_declaration_range.getEnd().getRawEncoding() - variable_declaration_range.getBegin().getRawEncoding();
                            if (variable_declaration_range.isInvalid() || declaration_source_range_size < min_declaration_source_range_size)
                            {
                                variable_declaration_range = variable_declaration_range_iterator.first;
                            }
                        }
                        else
                        {
                            cerr << "Initialization of declared variable with EDC objects is not supported." << endl;
                            g_is_generation_error = true;
                            return;
                        }
                    }
                }
            }

            if (variable_declaration_range.isValid())
            {
                string declaration_code = rewriter.getRewrittenText(variable_declaration_range);
                if (!declaration_code.empty())
                {
                    size_t start_position = declaration_code.find(data_iterator.variable_name);
                    if (start_position != std::string::npos)
                    {
                        const auto& table_data = table_navigation_t::get_table_data();
                        auto anchor_table_data_itr = table_data.find(anchor_table_name);

                        if (anchor_table_data_itr == table_data.end())
                        {
                            return;
                        }

                        string replacement_code
                            = string("gaia::")
                                  .append(anchor_table_data_itr->second.db_name)
                                  .append("::")
                                  .append(anchor_table_name)
                                  .append("_t::get(context->record)");

                        declaration_code.replace(start_position, data_iterator.variable_name.length(), replacement_code);
                        rewriter.ReplaceText(variable_declaration_range, declaration_code);
                        continue;
                    }
                }
            }

            if (data_iterator.skip_implicit_path_generation && data_iterator.path_components.size() == 1)
            {
                continue;
            }

            navigation_code_data_t navigation_code = table_navigation_t::generate_explicit_navigation_code(
                anchor_table, data_iterator);
            if (navigation_code.prefix.empty())
            {
                g_is_generation_error = true;
                return;
            }

            if (nomatch_range.isValid())
            {
                string variable_name = table_navigation_t::get_variable_name("", unordered_map<string, string>());
                string nomatch_prefix = "{\nbool " + variable_name + " = false;\n";
                rewriter.InsertTextBefore(
                    explicit_path_data_iterator.first.getBegin(),
                    nomatch_prefix + navigation_code.prefix);
                rewriter.InsertTextAfter(explicit_path_data_iterator.first.getBegin(), variable_name + " = true;\n");
                rewriter.ReplaceText(
                    SourceRange(g_nomatch_location_map[nomatch_range], nomatch_range.getEnd()),
                        navigation_code.postfix + "\nif (!" + variable_name + ")\n" + rewriter.getRewrittenText(nomatch_range) + "}\n");
            }
            else
            {
                rewriter.InsertTextBefore(
                    explicit_path_data_iterator.first.getBegin(),
                    navigation_code.prefix);
                rewriter.InsertTextAfter(
                    explicit_path_data_iterator.first.getEnd(),
                    navigation_code.postfix);
            }
        }
    }
}

void generate_table_subscription(
    const string& table,
    const string& field_subscription_code,
    int rule_count,
    bool subscribe_update,
    unordered_map<uint32_t, string>& rule_line_numbers,
    Rewriter& rewriter)
{
    string common_subscription_code;
    if (table_navigation_t::get_table_data().find(table) == table_navigation_t::get_table_data().end())
    {
        cerr << "Table '" << table << "' was not found in the catalog." << endl;
        g_is_generation_error = true;
        return;
    }
    string rule_name
        = g_current_ruleset + "_" + g_current_rule_declaration->getName().str() + "_" + to_string(rule_count);
    string rule_name_log = to_string(g_current_ruleset_rule_number);
    rule_name_log.append("_").append(table);

    string rule_line_var = rule_line_numbers[g_current_ruleset_rule_number];

    // Declare a constant for the line number of the rule if this is the first
    // time we've seen this rule.  Note that we may see a rule multiple times if
    // the rule has multiple anchor rows.
    if (rule_line_var.empty())
    {
        rule_line_var = "c_rule_line_";
        rule_line_var.append(to_string(g_current_ruleset_rule_number));
        rule_line_numbers[g_current_ruleset_rule_number] = rule_line_var;

        common_subscription_code
            .append(c_ident)
            .append("const uint32_t ")
            .append(rule_line_var)
            .append(" = ")
            .append(to_string(g_current_ruleset_rule_line_number))
            .append(";\n");
    }

    common_subscription_code
        .append(c_ident)
        .append(c_nolint_identifier_naming)
        .append("\n")
        .append(c_ident)
        .append("gaia::rules::rule_binding_t ")
        .append(rule_name)
        .append("binding(\"")
        .append(g_current_ruleset)
        .append("\",\"")
        .append(rule_name_log)
        .append("\",")
        .append(g_current_ruleset)
        .append("::")
        .append(rule_name)
        .append(",")
        .append(rule_line_var)
        .append(");\n");

    g_current_ruleset_subscription += common_subscription_code;
    g_current_ruleset_unsubscription += common_subscription_code;

    if (field_subscription_code.empty())
    {
        g_current_ruleset_subscription
            .append(c_ident)
            .append("gaia::rules::subscribe_rule(gaia::")
            .append(table_navigation_t::get_table_data().find(table)->second.db_name)
            .append("::")
            .append(table);
        if (subscribe_update)
        {
            g_current_ruleset_subscription.append(
                "_t::s_gaia_type, gaia::db::triggers::event_type_t::row_update, gaia::rules::empty_fields,");
        }
        else
        {
            g_current_ruleset_subscription.append(
                "_t::s_gaia_type, gaia::db::triggers::event_type_t::row_insert, gaia::rules::empty_fields,");
        }
        g_current_ruleset_subscription
            .append(rule_name)
            .append("binding);\n");

        g_current_ruleset_unsubscription
            .append(c_ident)
            .append("gaia::rules::unsubscribe_rule(gaia::")
            .append(table_navigation_t::get_table_data().find(table)->second.db_name)
            .append("::")
            .append(table)
            .append("_t::s_gaia_type, gaia::db::triggers::event_type_t::row_insert, gaia::rules::empty_fields,")
            .append(rule_name)
            .append("binding);\n");
    }
    else
    {
        g_current_ruleset_subscription
            .append(field_subscription_code)
            .append(c_ident)
            .append("gaia::rules::subscribe_rule(gaia::")
            .append(table_navigation_t::get_table_data().find(table)->second.db_name)
            .append("::")
            .append(table)
            .append("_t::s_gaia_type, gaia::db::triggers::event_type_t::row_update, fields_")
            .append(rule_name)
            .append(",")
            .append(rule_name)
            .append("binding);\n");
        g_current_ruleset_unsubscription
            .append(field_subscription_code)
            .append(c_ident)
            .append("gaia::rules::unsubscribe_rule(gaia::")
            .append(table_navigation_t::get_table_data().find(table)->second.db_name)
            .append("::")
            .append(table)
            .append("_t::s_gaia_type, gaia::db::triggers::event_type_t::row_update, fields_")
            .append(rule_name)
            .append(",")
            .append(rule_name)
            .append("binding);\n");
    }

    string function_prologue = string("\n")
                                   .append(c_nolint_identifier_naming)
                                   .append("\nvoid ")
                                   .append(rule_name);
    bool is_absoute_path_only = true;
    for (const auto& explicit_path_data_iterator : g_expression_explicit_path_data)
    {
        if (!is_absoute_path_only)
        {
            break;
        }
        for (const auto& data_iterator : explicit_path_data_iterator.second)
        {
            if (!data_iterator.is_absolute_path)
            {
                is_absoute_path_only = false;
                break;
            }
        }
    }

    if (!g_is_rule_context_rule_name_referenced && (is_absoute_path_only || g_expression_explicit_path_data.empty()))
    {
        function_prologue.append("(const gaia::rules::rule_context_t*)\n");
    }
    else
    {
        function_prologue.append("(const gaia::rules::rule_context_t* context)\n");
    }

    if (rule_count == 1)
    {
        if (g_is_rule_context_rule_name_referenced)
        {
            rewriter.InsertTextAfterToken(
                g_current_rule_declaration->getLocation(),
                "\nstatic const char gaia_rule_name[] = \"" + rule_name_log + "\";\n");
        }
        if (g_rule_attribute_source_range.isValid())
        {
            rewriter.ReplaceText(g_rule_attribute_source_range, function_prologue);
        }
        else
        {
            rewriter.InsertText(g_current_rule_declaration->getLocation(), function_prologue);
        }

        generate_navigation(table, rewriter);
    }
    else
    {
        Rewriter copy_rewriter = Rewriter(rewriter.getSourceMgr(), rewriter.getLangOpts());

        for (const auto& history_item : g_rewriter_history)
        {
            switch (history_item.operation)
            {
            case replace_text:
                copy_rewriter.ReplaceText(history_item.range, history_item.string_argument);
                break;
            case insert_text_after_token:
                copy_rewriter.InsertTextAfterToken(history_item.range.getBegin(), history_item.string_argument);
                break;
            case remove_text:
                copy_rewriter.RemoveText(history_item.range);
                break;
            case insert_text_before:
                copy_rewriter.InsertTextBefore(history_item.range.getBegin(), history_item.string_argument);
                break;
            default:
                break;
            }
        }
        if (g_is_rule_context_rule_name_referenced)
        {
            copy_rewriter.InsertTextAfterToken(
                g_current_rule_declaration->getLocation(),
                "\nstatic const char gaia_rule_name[] = \"" + rule_name_log + "\";\n");
        }

        generate_navigation(table, copy_rewriter);

        if (g_rule_attribute_source_range.isValid())
        {
            copy_rewriter.RemoveText(g_rule_attribute_source_range);
            rewriter.InsertTextBefore(
                g_rule_attribute_source_range.getBegin(),
                function_prologue + copy_rewriter.getRewrittenText(g_current_rule_declaration->getSourceRange()));
        }
        else
        {
            rewriter.InsertTextBefore(
                g_current_rule_declaration->getLocation(),
                function_prologue + copy_rewriter.getRewrittenText(g_current_rule_declaration->getSourceRange()));
        }
    }
}

void optimize_subscription(const string& table, int rule_count)
{
    // This is to reuse the same rule function and rule_binding_t
    // for the same table in case update and insert operation.
    if (g_insert_tables.find(table) != g_insert_tables.end())
    {
        string rule_name
            = g_current_ruleset + "_" + g_current_rule_declaration->getName().str() + "_" + to_string(rule_count);
        g_current_ruleset_subscription
            .append(c_ident)
            .append("gaia::rules::subscribe_rule(gaia::")
            .append(table_navigation_t::get_table_data().find(table)->second.db_name)
            .append("::")
            .append(table)
            .append("_t::s_gaia_type, gaia::db::triggers::event_type_t::row_insert, gaia::rules::empty_fields,")
            .append(rule_name)
            .append("binding);\n");

        g_current_ruleset_unsubscription
            .append(c_ident)
            .append("gaia::rules::unsubscribe_rule(gaia::")
            .append(table_navigation_t::get_table_data().find(table)->second.db_name)
            .append("::")
            .append(table)
            .append("_t::s_gaia_type, gaia::db::triggers::event_type_t::row_insert, gaia::rules::empty_fields,")
            .append(rule_name)
            .append("binding);\n");

        g_insert_tables.erase(table);
    }
}

void generate_rules(Rewriter& rewriter)
{
    validate_table_data();
    if (g_is_generation_error)
    {
        return;
    }
    if (g_current_rule_declaration == nullptr)
    {
        return;
    }
    if (g_active_fields.empty() && g_update_tables.empty() && g_insert_tables.empty())
    {
        cerr << "No active fields for the rule." << endl;
        g_is_generation_error = true;
        return;
    }
    int rule_count = 1;
    unordered_map<uint32_t, string> rule_line_numbers;

    // Optimize active fields by removing field subscriptions
    // if entire table was subscribed in rule prolog.
    for (const auto& table : g_update_tables)
    {
        g_active_fields.erase(table);
    }

    for (const auto& field_description : g_active_fields)
    {
        if (g_is_generation_error)
        {
            return;
        }

        string table = field_description.first;

        if (field_description.second.empty())
        {
            cerr << "No fields referenced by table '" << table << "'." << endl;
            g_is_generation_error = true;
            return;
        }

        string field_subscription_code;
        string rule_name
            = g_current_ruleset + "_" + g_current_rule_declaration->getName().str() + "_" + to_string(rule_count);

        field_subscription_code
            .append(c_ident)
            .append(c_nolint_identifier_naming)
            .append("\n")
            .append(c_ident)
            .append("gaia::common::field_position_list_t fields_")
            .append(rule_name)
            .append(";\n");

        auto fields = table_navigation_t::get_table_data().find(table)->second.field_data;

        for (const auto& field : field_description.second)
        {
            if (fields.find(field) == fields.end())
            {
                cerr << "Field '" << field << "' of table '" << table << "' was not found in the catalog." << endl;
                g_is_generation_error = true;
                return;
            }
            field_subscription_code
                .append(c_ident)
                .append("fields_")
                .append(rule_name)
                .append(".push_back(")
                .append(to_string(fields[field].position))
                .append(");\n");
        }

        generate_table_subscription(table, field_subscription_code, rule_count, true, rule_line_numbers, rewriter);

        optimize_subscription(table, rule_count);

        rule_count++;
    }

    for (const auto& table : g_update_tables)
    {
        if (g_is_generation_error)
        {
            return;
        }

        generate_table_subscription(table, "", rule_count, true, rule_line_numbers, rewriter);

        optimize_subscription(table, rule_count);

        rule_count++;
    }

    for (const auto& table : g_insert_tables)
    {
        if (g_is_generation_error)
        {
            return;
        }

        generate_table_subscription(table, "", rule_count, false, rule_line_numbers, rewriter);
        rule_count++;
    }
}

void update_expression_location(SourceRange& source, SourceLocation start, SourceLocation end)
{
    if (source.isInvalid())
    {
        source.setBegin(start);
        source.setEnd(end);
        return;
    }

    if (start < source.getBegin())
    {
        source.setBegin(start);
    }

    if (source.getEnd() < end)
    {
        source.setEnd(end);
    }
}

SourceRange get_expression_source_range(ASTContext* context, const Stmt& node, const SourceRange& source_range, Rewriter& rewriter)
{
    SourceRange return_value(source_range.getBegin(), source_range.getEnd());
    if (g_is_generation_error)
    {
        return return_value;
    }
    auto node_parents = context->getParents(node);

    for (const auto& node_parents_iterator : node_parents)
    {
        if (node_parents_iterator.get<CompoundStmt>())
        {
            return return_value;
        }
        else if (node_parents_iterator.get<FunctionDecl>())
        {
            return return_value;
        }
        else if (const auto* expression = node_parents_iterator.get<CXXOperatorCallExpr>())
        {
            auto offset = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
            update_expression_location(return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<BinaryOperator>())
        {
            auto offset = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
            update_expression_location(return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<UnaryOperator>())
        {
            auto offset = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
            update_expression_location(return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<ConditionalOperator>())
        {
            auto offset = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
            update_expression_location(return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<BinaryConditionalOperator>())
        {
            auto offset = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
            update_expression_location(return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<CompoundAssignOperator>())
        {
            auto offset = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
            update_expression_location(return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<CXXMemberCallExpr>())
        {
            auto offset = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
            update_expression_location(return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<CallExpr>())
        {
            auto offset = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
            update_expression_location(return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<IfStmt>())
        {
            if (is_range_contained_in_another_range(expression->getCond()->getSourceRange(), return_value)
                || is_range_contained_in_another_range(return_value, expression->getCond()->getSourceRange()))
            {
                SourceRange if_source_range = get_if_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
                update_expression_location(return_value, if_source_range.getBegin(), if_source_range.getEnd());
            }
            return return_value;
        }
        else if (const auto* expression = node_parents_iterator.get<SwitchStmt>())
        {
            auto offset = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
            update_expression_location(return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            return return_value;
        }
        else if (const auto* expression = node_parents_iterator.get<WhileStmt>())
        {
            if (is_range_contained_in_another_range(expression->getCond()->getSourceRange(), return_value)
                || is_range_contained_in_another_range(return_value, expression->getCond()->getSourceRange()))
            {
                auto offset
                    = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
                update_expression_location(
                    return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            }
            return return_value;
        }
        else if (const auto* expression = node_parents_iterator.get<DoStmt>())
        {
            if (is_range_contained_in_another_range(expression->getCond()->getSourceRange(), return_value)
                || is_range_contained_in_another_range(return_value, expression->getCond()->getSourceRange()))
            {
                auto offset
                    = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
                update_expression_location(
                    return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            }
            return return_value;
        }
        else if (const auto* expression = node_parents_iterator.get<ForStmt>())
        {
            if (is_range_contained_in_another_range(expression->getCond()->getSourceRange(), return_value)
                || is_range_contained_in_another_range(expression->getInit()->getSourceRange(), return_value)
                || is_range_contained_in_another_range(expression->getInc()->getSourceRange(), return_value)
                || is_range_contained_in_another_range(return_value, expression->getCond()->getSourceRange())
                || is_range_contained_in_another_range(return_value, expression->getInit()->getSourceRange())
                || is_range_contained_in_another_range(return_value, expression->getInc()->getSourceRange()))
            {
                auto offset
                    = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
                update_expression_location(
                    return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            }
            return return_value;
        }
        else if (const auto* expression = node_parents_iterator.get<GaiaForStmt>())
        {
            if (is_range_contained_in_another_range(
                SourceRange(expression->getLParenLoc().getLocWithOffset(1), expression->getRParenLoc().getLocWithOffset(-1)),
                return_value))
            {
                auto offset
                    = Lexer::MeasureTokenLength(expression->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
                update_expression_location(
                    return_value, expression->getBeginLoc(), expression->getEndLoc().getLocWithOffset(offset));
            }
            return return_value;
        }
        else if (const auto* declaration = node_parents_iterator.get<VarDecl>())
        {
            auto offset
                = Lexer::MeasureTokenLength(declaration->getEndLoc(), rewriter.getSourceMgr(), rewriter.getLangOpts()) + 1;
            update_expression_location(
                return_value, declaration->getBeginLoc(), declaration->getEndLoc().getLocWithOffset(offset));
            return_value.setEnd(declaration->getEndLoc().getLocWithOffset(offset));
            return_value.setBegin(declaration->getBeginLoc());
            auto node_parents = context->getParents(*declaration);
            return get_expression_source_range(context, *(node_parents[0].get<DeclStmt>()), return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<Stmt>())
        {
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
    }
    return return_value;
}

bool is_expression_from_body(ASTContext* context, const Stmt& node)
{
    auto node_parents = context->getParents(node);
    for (const auto& node_parents_iterator : node_parents)
    {
        if (const auto* expression = node_parents_iterator.get<IfStmt>())
        {
            return !is_range_contained_in_another_range(expression->getCond()->getSourceRange(), node.getSourceRange());
        }
        else if (const auto* expression = node_parents_iterator.get<WhileStmt>())
        {
            return !is_range_contained_in_another_range(expression->getCond()->getSourceRange(), node.getSourceRange());
        }
        else if (const auto* expression = node_parents_iterator.get<DoStmt>())
        {
            return !is_range_contained_in_another_range(expression->getCond()->getSourceRange(), node.getSourceRange());
        }
        else if (const auto* expression = node_parents_iterator.get<ForStmt>())
        {
            return !(
                is_range_contained_in_another_range(expression->getInit()->getSourceRange(), node.getSourceRange())
                || is_range_contained_in_another_range(expression->getCond()->getSourceRange(), node.getSourceRange())
                || is_range_contained_in_another_range(expression->getInc()->getSourceRange(), node.getSourceRange()));
        }
        else if (const auto* expression = node_parents_iterator.get<GaiaForStmt>())
        {
            return !is_range_contained_in_another_range(
                SourceRange(expression->getLParenLoc().getLocWithOffset(1), expression->getRParenLoc().getLocWithOffset(-1)),
                node.getSourceRange());
        }
        else if (const auto* declaration = node_parents_iterator.get<VarDecl>())
        {
            auto node_parents = context->getParents(*declaration);
            return is_expression_from_body(context, *(node_parents[0].get<DeclStmt>()));
        }
        else if (const auto* expression = node_parents_iterator.get<Stmt>())
        {
            return is_expression_from_body(context, *expression);
        }
    }
    return false;
}

bool should_expression_location_be_merged(ASTContext* context, const Stmt& node, bool special_parent = false)
{
    auto node_parents = context->getParents(node);
    for (const auto& node_parents_iterator : node_parents)
    {
        if (const auto* expression = node_parents_iterator.get<IfStmt>())
        {
            if (special_parent)
            {
                return false;
            }
            else
            {
                return should_expression_location_be_merged(context, *expression, true);
            }
        }
        else if (const auto* expression = node_parents_iterator.get<WhileStmt>())
        {
            if (special_parent)
            {
                return false;
            }
            else
            {
                return should_expression_location_be_merged(context, *expression, true);
            }
        }
        else if (const auto* expression = node_parents_iterator.get<DoStmt>())
        {
            if (special_parent)
            {
                return false;
            }
            else
            {
                return should_expression_location_be_merged(context, *expression, true);
            }
        }
        else if (const auto* expression = node_parents_iterator.get<ForStmt>())
        {
            if (special_parent)
            {
                return false;
            }
            else
            {
                return should_expression_location_be_merged(context, *expression, true);
            }
        }
        else if (const auto* expression = node_parents_iterator.get<GaiaForStmt>())
        {
            if (special_parent)
            {
                return false;
            }
            else
            {
                return should_expression_location_be_merged(context, *expression, true);
            }
        }
        else if (const auto* declaration = node_parents_iterator.get<VarDecl>())
        {
            auto node_parents = context->getParents(*declaration);
            return should_expression_location_be_merged(context, *(node_parents[0].get<DeclStmt>()));
        }
        else if (const auto* expression = node_parents_iterator.get<Stmt>())
        {
            return should_expression_location_be_merged(context, *expression, special_parent);
        }
    }
    return true;
}

void update_expression_explicit_path_data(
    ASTContext* context,
    const Stmt* node,
    explicit_path_data_t data,
    const SourceRange& source_range,
    Rewriter& rewriter)
{
    if (node == nullptr || data.path_components.empty())
    {
        return;
    }
    vector<explicit_path_data_t> path_data;
    SourceRange expression_source_range = get_expression_source_range(context, *node, source_range, rewriter);
    if (expression_source_range.isInvalid())
    {
        return;
    }
    for (auto& expression_explicit_path_data_iterator : g_expression_explicit_path_data)
    {
        if (is_range_contained_in_another_range(expression_explicit_path_data_iterator.first, expression_source_range))
        {
            if (!is_expression_from_body(context, *node))
            {
                if (is_path_segment_contained_in_another_path(expression_explicit_path_data_iterator.second, data))
                {
                    return;
                }
                if (optimize_path(expression_explicit_path_data_iterator.second, data))
                {
                    return;
                }
                if (expression_explicit_path_data_iterator.first == expression_source_range
                    || should_expression_location_be_merged(context, *node))
                {
                    expression_explicit_path_data_iterator.second.push_back(data);
                    return;
                }
            }
            else
            {
                string first_component = get_table_from_expression(data.path_components.front());
                for (const auto& defined_tag_iterator : expression_explicit_path_data_iterator.second.front().defined_tags)
                {
                    data.tag_table_map[defined_tag_iterator.second] = defined_tag_iterator.first;
                    if (first_component == defined_tag_iterator.second)
                    {
                        data.skip_implicit_path_generation = true;
                    }
                }
            }
        }
    }
    path_data.push_back(data);

    auto explicit_path_data_iterator = g_expression_explicit_path_data.find(expression_source_range);
    if (explicit_path_data_iterator == g_expression_explicit_path_data.end())
    {
        g_expression_explicit_path_data[expression_source_range] = path_data;
    }
    else
    {
        explicit_path_data_iterator->second.insert(explicit_path_data_iterator->second.end(), path_data.begin(), path_data.end());
    }
}

void update_expression_used_tables(
    ASTContext* context,
    const Stmt* node,
    const string& table,
    const string& variable_name,
    const SourceRange& source_range,
    Rewriter& rewriter)
{
    explicit_path_data_t path_data;
    string table_name = get_table_from_expression(table);
    path_data.is_absolute_path = false;
    path_data.path_components.push_back(table);
    path_data.tag_table_map[variable_name] = table_name;
    path_data.used_tables.insert(table_name);
    path_data.variable_name = variable_name;
    update_expression_explicit_path_data(context, node, path_data, source_range, rewriter);
}

bool get_explicit_path_data(const Decl* decl, explicit_path_data_t& data, SourceRange& path_source_range)
{
    if (decl == nullptr)
    {
        return false;
    }
    const GaiaExplicitPathAttr* explicit_path_attribute = decl->getAttr<GaiaExplicitPathAttr>();
    if (explicit_path_attribute == nullptr)
    {
        return false;
    }
    data.is_absolute_path = explicit_path_attribute->getPath().startswith("/");
    path_source_range.setBegin(SourceLocation::getFromRawEncoding(explicit_path_attribute->getPathStart()));
    path_source_range.setEnd(SourceLocation::getFromRawEncoding(explicit_path_attribute->getPathEnd()));
    vector<string> path_components;
    unordered_map<string, string> tag_map;
    for (const auto& path_component_iterator : explicit_path_attribute->pathComponents())
    {
        data.path_components.push_back(path_component_iterator);
    }

    data.used_tables.insert(get_table_from_expression(data.path_components.front()));
    const GaiaExplicitPathTagKeysAttr* explicit_path_tag_key_attribute = decl->getAttr<GaiaExplicitPathTagKeysAttr>();
    const GaiaExplicitPathTagValuesAttr* explicit_path_tag_value_attribute = decl->getAttr<GaiaExplicitPathTagValuesAttr>();

    const GaiaExplicitPathDefinedTagKeysAttr* explicit_path_defined_tag_key_attribute
        = decl->getAttr<GaiaExplicitPathDefinedTagKeysAttr>();
    const GaiaExplicitPathDefinedTagValuesAttr* explicit_path_defined_tag_value_attribute
        = decl->getAttr<GaiaExplicitPathDefinedTagValuesAttr>();

    if (explicit_path_tag_key_attribute->tagMapKeys_size() != explicit_path_tag_value_attribute->tagMapValues_size())
    {
        g_is_generation_error = true;
        return false;
    }
    vector<string> tag_map_keys, tag_map_values, defined_tag_map_keys, defined_tag_map_values;

    for (const auto& tag_map_keys_iterator : explicit_path_tag_key_attribute->tagMapKeys())
    {
        tag_map_keys.push_back(tag_map_keys_iterator);
    }

    for (const auto& tag_map_values_iterator : explicit_path_tag_value_attribute->tagMapValues())
    {
        tag_map_values.push_back(tag_map_values_iterator);
    }

    if (explicit_path_defined_tag_key_attribute != nullptr)
    {
        for (const auto& tag_map_keys_iterator : explicit_path_defined_tag_key_attribute->tagMapKeys())
        {
            defined_tag_map_keys.push_back(tag_map_keys_iterator);
        }

        for (const auto& tag_map_values_iterator : explicit_path_defined_tag_value_attribute->tagMapValues())
        {
            defined_tag_map_values.push_back(tag_map_values_iterator);
        }
    }

    for (unsigned int tag_index = 0; tag_index < tag_map_keys.size(); ++tag_index)
    {
        data.tag_table_map[tag_map_keys[tag_index]] = tag_map_values[tag_index];
    }

    for (unsigned int tag_index = 0; tag_index < defined_tag_map_keys.size(); ++tag_index)
    {
        data.defined_tags[defined_tag_map_keys[tag_index]] = defined_tag_map_values[tag_index];
    }

    for (const auto& attribute_tag_map_iterator : g_attribute_tag_map)
    {
        data.tag_table_map[attribute_tag_map_iterator.first] = attribute_tag_map_iterator.second;
    }
    return true;
}

void update_used_dbs(const explicit_path_data_t& explicit_path_data)
{
    for (const auto& component : explicit_path_data.path_components)
    {
        string table_name = get_table_from_expression(component);

        auto tag_table_iterator = explicit_path_data.tag_table_map.find(table_name);
        if (tag_table_iterator != explicit_path_data.tag_table_map.end())
        {
            table_name = tag_table_iterator->second;
        }
        g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
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
        validate_table_data();
        if (g_is_generation_error)
        {
            return;
        }
        const auto* expression = result.Nodes.getNodeAs<DeclRefExpr>("fieldGet");
        const auto* member_expression = result.Nodes.getNodeAs<MemberExpr>("tableFieldGet");
        string table_name;
        string field_name;
        string variable_name;
        SourceRange expression_source_range;
        explicit_path_data_t explicit_path_data;
        bool explicit_path_present = true;
        if (expression != nullptr)
        {
            const ValueDecl* decl = expression->getDecl();
            if (decl->getType()->isStructureType())
            {
                return;
            }
            table_name = get_table_name(decl);
            field_name = decl->getName().str();
            variable_name = expression->getNameInfo().getAsString();
            if (!get_explicit_path_data(decl, explicit_path_data, expression_source_range))
            {
                variable_name = table_navigation_t::get_variable_name(table_name, explicit_path_data.tag_table_map);
                explicit_path_present = false;
                expression_source_range = SourceRange(expression->getLocation(), expression->getEndLoc());
                g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
            }
            else
            {
                variable_name = get_table_from_expression(explicit_path_data.path_components.back());
                get_variable_name(variable_name, table_name, explicit_path_data);
                update_used_dbs(explicit_path_data);
                expression_source_range.setEnd(expression->getEndLoc());
            }
            if (decl->hasAttr<GaiaFieldValueAttr>())
            {
                if (!explicit_path_present)
                {
                    expression_source_range
                        = SourceRange(expression_source_range.getBegin().getLocWithOffset(-1), expression_source_range.getEnd());
                }

                if (!validate_and_add_active_field(table_name, field_name, true))
                {
                    return;
                }
            }
        }
        else if (member_expression != nullptr)
        {
            auto* declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
            if (declaration_expression != nullptr)
            {
                field_name = member_expression->getMemberNameInfo().getName().getAsString();
                table_name = get_table_name(declaration_expression->getDecl());
                variable_name = declaration_expression->getNameInfo().getAsString();
                const ValueDecl* decl = declaration_expression->getDecl();

                if (!get_explicit_path_data(decl, explicit_path_data, expression_source_range))
                {
                    variable_name = table_navigation_t::get_variable_name(variable_name, explicit_path_data.tag_table_map);
                    explicit_path_present = false;
                    expression_source_range
                        = SourceRange(
                            member_expression->getBeginLoc(),
                            member_expression->getEndLoc());
                    g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
                }
                else
                {
                    variable_name = get_table_from_expression(explicit_path_data.path_components.back());
                    get_variable_name(variable_name, table_name, explicit_path_data);
                    update_used_dbs(explicit_path_data);
                    expression_source_range.setEnd(member_expression->getEndLoc());
                }

                if (decl->hasAttr<GaiaFieldValueAttr>())
                {
                    if (!explicit_path_present)
                    {
                        expression_source_range
                            = SourceRange(
                                expression_source_range.getBegin().getLocWithOffset(-1), expression_source_range.getEnd());
                    }

                    if (!validate_and_add_active_field(table_name, field_name, true))
                    {
                        return;
                    }
                }
            }
            else
            {
                cerr << "Incorrect base type of generated type." << endl;
                g_is_generation_error = true;
            }
        }
        else
        {
            cerr << "Incorrect matched expression." << endl;
            g_is_generation_error = true;
        }
        if (expression_source_range.isValid())
        {
            g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
            m_rewriter.ReplaceText(expression_source_range, variable_name + "." + field_name + "()");
            g_rewriter_history.push_back({expression_source_range, variable_name + "." + field_name + "()", replace_text});
            auto offset
                = Lexer::MeasureTokenLength(
                      expression_source_range.getEnd(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts())
                + 1;
            if (!explicit_path_present)
            {
                update_expression_used_tables(
                    result.Context,
                    expression,
                    table_name,
                    variable_name,
                    SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(offset)),
                    m_rewriter);
                update_expression_used_tables(
                    result.Context,
                    member_expression,
                    table_name,
                    variable_name,
                    SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(offset)),
                    m_rewriter);
            }
            else
            {
                update_expression_explicit_path_data(
                    result.Context,
                    expression,
                    explicit_path_data,
                    SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(offset)),
                    m_rewriter);
                update_expression_explicit_path_data(
                    result.Context,
                    member_expression,
                    explicit_path_data,
                    SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(offset)),
                    m_rewriter);
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
        validate_table_data();
        if (g_is_generation_error)
        {
            return;
        }
        const auto* op = result.Nodes.getNodeAs<BinaryOperator>("fieldSet");
        if (op == nullptr)
        {
            cerr << "Incorrect matched operator." << endl;
            g_is_generation_error = true;
            return;
        }
        const Expr* operator_expression = op->getLHS();
        if (operator_expression == nullptr)
        {
            cerr << "Incorrect operator expression" << endl;
            g_is_generation_error = true;
            return;
        }
        const auto* left_declaration_expression = dyn_cast<DeclRefExpr>(operator_expression);
        const auto* member_expression = dyn_cast<MemberExpr>(operator_expression);

        explicit_path_data_t explicit_path_data;
        bool explicit_path_present = true;

        string table_name;
        string field_name;
        string variable_name;
        SourceRange set_source_range;
        if (left_declaration_expression == nullptr && member_expression == nullptr)
        {
            cerr << "Incorrect operator expression type." << endl;
            g_is_generation_error = true;
            return;
        }
        if (left_declaration_expression != nullptr)
        {
            const ValueDecl* operator_declaration = left_declaration_expression->getDecl();
            if (operator_declaration->getType()->isStructureType())
            {
                return;
            }
            table_name = get_table_name(operator_declaration);
            field_name = operator_declaration->getName().str();
            variable_name = table_navigation_t::get_variable_name(table_name, unordered_map<string, string>());
            if (!get_explicit_path_data(operator_declaration, explicit_path_data, set_source_range))
            {
                explicit_path_present = false;
                set_source_range.setBegin(left_declaration_expression->getLocation());
                g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
            }
            else
            {
                variable_name = get_table_from_expression(explicit_path_data.path_components.back());
                get_variable_name(variable_name, table_name, explicit_path_data);
                update_used_dbs(explicit_path_data);
            }
        }
        else
        {
            auto* declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
            if (declaration_expression == nullptr)
            {
                cerr << "Incorrect base type of generated type." << endl;
                g_is_generation_error = true;
                return;
            }
                const ValueDecl* decl = declaration_expression->getDecl();
                field_name = member_expression->getMemberNameInfo().getName().getAsString();
                table_name = get_table_name(decl);
                variable_name = declaration_expression->getNameInfo().getAsString();

            if (!get_explicit_path_data(decl, explicit_path_data, set_source_range))
            {
                variable_name = table_navigation_t::get_variable_name(variable_name, explicit_path_data.tag_table_map);
                explicit_path_present = false;
                set_source_range.setBegin(member_expression->getBeginLoc());
                g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
            }
            else
            {
                variable_name = get_table_from_expression(explicit_path_data.path_components.back());
                get_variable_name(variable_name, table_name, explicit_path_data);
                update_used_dbs(explicit_path_data);
            }
        }
        tok::TokenKind token_kind;
        string replacement_text = "[&]() mutable {auto w = " + variable_name + ".writer(); w." + field_name;

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
                g_is_generation_error = true;
                return;
        }

        replacement_text += convert_compound_binary_opcode(op->getOpcode());

        if (left_declaration_expression != nullptr)
        {
            set_source_range.setEnd(Lexer::findLocationAfterToken(
                set_source_range.getBegin(), token_kind, m_rewriter.getSourceMgr(),
                m_rewriter.getLangOpts(), true).getLocWithOffset(-1));
        }
        else
        {
            set_source_range.setEnd(Lexer::findLocationAfterToken(
                member_expression->getExprLoc(), token_kind, m_rewriter.getSourceMgr(),
                m_rewriter.getLangOpts(), true).getLocWithOffset(-1));
        }
        m_rewriter.ReplaceText(set_source_range, replacement_text);
        g_rewriter_history.push_back({set_source_range, replacement_text, replace_text});
        m_rewriter.InsertTextAfterToken(
            op->getEndLoc(), "; w.update_row(); return w." + field_name + ";}()");
        g_rewriter_history.push_back(
            {SourceRange(op->getEndLoc()), "; w.update_row(); return w." + field_name + ";}()",
            insert_text_after_token});

        auto offset = Lexer::MeasureTokenLength(op->getEndLoc(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts()) + 1;
        if (!explicit_path_present)
        {
            update_expression_used_tables(
                result.Context,
                op,
                table_name,
                variable_name,
                SourceRange(set_source_range.getBegin(), op->getEndLoc().getLocWithOffset(offset)),
                m_rewriter);
        }
        else
        {
            update_expression_explicit_path_data(
                result.Context,
                op,
                explicit_path_data,
                SourceRange(set_source_range.getBegin(), op->getEndLoc().getLocWithOffset(offset)),
                m_rewriter);
        }
    }

private:
    string convert_compound_binary_opcode(BinaryOperator::Opcode op_code)
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
            g_is_generation_error = true;
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
        validate_table_data();
        if (g_is_generation_error)
        {
            return;
        }
        const auto* op = result.Nodes.getNodeAs<UnaryOperator>("fieldUnaryOp");
        if (op == nullptr)
        {
            cerr << "Incorrect matched operator." << endl;
            g_is_generation_error = true;
            return;
        }
        const Expr* operator_expression = op->getSubExpr();
        if (operator_expression == nullptr)
        {
            cerr << "Incorrect operator expression." << endl;
            g_is_generation_error = true;
            return;
        }
        const auto* declaration_expression = dyn_cast<DeclRefExpr>(operator_expression);
        const auto* member_expression = dyn_cast<MemberExpr>(operator_expression);

        if (declaration_expression == nullptr && member_expression == nullptr)
        {
            cerr << "Incorrect operator expression type." << endl;
            g_is_generation_error = true;
            return;
        }
        explicit_path_data_t explicit_path_data;
        bool explicit_path_present = true;
        string replace_string;
        string table_name;
        string field_name;
        string variable_name;
        SourceRange operator_source_range;

        if (declaration_expression != nullptr)
        {
            const ValueDecl* operator_declaration = declaration_expression->getDecl();
            if (operator_declaration->getType()->isStructureType())
            {
                return;
            }

            table_name = get_table_name(operator_declaration);
            field_name = operator_declaration->getName().str();
            variable_name = declaration_expression->getNameInfo().getAsString();
            if (!get_explicit_path_data(operator_declaration, explicit_path_data, operator_source_range))
            {
                variable_name = table_navigation_t::get_variable_name(table_name, explicit_path_data.tag_table_map);
                explicit_path_present = false;
                g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
            }
            else
            {
                variable_name = get_table_from_expression(explicit_path_data.path_components.back());
                get_variable_name(variable_name, table_name, explicit_path_data);
                update_used_dbs(explicit_path_data);
            }
        }
        else
        {
            auto* declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
            if (declaration_expression == nullptr)
            {
                cerr << "Incorrect base type of generated type." << endl;
                g_is_generation_error = true;
                return;
            }
            const ValueDecl* operator_declaration = declaration_expression->getDecl();
            field_name = member_expression->getMemberNameInfo().getName().getAsString();
            table_name = get_table_name(operator_declaration);
            variable_name = declaration_expression->getNameInfo().getAsString();
            if (!get_explicit_path_data(operator_declaration, explicit_path_data, operator_source_range))
            {
                variable_name = table_navigation_t::get_variable_name(variable_name, explicit_path_data.tag_table_map);
                explicit_path_present = false;
                g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
            }
            else
            {
                variable_name = get_table_from_expression(explicit_path_data.path_components.back());
                get_variable_name(variable_name, table_name, explicit_path_data);
                update_used_dbs(explicit_path_data);
            }
        }

        if (op->isPostfix())
        {
            if (op->isIncrementOp())
            {
                replace_string
                    = "[&]() mutable {auto t = "
                    + variable_name + "." + field_name + "(); auto w = "
                    + variable_name + ".writer(); w." + field_name + "++; w.update_row(); return t;}()";
            }
            else if (op->isDecrementOp())
            {
                replace_string
                    = "[&]() mutable {auto t =" + variable_name + "." + field_name + "(); auto w = "
                    + variable_name + ".writer(); w." + field_name + "--; w.update_row(); return t;}()";
            }
        }
        else
        {
            if (op->isIncrementOp())
            {
                replace_string
                    = "[&]() mutable {auto w = " + variable_name + ".writer(); ++ w." + field_name
                    + ";w.update_row(); return w." + field_name + ";}()";
            }
            else if (op->isDecrementOp())
            {
                replace_string
                    = "[&]() mutable {auto w = " + variable_name + ".writer(); -- w." + field_name
                    + ";w.update_row(); return w." + field_name + ";}()";
            }
        }
        m_rewriter.ReplaceText(
            SourceRange(op->getBeginLoc().getLocWithOffset(-1), op->getEndLoc().getLocWithOffset(1)),
            replace_string);
        g_rewriter_history.push_back(
                {SourceRange(op->getBeginLoc().getLocWithOffset(-1), op->getEndLoc().getLocWithOffset(1)),
                replace_string, replace_text});
        auto offset = Lexer::MeasureTokenLength(op->getEndLoc(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts()) + 1;

        if (!explicit_path_present)
        {
            update_expression_used_tables(
                result.Context,
                op,
                table_name,
                variable_name,
                SourceRange(op->getBeginLoc().getLocWithOffset(-1), op->getEndLoc().getLocWithOffset(offset)),
                m_rewriter);
        }
        else
        {
            if (op->isPrefix())
            {
                offset += 1;
            }
            update_expression_explicit_path_data(
                result.Context,
                op,
                explicit_path_data,
                SourceRange(op->getBeginLoc().getLocWithOffset(-1), op->getEndLoc().getLocWithOffset(offset)),
                m_rewriter);
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
        if (g_is_generation_error)
        {
            return;
        }

        const auto* rule_declaration = result.Nodes.getNodeAs<FunctionDecl>("ruleDecl");
        const GaiaOnUpdateAttr* update_attribute = rule_declaration->getAttr<GaiaOnUpdateAttr>();
        const GaiaOnInsertAttr* insert_attribute = rule_declaration->getAttr<GaiaOnInsertAttr>();
        const GaiaOnChangeAttr* change_attribute = rule_declaration->getAttr<GaiaOnChangeAttr>();

        generate_rules(m_rewriter);
        if (g_is_generation_error)
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
        g_expression_explicit_path_data.clear();
        g_insert_tables.clear();
        g_update_tables.clear();
        g_active_fields.clear();
        g_attribute_tag_map.clear();
        g_rewriter_history.clear();
        g_nomatch_location.clear();
        g_nomatch_location_map.clear();
        g_variable_declaration_location.clear();
        g_variable_declaration_init_location.clear();
        g_is_rule_prolog_specified = false;
        g_rule_attribute_source_range = SourceRange();
        g_is_rule_context_rule_name_referenced = false;

        if (update_attribute != nullptr)
        {
            g_rule_attribute_source_range = update_attribute->getRange();
            g_is_rule_prolog_specified = true;
            for (const auto& table_iterator : update_attribute->tables())
            {
                string table, field, tag;
                if (parse_attribute(table_iterator, table, field, tag))
                {
                    if (field.empty())
                    {
                        g_update_tables.insert(table);
                    }
                    else
                    {
                        if (!validate_and_add_active_field(table, field))
                        {
                            return;
                        }
                    }

                    if (!tag.empty())
                    {
                        g_attribute_tag_map[tag] = table;
                    }
                }
            }
        }

        if (insert_attribute != nullptr)
        {
            g_is_rule_prolog_specified = true;
            g_rule_attribute_source_range = insert_attribute->getRange();
            for (const auto& table_iterator : insert_attribute->tables())
            {
                string table, field, tag;
                if (parse_attribute(table_iterator, table, field, tag))
                {
                    g_insert_tables.insert(table);
                    if (!tag.empty())
                    {
                        g_attribute_tag_map[tag] = table;
                    }
                }
            }
        }

        if (change_attribute != nullptr)
        {
            g_is_rule_prolog_specified = true;
            g_rule_attribute_source_range = change_attribute->getRange();
            for (const auto& table_iterator : change_attribute->tables())
            {
                string table, field, tag;
                if (parse_attribute(table_iterator, table, field, tag))
                {
                    g_insert_tables.insert(table);
                    if (field.empty())
                    {
                        g_update_tables.insert(table);
                    }
                    else
                    {
                        if (!validate_and_add_active_field(table, field))
                        {
                            return;
                        }
                    }
                    if (!tag.empty())
                    {
                        g_attribute_tag_map[tag] = table;
                    }
                }
            }
        }
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
        if (g_is_generation_error)
        {
            return;
        }
        generate_rules(m_rewriter);
        if (g_is_generation_error)
        {
            return;
        }
        g_current_rule_declaration = nullptr;
        g_expression_explicit_path_data.clear();
        g_active_fields.clear();
        g_insert_tables.clear();
        g_update_tables.clear();
        g_attribute_tag_map.clear();
        g_rewriter_history.clear();
        g_nomatch_location.clear();
        g_nomatch_location_map.clear();
        g_variable_declaration_location.clear();
        g_variable_declaration_init_location.clear();
        g_is_rule_prolog_specified = false;
        g_rule_attribute_source_range = SourceRange();

        const auto* ruleset_declaration = result.Nodes.getNodeAs<RulesetDecl>("rulesetDecl");
        if (ruleset_declaration == nullptr)
        {
            return;
        }
        if (!g_current_ruleset.empty())
        {
            g_generated_subscription_code
                += "\nnamespace " + g_current_ruleset
                + "{\nvoid subscribe_ruleset_" + g_current_ruleset
                + "()\n{\n" + g_current_ruleset_subscription
                + "}\nvoid unsubscribe_ruleset_" + g_current_ruleset
                + "()\n{\n" + g_current_ruleset_unsubscription + "}\n}\n";
        }
        g_current_ruleset = ruleset_declaration->getName().str();

        // Make sure each new ruleset name is unique.
        for (const auto& r : g_rulesets)
        {
            if (r == g_current_ruleset)
            {
                cerr << "Ruleset names must be unique - '"
                    << g_current_ruleset
                    << "' has been found multiple times." << endl;
                g_is_generation_error = true;
                return;
            }
        }

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
                "namespace " + g_current_ruleset
                + "\n{\n} // namespace " + g_current_ruleset + "\n");
            g_rewriter_history.push_back(
                {SourceRange(ruleset_declaration->getBeginLoc(), ruleset_declaration->getEndLoc()),
                "namespace " + g_current_ruleset + "\n{\n} // namespace " + g_current_ruleset + "\n",
                replace_text});
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

            g_rewriter_history.push_back(
                {SourceRange(ruleset_declaration->getBeginLoc(), ruleset_declaration->decls_begin()->getBeginLoc().getLocWithOffset(c_declaration_to_ruleset_offset)),
                "namespace " + g_current_ruleset + "\n{\n", replace_text});
            g_rewriter_history.push_back(
                {SourceRange(ruleset_declaration->getEndLoc()),
                "}// namespace " + g_current_ruleset, replace_text});
        }
    }

private:
    Rewriter& m_rewriter;
};

class variable_declaration_match_handler_t : public MatchFinder::MatchCallback
{
public:
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_is_generation_error)
        {
            return;
        }
        validate_table_data();
        const auto* variable_declaration = result.Nodes.getNodeAs<VarDecl>("varDeclaration");
        const auto* variable_declaration_init = result.Nodes.getNodeAs<VarDecl>("varDeclarationInit");
        if (variable_declaration_init != nullptr)
        {
            g_variable_declaration_init_location.insert(variable_declaration_init->getSourceRange());
        }
        if (variable_declaration == nullptr)
        {
            return;
        }
        const auto variable_name = variable_declaration->getNameAsString();
        if (variable_name != "")
        {
            g_variable_declaration_location[variable_declaration->getSourceRange()] = variable_name;

            if (table_navigation_t::get_table_data().find(variable_name) != table_navigation_t::get_table_data().end())
            {
                cerr << "Local variable declaration '" << variable_name
                     << "' hides database table of the same name." << endl;
                return;
            }

            for (auto table_data : table_navigation_t::get_table_data())
            {
                if (table_data.second.field_data.find(variable_name) != table_data.second.field_data.end())
                {
                    cerr << "Local variable declaration '" << variable_name
                        << "' hides catalog field entity of the same name." << endl;
                    return;
                }
            }
        }
    }
};

class rule_context_rule_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit rule_context_rule_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_is_generation_error)
        {
            return;
        }

        const auto* rule_expression = result.Nodes.getNodeAs<MemberExpr>("rule_name");
        const auto* ruleset_expression = result.Nodes.getNodeAs<MemberExpr>("ruleset_name");
        const auto* event_expression = result.Nodes.getNodeAs<MemberExpr>("event_type");
        const auto* type_expression = result.Nodes.getNodeAs<MemberExpr>("gaia_type");

        if (ruleset_expression != nullptr)
        {
            m_rewriter.ReplaceText(
                SourceRange(ruleset_expression->getBeginLoc(), ruleset_expression->getEndLoc()),
                "\"" + g_current_ruleset + "\"");
            g_rewriter_history.push_back(
                {SourceRange(ruleset_expression->getBeginLoc(), ruleset_expression->getEndLoc()),
                "\"" + g_current_ruleset + "\"", replace_text});
        }

        if (rule_expression != nullptr)
        {
            m_rewriter.ReplaceText(
                SourceRange(rule_expression->getBeginLoc(), rule_expression->getEndLoc()),
                "gaia_rule_name");
            g_is_rule_context_rule_name_referenced = true;
            g_rewriter_history.push_back(
                {SourceRange(rule_expression->getBeginLoc(), rule_expression->getEndLoc()),
                "gaia_rule_name", replace_text});
        }

        if (event_expression != nullptr)
        {
            m_rewriter.ReplaceText(
                SourceRange(event_expression->getBeginLoc(), event_expression->getEndLoc()),
                "context->event_type");
            g_rewriter_history.push_back(
                {SourceRange(event_expression->getBeginLoc(), event_expression->getEndLoc()),
                 "context->event_type", replace_text});
        }

        if (type_expression != nullptr)
        {
            m_rewriter.ReplaceText(
                SourceRange(type_expression->getBeginLoc(), type_expression->getEndLoc()),
                "context->gaia_type");
            g_rewriter_history.push_back(
                {SourceRange(type_expression->getBeginLoc(), type_expression->getEndLoc()),
                "context->gaia_type", replace_text});
        }
    }

private:
    Rewriter& m_rewriter;
};

// AST handler that called when a table or a tag is an argument for a function call.
class table_call_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit table_call_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        validate_table_data();
        if (g_is_generation_error)
        {
            return;
        }
        const auto* expression = result.Nodes.getNodeAs<DeclRefExpr>("tableCall");

        if (expression == nullptr)
        {
            cerr << "Incorrect matched expression." << endl;
            g_is_generation_error = true;
            return;
        }
        string table_name;
        SourceRange expression_source_range;
        explicit_path_data_t explicit_path_data;
        bool explicit_path_present = true;
        string variable_name;
        const ValueDecl* decl = expression->getDecl();
        if (!decl->getType()->isStructureType())
        {
            return;
        }
        table_name = get_table_name(decl);
        variable_name = decl->getNameAsString();
        if (!get_explicit_path_data(decl, explicit_path_data, expression_source_range))
        {
            variable_name = table_navigation_t::get_variable_name(table_name, explicit_path_data.tag_table_map);
            explicit_path_present = false;
            expression_source_range = SourceRange(expression->getLocation(), expression->getEndLoc());
            g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
        }
        else
        {
            variable_name = get_table_from_expression(explicit_path_data.path_components.back());
            get_variable_name(variable_name, table_name, explicit_path_data);
            update_used_dbs(explicit_path_data);
            expression_source_range
                = SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(-1));
        }

        if (decl->hasAttr<GaiaFieldValueAttr>())
        {
            expression_source_range
                = SourceRange(expression_source_range.getBegin().getLocWithOffset(-1), expression_source_range.getEnd());
        }

        if (expression_source_range.isValid())
        {
            if (g_insert_call_locations.find(expression->getBeginLoc().getRawEncoding()) != g_insert_call_locations.end())
            {
                if (explicit_path_present)
                {
                    cerr << "Insert call cannot be used with navigation." << endl;
                    g_is_generation_error = true;
                    return;
                }

                if (table_name == variable_name)
                {
                    cerr << "Insert call cannot be used with tags." << endl;
                    g_is_generation_error = true;
                    return;
                }
                return;
            }
            m_rewriter.ReplaceText(expression_source_range, variable_name);
            g_rewriter_history.push_back({expression_source_range, variable_name, replace_text});

            auto offset
                = Lexer::MeasureTokenLength(expression_source_range.getEnd(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts()) + 1;
            if (explicit_path_present)
            {
                update_expression_explicit_path_data(
                    result.Context,
                    expression,
                    explicit_path_data,
                    SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(offset)),
                    m_rewriter);
            }
            else
            {
                update_expression_used_tables(
                    result.Context,
                    expression,
                    table_name,
                    variable_name,
                    SourceRange(expression_source_range.getBegin(), expression_source_range.getEnd().getLocWithOffset(offset)),
                    m_rewriter);
            }
        }
    }

private:
    Rewriter& m_rewriter;
};

// AST handler that called when a nomatch is used with if.
class if_nomatch_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit if_nomatch_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }

    void run(const MatchFinder::MatchResult& result) override
    {
        const auto* expression = result.Nodes.getNodeAs<IfStmt>("NoMatchIf");
        if (expression != nullptr)
        {
            SourceRange nomatch_location = get_statement_source_range(expression->getNoMatch(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts());
            g_nomatch_location_map[nomatch_location] = expression->getNoMatchLoc();
            g_nomatch_location.emplace_back(nomatch_location);
        }
        else
        {
            cerr << "Incorrect matched expression." << endl;
            g_is_generation_error = true;
        }
    }

private:
    Rewriter& m_rewriter;
};

// AST handler that is called when a while has a declarative expression.
class declarative_while_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit declarative_while_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        const auto* expression = result.Nodes.getNodeAs<WhileStmt>("DeclWhile");
        if (expression != nullptr)
        {
            // Find the SourceRange of the "while" statement.
            SourceLocation while_begin_loc = expression->getBeginLoc();
            auto offset = Lexer::MeasureTokenLength(while_begin_loc, m_rewriter.getSourceMgr(), m_rewriter.getLangOpts());
            SourceRange while_source_range = SourceRange(while_begin_loc, while_begin_loc.getLocWithOffset(offset));

            // Replace the text of the "while" with "if".
            m_rewriter.ReplaceText(while_source_range, c_if_stmt);
        }
        else
        {
            cerr << "Incorrect matched expression." << endl;
            g_is_generation_error = true;
        }
    }

private:
    Rewriter& m_rewriter;
};

// AST handler that is called when Delete function is invoked on a table.
class declarative_delete_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit declarative_delete_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        const auto* expression = result.Nodes.getNodeAs<CXXMemberCallExpr>("DeleteCall");
        if (expression != nullptr)
        {
            m_rewriter.ReplaceText(SourceRange(expression->getExprLoc(), expression->getEndLoc()), "delete_row()");
            g_rewriter_history.push_back({SourceRange(expression->getExprLoc(), expression->getEndLoc()), "delete_row()", replace_text});
        }
        else
        {
            cerr << "Incorrect matched expression." << endl;
            g_is_generation_error = true;
        }
    }

private:
    Rewriter& m_rewriter;
};

// AST handler that is called when Insert function is invoked on a table.
class declarative_insert_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit declarative_insert_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        const auto* expression = result.Nodes.getNodeAs<CXXMemberCallExpr>("InsertCall");
        const auto* expression_declaration = result.Nodes.getNodeAs<DeclRefExpr>("tableCall");
        if (expression == nullptr || expression_declaration == nullptr)
        {
            cerr << "Incorrect matched expression." << endl;
            g_is_generation_error = true;
            return;
        }
        SourceLocation argument_start_location = expression->getExprLoc();
        unordered_map<string, string> argument_map;
        const ValueDecl* decl = expression_declaration->getDecl();
        string table_name = get_table_name(decl);

        // Parse insert call arguments to buid name value map.
        for (auto argument : expression->arguments())
        {
            string raw_argument_name =
                m_rewriter.getRewrittenText(
                    SourceRange(argument_start_location,
                        argument->getSourceRange().getBegin().getLocWithOffset(-1)));
            size_t argument_name_start_position = raw_argument_name.find(',');
            if (argument_name_start_position == string::npos)
            {
                argument_name_start_position = raw_argument_name.find('(');
            }
            size_t argument_name_end_position = raw_argument_name.find(':');
            string argument_name = raw_argument_name.substr(
                argument_name_start_position + 1, argument_name_end_position - argument_name_start_position -1);
            //Trim the argument name of whitespaces.
            argument_name.erase(argument_name.begin(), find_if(argument_name.begin(), argument_name.end(),
                [](unsigned char ch) { return !isspace(ch); }));
            argument_name.erase(find_if(argument_name.rbegin(), argument_name.rend(),
                [](unsigned char ch) { return !isspace(ch); }).base(), argument_name.end());
            argument_map[argument_name] = m_rewriter.getRewrittenText(argument->getSourceRange());

            argument_start_location = argument->getSourceRange().getEnd().getLocWithOffset(1);
        }
        string class_qualification_string = "gaia::";
        class_qualification_string
            .append(table_navigation_t::get_table_data().find(table_name)->second.db_name)
            .append("::")
            .append(table_name)
            .append("_t::");
        string replacement_string = class_qualification_string;
        replacement_string
            .append("get(")
            .append(class_qualification_string)
            .append("insert_row(");
        vector<string> function_arguments = table_navigation_t::get_table_fields(table_name);
        const auto table_data_iterator = table_navigation_t::get_table_data().find(table_name);
        // Generate call arguments.
        for (const auto& call_argument : function_arguments)
        {
            const auto argument_map_iterator = argument_map.find(call_argument);
            if (argument_map_iterator == argument_map.end())
            {
                // Provide default parameter value.
                const auto field_data_iterator = table_data_iterator->second.field_data.find(call_argument);
                switch (static_cast<data_type_t>(field_data_iterator->second.field_type))
                {
                case data_type_t::e_bool:
                    replacement_string.append("false,");
                    break;
                case data_type_t::e_int8:
                case data_type_t::e_uint8:
                case data_type_t::e_int16:
                case data_type_t::e_uint16:
                case data_type_t::e_int32:
                case data_type_t::e_uint32:
                case data_type_t::e_int64:
                case data_type_t::e_uint64:
                case data_type_t::e_float:
                case data_type_t::e_double:
                    replacement_string.append("0,");
                    break;
                case data_type_t::e_string:
                    replacement_string.append("\"\",");
                    break;
                }
            }
            else
            {
                // Provide value from the code.
                replacement_string.append(argument_map_iterator->second).append(",");
            }
        }
        replacement_string.resize(replacement_string.size() - 1);
        replacement_string.append("))");

        m_rewriter.ReplaceText(SourceRange(expression->getBeginLoc(), expression->getEndLoc()), replacement_string);
        g_rewriter_history.push_back({SourceRange(expression->getBeginLoc(), expression->getEndLoc()), replacement_string, replace_text});
        g_insert_call_locations.insert(expression->getBeginLoc().getRawEncoding());
    }

private:
    Rewriter& m_rewriter;
};

// AST handler that is called when a declarative for.
class declarative_for_match_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit declarative_for_match_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        const auto* expression = result.Nodes.getNodeAs<GaiaForStmt>("DeclFor");
        if (expression == nullptr)
        {
            cerr << "Incorrect matched expression." << endl;
            g_is_generation_error = true;
        }

        string table_name;
        string variable_name;
        SourceRange expression_source_range;
        explicit_path_data_t explicit_path_data;
        bool explicit_path_present = true;
        const auto* path = static_cast<const DeclRefExpr*>(expression->getPath());

        const ValueDecl* decl = path->getDecl();
        table_name = get_table_name(decl);

        if (!get_explicit_path_data(decl, explicit_path_data, expression_source_range))
        {
            variable_name = table_navigation_t::get_variable_name(table_name, explicit_path_data.tag_table_map);
            g_used_dbs.insert(table_navigation_t::get_table_data().find(table_name)->second.db_name);
            explicit_path_present = false;
            expression_source_range.setBegin(expression->getLParenLoc().getLocWithOffset(1));
        }
        else
        {
            variable_name = get_table_from_expression(explicit_path_data.path_components.back());
            get_variable_name(variable_name, table_name, explicit_path_data);
            update_used_dbs(explicit_path_data);
        }
        expression_source_range.setEnd(expression->getRParenLoc().getLocWithOffset(-1));

        if (expression_source_range.isValid())
        {
            if (explicit_path_present)
            {
                update_expression_explicit_path_data(
                    result.Context,
                    path,
                    explicit_path_data,
                    expression_source_range,
                    m_rewriter);
            }
            else
            {
                update_expression_used_tables(
                    result.Context,
                    path,
                    table_name,
                    variable_name,
                    expression_source_range,
                    m_rewriter);
            }
        }
        m_rewriter.RemoveText(SourceRange(expression->getForLoc(), expression->getRParenLoc()));
        g_rewriter_history.push_back({SourceRange(expression->getForLoc(), expression->getRParenLoc()), "", remove_text});
    }

private:
    Rewriter& m_rewriter;
};


class translation_engine_consumer_t : public clang::ASTConsumer
{
public:
    explicit translation_engine_consumer_t(ASTContext*, Rewriter& r)
        : m_field_get_match_handler(r)
        , m_field_set_match_handler(r)
        , m_rule_match_handler(r)
        , m_ruleset_match_handler(r)
        , m_field_unary_operator_match_handler(r)
        , m_rule_context_match_handler(r)
        , m_table_call_match_handler(r)
        , m_if_nomatch_match_handler(r)
        , m_declarative_while_match_handler(r)
        , m_declarative_for_match_handler(r)
        , m_declarative_delete_handler(r)
        , m_declarative_insert_handler(r)
    {
        DeclarationMatcher ruleset_matcher = rulesetDecl().bind("rulesetDecl");
        DeclarationMatcher rule_matcher
            = functionDecl(allOf(
                               hasAncestor(ruleset_matcher),
                               hasAttr(attr::Rule)))
                  .bind("ruleDecl");
        StatementMatcher ruleset_name_matcher
            = memberExpr(
                  hasAncestor(ruleset_matcher),
                  hasDescendant(gaiaRuleContextExpr()),
                  member(hasName("ruleset_name")))
                  .bind("ruleset_name");
        StatementMatcher rule_name_matcher
            = memberExpr(
                  hasAncestor(ruleset_matcher),
                  hasDescendant(gaiaRuleContextExpr()),
                  member(hasName("rule_name")))
                  .bind("rule_name");
        StatementMatcher event_type_matcher
            = memberExpr(
                  hasAncestor(ruleset_matcher),
                  hasDescendant(gaiaRuleContextExpr()),
                  member(hasName("event_type")))
                  .bind("event_type");
        StatementMatcher gaia_type_matcher
            = memberExpr(
                  hasAncestor(ruleset_matcher),
                  hasDescendant(gaiaRuleContextExpr()),
                  member(hasName("gaia_type")))
                  .bind("gaia_type");

        DeclarationMatcher variable_declaration_matcher = varDecl(hasAncestor(rule_matcher)).bind("varDeclaration");

        StatementMatcher field_get_matcher
            = declRefExpr(to(varDecl(
                              anyOf(
                                  hasAttr(attr::GaiaField),
                                  hasAttr(attr::FieldTable),
                                  hasAttr(attr::GaiaFieldValue)),
                              unless(hasAttr(attr::GaiaFieldLValue)))))
                  .bind("fieldGet");
        StatementMatcher table_call_matcher
            = declRefExpr(allOf(
                              to(varDecl(
                                  anyOf(
                                      hasAttr(attr::GaiaField),
                                      hasAttr(attr::FieldTable),
                                      hasAttr(attr::GaiaFieldValue)),
                                  unless(hasAttr(attr::GaiaFieldLValue)))),
                              allOf(
                                  unless(
                                      hasAncestor(
                                          memberExpr(
                                              member(
                                                  allOf(
                                                      hasAttr(attr::GaiaField), unless(hasAttr(attr::GaiaFieldLValue))))))),
                                  anyOf(
                                      hasAncestor(callExpr()), hasAncestor(cxxMemberCallExpr())))))
                  .bind("tableCall");

        StatementMatcher field_set_matcher
            = binaryOperator(
                  allOf(
                      hasAncestor(ruleset_matcher),
                      isAssignmentOperator(),
                      hasLHS(declRefExpr(to(varDecl(hasAttr(attr::GaiaFieldLValue)))))))
                  .bind("fieldSet");
        StatementMatcher field_unary_operator_matcher
            = unaryOperator(
                  allOf(
                      hasAncestor(ruleset_matcher),
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
                          hasAttr(attr::GaiaFieldValue)))))))
                  .bind("tableFieldGet");
        StatementMatcher table_field_set_matcher
            = binaryOperator(
                  allOf(
                      hasAncestor(ruleset_matcher),
                      isAssignmentOperator(),
                      hasLHS(
                          memberExpr(
                              member(hasAttr(attr::GaiaFieldLValue))))))
                  .bind("fieldSet");
        StatementMatcher table_field_unary_operator_matcher
            = unaryOperator(allOf(
                                hasAncestor(ruleset_matcher),
                                anyOf(
                                    hasOperatorName("++"),
                                    hasOperatorName("--")),
                                hasUnaryOperand(memberExpr(member(hasAttr(attr::GaiaFieldLValue))))))
                  .bind("fieldUnaryOp");

        StatementMatcher if_no_match_matcher
            = ifStmt(allOf(
                         hasAncestor(rule_matcher),
                         hasNoMatch(anything())))
                  .bind("NoMatchIf");

        StatementMatcher declarative_while_matcher
            = whileStmt(allOf(
                            hasAncestor(rule_matcher),
                            hasCondition(anyOf(
                                hasDescendant(field_get_matcher),
                                hasDescendant(field_unary_operator_matcher),
                                hasDescendant(table_field_get_matcher),
                                hasDescendant(table_field_unary_operator_matcher)))))
                  .bind("DeclWhile");

        StatementMatcher declarative_for_matcher
            = gaiaForStmt().bind("DeclFor");

        DeclarationMatcher variable_declaration_init_matcher
            = varDecl(allOf(
                          hasAncestor(rule_matcher),
                          hasInitializer(anyOf(
                              hasDescendant(field_get_matcher),
                              hasDescendant(field_unary_operator_matcher),
                              hasDescendant(table_field_get_matcher),
                              hasDescendant(table_field_unary_operator_matcher)))))
                  .bind("varDeclarationInit");

        StatementMatcher declarative_delete_matcher
            = cxxMemberCallExpr(
                hasAncestor(ruleset_matcher),
                callee(cxxMethodDecl(hasName("Delete"))),
                hasDescendant(table_call_matcher)
                ).bind("DeleteCall");

        StatementMatcher declarative_insert_matcher
            = cxxMemberCallExpr(
                hasAncestor(ruleset_matcher),
                callee(cxxMethodDecl(hasName("Insert"))),
                hasDescendant(table_call_matcher)
                ).bind("InsertCall");

        m_matcher.addMatcher(field_get_matcher, &m_field_get_match_handler);
        m_matcher.addMatcher(table_field_get_matcher, &m_field_get_match_handler);

        m_matcher.addMatcher(field_set_matcher, &m_field_set_match_handler);
        m_matcher.addMatcher(table_field_set_matcher, &m_field_set_match_handler);

        m_matcher.addMatcher(rule_matcher, &m_rule_match_handler);
        m_matcher.addMatcher(ruleset_matcher, &m_ruleset_match_handler);
        m_matcher.addMatcher(field_unary_operator_matcher, &m_field_unary_operator_match_handler);

        m_matcher.addMatcher(table_field_unary_operator_matcher, &m_field_unary_operator_match_handler);

        m_matcher.addMatcher(variable_declaration_matcher, &m_variable_declaration_match_handler);
        m_matcher.addMatcher(variable_declaration_init_matcher, &m_variable_declaration_match_handler);
        m_matcher.addMatcher(ruleset_name_matcher, &m_rule_context_match_handler);
        m_matcher.addMatcher(rule_name_matcher, &m_rule_context_match_handler);
        m_matcher.addMatcher(event_type_matcher, &m_rule_context_match_handler);
        m_matcher.addMatcher(gaia_type_matcher, &m_rule_context_match_handler);
        m_matcher.addMatcher(table_call_matcher, &m_table_call_match_handler);
        m_matcher.addMatcher(if_no_match_matcher, &m_if_nomatch_match_handler);
        m_matcher.addMatcher(declarative_while_matcher, &m_declarative_while_match_handler);
        m_matcher.addMatcher(declarative_for_matcher, &m_declarative_for_match_handler);
        m_matcher.addMatcher(declarative_delete_matcher, &m_declarative_delete_handler);
        m_matcher.addMatcher(declarative_insert_matcher, &m_declarative_insert_handler);
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
    variable_declaration_match_handler_t m_variable_declaration_match_handler;
    rule_context_rule_match_handler_t m_rule_context_match_handler;
    table_call_match_handler_t m_table_call_match_handler;
    if_nomatch_match_handler_t m_if_nomatch_match_handler;
    declarative_while_match_handler_t m_declarative_while_match_handler;
    declarative_for_match_handler_t m_declarative_for_match_handler;
    declarative_delete_handler_t m_declarative_delete_handler;
    declarative_insert_handler_t m_declarative_insert_handler;
};

class translation_engine_action_t : public clang::ASTFrontendAction
{
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& compiler, llvm::StringRef) override
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
        if (g_is_generation_error)
        {
            return;
        }
        g_generated_subscription_code
            += "namespace " + g_current_ruleset
            + "{\nvoid subscribe_ruleset_" + g_current_ruleset + "()\n{\n" + g_current_ruleset_subscription + "}\n"
            + "void unsubscribe_ruleset_" + g_current_ruleset + "()\n{\n" + g_current_ruleset_unsubscription + "}\n"
            + "} // namespace " + g_current_ruleset + "\n"
            + generate_general_subscription_code();

        if (!shouldEraseOutputFiles() && !g_is_generation_error && !g_translation_engine_output_option.empty())
        {
            std::error_code error_code;
            llvm::raw_fd_ostream output_file(g_translation_engine_output_option, error_code, llvm::sys::fs::F_None);

            if (!output_file.has_error())
            {
                output_file << "#include <cstring>\n";
                output_file << "\n";
                output_file << "#include \"gaia/common.hpp\"\n";
                output_file << "#include \"gaia/events.hpp\"\n";
                output_file << "#include \"gaia/rules/rules.hpp\"\n";
                output_file << "\n";
                for (const string& db : g_used_dbs)
                {
                    output_file << "#include \"gaia_" << db << ".h\"\n";
                }

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
    cl::opt<std::string> instance_name(
        "n", cl::desc("DB instance name"), cl::Optional,
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

    if (!instance_name.empty())
    {
        gaia::db::config::session_options_t session_options = gaia::db::config::get_default_session_options();
        session_options.db_instance_name = instance_name.getValue();
        gaia::db::config::set_default_session_options(session_options);
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
    if (result == 0 && !g_is_generation_error)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}
