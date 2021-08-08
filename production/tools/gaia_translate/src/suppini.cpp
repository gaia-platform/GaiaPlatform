//
// Created by simone on 8/8/21.
//
#include <iostream>
#include <string>
#include <unordered_map>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#pragma clang diagnostic pop

#include "gaiat_state.hpp"

void fn(
    clang::CXXOperatorCallExpr* a,
    clang::ASTContext* b,
    clang::Decl* c,
    clang::Stmt* f,
    clang::LangOptions* g,
    clang::SourceLocation h,
    clang::Lexer* i,
    clang::Rewriter* j,
    llvm::raw_ostream* k)
{
    a->getBeginLoc();
    b->getTranslationUnitDecl();
    c->getBeginLoc();
    f->dump();
    g->trackLocalOwningModule();
    h.isFileID();
    i->getSourceLocation();
    j->getSourceMgr();
    k->GetBufferSize();
}

using namespace clang;
using namespace std;
using namespace gaia::translation;

namespace gaia
{
namespace translation1
{

static const char c_ident[] = "    ";
// Suppress these clang-tidy warnings for now.
static const char c_nolint_identifier_naming[] = "// NOLINTNEXTLINE(readability-identifier-naming)";

void print_version(raw_ostream& stream)
{
    stream << "Gaia Translation Engine \nCopyright (c) Gaia Platform LLC\n";
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

SourceRange get_statement_source_range(const Stmt* expression, const SourceManager& source_manager, const LangOptions& options)
{
    if (expression == nullptr)
    {
        return SourceRange();
    }
    SourceRange return_value = expression->getSourceRange();
    if (dyn_cast<CompoundStmt>(expression) == nullptr)
    {
        SourceLocation end_location = Lexer::findLocationAfterToken(return_value.getEnd(), tok::semi, source_manager, options, true);
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
void optimize_path(vector<explicit_path_data_t>& path, explicit_path_data_t& path_segment)
{
    string first_table = get_table_from_expression(path_segment.path_components.front());
    for (auto& path_iterator : path)
    {
        if (is_tag_defined(path_iterator.defined_tags, first_table))
        {
            path_segment.skip_implicit_path_generation = true;
            return;
        }
        if (is_tag_defined(path_segment.defined_tags, get_table_from_expression(path_iterator.path_components.front())))
        {
            path_iterator.skip_implicit_path_generation = true;
        }
    }
}
bool is_path_segment_contained_in_another_path(const vector<explicit_path_data_t>& path, const explicit_path_data_t& path_segment)
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
bool validate_and_add_active_field(const string& table_name, const string& field_name, bool is_active_from_field)
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
        SourceRange nomatch_range;
        // Find correct nomatch segment for a current declarative expression.
        // This segment will be used later to inject nomatch code at
        // the end of navigation code wrapping the declarative expression.
        for (const auto& nomatch_source_range : g_nomatch_location)
        {
            if (explicit_path_data_iterator.first.getEnd() == nomatch_source_range.getEnd())
            {
                nomatch_range = nomatch_source_range;
                break;
            }
        }
        const auto break_label_iterator = g_break_label_map.find(explicit_path_data_iterator.first);
        const auto continue_label_iterator = g_continue_label_map.find(explicit_path_data_iterator.first);
        string break_label;
        string continue_label;
        if (break_label_iterator != g_break_label_map.end())
        {
            break_label = break_label_iterator->second;
        }
        if (continue_label_iterator != g_continue_label_map.end())
        {
            continue_label = continue_label_iterator->second;
        }
        for (const auto& data_iterator : explicit_path_data_iterator.second)
        {
            string anchor_table_name = get_table_name(
                get_table_from_expression(anchor_table), data_iterator.tag_table_map);

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
                            auto declaration_source_range_size = variable_declaration_range_iterator.first.getEnd().getRawEncoding() - variable_declaration_range_iterator.first.getBegin().getRawEncoding();
                            auto min_declaration_source_range_size = variable_declaration_range.getEnd().getRawEncoding() - variable_declaration_range.getBegin().getRawEncoding();
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

            // The following code
            // l1:if (x.value>5)
            //  break l1;
            // else
            //  continue l1;
            // .............
            // is translated into
            // ..........Navigation code for x
            // l1:if (x.value()>5)
            //  goto l1_break;
            // else
            //  goto l1_continue;
            // ..........................
            // l1_continue:
            //...............Navigation code for x
            // l1_break:

            if (!break_label.empty())
            {
                navigation_code.postfix += "\n" + break_label + ":\n";
            }
            if (!continue_label.empty())
            {
                navigation_code.postfix = "\n" + continue_label + ":\n" + navigation_code.postfix;
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
                rewriter.InsertTextAfterToken(
                    explicit_path_data_iterator.first.getEnd(),
                    navigation_code.postfix);
            }
        }
    }
}
void generate_table_subscription(const string& table, const string& field_subscription_code, int rule_count, bool subscribe_update, unordered_map<uint32_t, string>& rule_line_numbers, Rewriter& rewriter)
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
                string first_component_table = get_table_from_expression(data_iterator.path_components.front());
                if (data_iterator.tag_table_map.find(first_component_table) == data_iterator.tag_table_map.end() || is_tag_defined(data_iterator.defined_tags, first_component_table) || g_attribute_tag_map.find(first_component_table) != g_attribute_tag_map.end())
                {
                    is_absoute_path_only = false;
                    break;
                }
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
bool has_multiple_anchors()
{
    static const char* c_multi_anchor_tables = "Multiple anchor rows: A rule may not specify multiple tables or active fields from different tables in "
                                               "'on_insert', 'on_change', or 'on_update'.";
    static const char* c_multi_anchor_fields = "Multiple anchor rows: A rule may not specify active fields "
                                               "from different tables.";

    if (g_insert_tables.size() > 1 || g_update_tables.size() > 1)
    {
        cerr << c_multi_anchor_tables << endl;
        return true;
    }

    if (g_active_fields.size() > 1)
    {
        cerr << c_multi_anchor_fields << endl;
        return true;
    }

    // Handle the special case of on_update(table1, table2.field)
    if (g_active_fields.size() == 1
        && g_update_tables.size() == 1
        && g_active_fields.find(*(g_update_tables.begin())) == g_active_fields.end())
    {
        cerr << c_multi_anchor_tables << endl;
        return true;
    }

    return false;
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

    if (has_multiple_anchors())
    {
        g_is_generation_error = true;
        return;
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
                update_expression_location(
                    return_value, expression->getSourceRange().getBegin(), expression->getSourceRange().getEnd());
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
                SourceRange for_source_range = expression->getSourceRange();
                SourceRange nomatch_source_range = get_statement_source_range(expression->getNoMatch(), rewriter.getSourceMgr(), rewriter.getLangOpts());

                if (nomatch_source_range.isValid())
                {
                    for_source_range.setEnd(nomatch_source_range.getEnd());
                }
                update_expression_location(
                    return_value, for_source_range.getBegin(), for_source_range.getEnd());
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
bool should_expression_location_be_merged(ASTContext* context, const Stmt& node, bool special_parent)
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
            //            return should_expression_location_be_merged(context, *(node_parents[0].get<DeclStmt>()));
        }
        else if (const auto* expression = node_parents_iterator.get<Stmt>())
        {
            return should_expression_location_be_merged(context, *expression, special_parent);
        }
    }
    return true;
}
void update_expression_explicit_path_data(ASTContext* context, const Stmt* node, explicit_path_data_t data, const SourceRange& source_range, Rewriter& rewriter)
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
                if (expression_explicit_path_data_iterator.first == expression_source_range
                    //                || should_expression_location_be_merged(context, *node)
                )
                {
                    expression_explicit_path_data_iterator.second.push_back(data);
                    return;
                }
                optimize_path(expression_explicit_path_data_iterator.second, data);
            }
            else
            {
                string first_component = get_table_from_expression(data.path_components.front());
                if (data.tag_table_map.find(first_component) != data.tag_table_map.end())
                {
                    data.skip_implicit_path_generation = true;
                }
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
void update_expression_used_tables(ASTContext* context, const Stmt* node, const string& table, const string& variable_name, const SourceRange& source_range, Rewriter& rewriter)
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

} // namespace translation1
} // namespace gaia
