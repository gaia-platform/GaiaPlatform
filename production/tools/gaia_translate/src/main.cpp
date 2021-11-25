/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/DiagnosticSema.h>
#include <clang/Driver/Options.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/Twine.h>
#pragma clang diagnostic pop

#include "gaia_internal/common/gaia_version.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/db/db_client_config.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

#include "diagnostics.h"
#include "table_navigation.h"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;
using namespace clang::ast_matchers;
using namespace ::gaia;
using namespace ::gaia::common;
using namespace ::gaia::translation;
using namespace clang::gaia::catalog;

cl::OptionCategory g_translation_engine_category("Translation engine options");

cl::opt<string> g_translation_engine_output_option("output", cl::desc("output file name"), cl::init(""), cl::cat(g_translation_engine_category));

cl::alias g_translation_engine_output_option_alias("o", cl::desc("Alias for -output"), cl::aliasopt(g_translation_engine_output_option), cl::NotHidden, cl::cat(g_translation_engine_category));

// An alias cannot be made for the -help option,
// so instead this cl::opt pretends to be the cl::alias for -help.
cl::opt<bool> g_help_option_alias("h", cl::desc("Alias for -help"), cl::ValueDisallowed, cl::cat(g_translation_engine_category));

// This should be "Required" instead of "ZeroOrMore", but its error message is not user-friendly:
// "gaiat: Not enough positional command line arguments specified! Must specify at least 1 positional argument: See: ./gaiat -help"
// Single-option statements like gaiat -h would print that error because the "Required" positional argument is missing.
// The number of source files is enforced manually instead of using llvm::cl parameters.
cl::list<std::string> g_source_files(cl::Positional, cl::desc("<sourceFile>"), cl::ZeroOrMore, cl::cat(g_translation_engine_category));

cl::opt<std::string> g_instance_name("n", cl::desc("DB instance name"), cl::Optional, cl::Hidden, cl::cat(g_translation_engine_category));

std::string g_current_ruleset;
std::string g_current_ruleset_serial_group;
bool g_is_generation_error = false;
int g_current_ruleset_rule_number = 1;
unsigned int g_current_ruleset_rule_line_number = 1;
constexpr int c_declaration_to_ruleset_offset = -2;
bool g_is_rule_context_rule_name_referenced = false;
SourceRange g_rule_attribute_source_range;
bool g_is_rule_prolog_specified = false;

constexpr char c_connect_keyword[] = "connect";
constexpr char c_disconnect_keyword[] = "disconnect";

llvm::SmallVector<string, 8> g_rulesets;
llvm::StringMap<llvm::StringSet<>> g_active_fields;
llvm::StringSet<> g_insert_tables;
llvm::StringSet<> g_update_tables;
llvm::StringMap<string> g_attribute_tag_map;

llvm::DenseSet<SourceLocation> g_insert_call_locations;

llvm::DenseMap<SourceRange, llvm::SmallVector<explicit_path_data_t, 8>> g_expression_explicit_path_data;

llvm::StringSet<> g_used_dbs;

const FunctionDecl* g_current_rule_declaration = nullptr;

llvm::SmallString<256> g_current_ruleset_subscription;
llvm::SmallString<256> g_generated_subscription_code;
llvm::SmallString<256> g_current_ruleset_unsubscription;

// We use this to report the rule location when reporting diagnostics.
// In the best caese, we'll update the location to report the exact
// line number and column for an error.  In some cases, however, we
// don't have this information so the least we can do here is point to
// the rule where the error occurred.  Note also that errors can occur
// when the translation engine starts generating code.  Code generation
// for rule N happens when we encounter rule N+1 or the end of file
// for the last rule.  So we need to save off this context.
SourceLocation g_last_rule_location;

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

// Data structure to hold data for code generation for insert function calls.
struct insert_data_t
{
    SourceRange expression_range;
    string table_name;
    llvm::StringMap<SourceRange> argument_map;
    llvm::DenseMap<SourceRange, string> argument_replacement_map;
    llvm::StringSet<> argument_table_names;
};

// Vector to contain all the data to properly generate code for insert function call.
// The generation deferred to allow proper code generation for declarative references as arguments for insert call.
llvm::SmallVector<insert_data_t, 8> g_insert_data;
llvm::SmallVector<rewriter_history_t, 8> g_rewriter_history;
llvm::DenseMap<SourceRange, string> g_variable_declaration_location;
llvm::DenseSet<SourceRange> g_variable_declaration_init_location;
llvm::SmallVector<SourceRange, 8> g_nomatch_location_list;
llvm::DenseMap<SourceRange, string> g_break_label_map;
llvm::DenseMap<SourceRange, string> g_continue_label_map;

// Suppress these clang-tidy warnings for now.
static const char c_nolint_identifier_naming[] = "// NOLINTNEXTLINE(readability-identifier-naming)";
static const char c_ident[] = "    ";

static void print_version(raw_ostream& stream)
{
    // Note that the clang::raw_ostream does not support 'endl'.
    stream << c_gaiat << " " << gaia_full_version() << "\n";
    stream << c_copyright << "\n";
}

// Get location of a token before the current location.
SourceLocation get_previous_token_location(SourceLocation location, const Rewriter& rewriter)
{
    Token token;
    token.setLocation(SourceLocation());
    token.setKind(tok::unknown);
    location = location.getLocWithOffset(-1);
    auto start_of_file = rewriter.getSourceMgr().getLocForStartOfFile(rewriter.getSourceMgr().getFileID(location));
    while (location != start_of_file)
    {
        location = Lexer::GetBeginningOfToken(location, rewriter.getSourceMgr(), rewriter.getLangOpts());
        if (!Lexer::getRawToken(location, token, rewriter.getSourceMgr(), rewriter.getLangOpts()) && token.isNot(tok::comment))
        {
            break;
        }
        location = location.getLocWithOffset(-1);
    }
    return token.getLocation();
}

SourceRange get_statement_source_range(const Stmt* expression, const SourceManager& source_manager, const LangOptions& options, bool is_nomatch = false)
{
    if (expression == nullptr)
    {
        return SourceRange();
    }
    SourceRange return_value = expression->getSourceRange();
    if (dyn_cast<CompoundStmt>(expression) == nullptr || is_nomatch)
    {
        SourceLocation end_location = Lexer::findLocationAfterToken(return_value.getEnd(), tok::semi, source_manager, options, true);
        if (end_location.isValid())
        {
            return_value.setEnd(end_location.getLocWithOffset(-1));
        }
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
    const auto& defined_tag_iterator = explicit_path_data.defined_tags.find(variable_name);
    if (defined_tag_iterator != explicit_path_data.defined_tags.end())
    {
        variable_name = defined_tag_iterator->second;
    }
    if (table_name != variable_name)
    {
        explicit_path_data.tag_table_map[variable_name] = table_name;
    }

    if (explicit_path_data.path_components.size() > 1 && table_name == variable_name)
    {
        variable_name = table_navigation_t::get_variable_name(table_name, explicit_path_data.tag_table_map);
    }

    if (explicit_path_data.tag_table_map.find(variable_name) == explicit_path_data.tag_table_map.end())
    {
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

StringRef get_table_from_expression(StringRef expression)
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

bool is_tag_defined(const llvm::StringMap<string>& tag_map, StringRef tag)
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

bool can_path_be_optimized(StringRef path_first_component, const llvm::SmallVector<explicit_path_data_t, 8>& path_data)
{
    for (const auto& path_iterator : path_data)
    {
        if (is_tag_defined(path_iterator.defined_tags, path_first_component))
        {
            return true;
        }

        if (path_iterator.tag_table_map.find(path_first_component) != path_iterator.tag_table_map.end())
        {
            return true;
        }
    }

    return false;
}

void optimize_path(llvm::SmallVector<explicit_path_data_t, 8>& path, explicit_path_data_t& path_segment, bool is_explicit_path_data_stored)
{
    StringRef first_table = get_table_from_expression(path_segment.path_components.front());
    const auto& tag_iterator = path_segment.tag_table_map.find(first_table);

    if (tag_iterator != path_segment.tag_table_map.end()
        && (tag_iterator->second != tag_iterator->first()
            || (is_explicit_path_data_stored && can_path_be_optimized(first_table, path))))
    {
        path_segment.skip_implicit_path_generation = true;
    }

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

bool is_path_segment_contained_in_another_path(
    const llvm::SmallVector<explicit_path_data_t, 8>& path,
    const explicit_path_data_t& path_segment)
{
    llvm::StringSet<> tag_container, table_container;

    if (!path_segment.defined_tags.empty())
    {
        return false;
    }

    for (const auto& path_iterator : path)
    {
        for (const auto& table_iterator : path_iterator.path_components)
        {
            StringRef table_name = get_table_from_expression(table_iterator);
            const auto& tag_iterator = path_iterator.defined_tags.find(table_name);
            if (tag_iterator != path_iterator.defined_tags.end())
            {
                table_name = tag_iterator->second;
            }
            table_container.insert(table_name);
        }
        for (const auto& tag_iterator : path_iterator.tag_table_map)
        {
            tag_container.insert(tag_iterator.first());
        }
    }

    for (const auto& tag_map_iterator : path_segment.tag_table_map)
    {
        if (tag_container.find(tag_map_iterator.first()) == tag_container.end())
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
    if (GaiaCatalog::getCatalogTableData().empty())
    {
        g_is_generation_error = true;
        return;
    }
}

llvm::SmallString<256> generate_general_subscription_code()
{
    llvm::SmallString<256> return_value = StringRef(
        "namespace gaia\n"
        "{\n"
        "namespace rules\n"
        "{\n"
        "extern \"C\" void subscribe_ruleset(const char* ruleset_name)\n"
        "{\n");

    for (const string& ruleset : g_rulesets)
    {
        return_value.append(c_ident);
        return_value.append("if (strcmp(ruleset_name, \"");
        return_value.append(ruleset);
        return_value.append("\") == 0)\n");
        return_value.append(c_ident);
        return_value.append("{\n");
        return_value.append(c_ident);
        return_value.append(c_ident);
        return_value.append("::");
        return_value.append(ruleset);
        return_value.append("::subscribe_ruleset_");
        return_value.append(ruleset);
        return_value.append("();\n");
        return_value.append(c_ident);
        return_value.append(c_ident);
        return_value.append("return;\n");
        return_value.append(c_ident);
        return_value.append("}\n");
    }

    return_value.append(c_ident);
    return_value.append("throw gaia::rules::ruleset_not_found(ruleset_name);\n"
                        "}\n"
                        "extern \"C\" void unsubscribe_ruleset(const char* ruleset_name)\n"
                        "{\n");

    for (const string& ruleset : g_rulesets)
    {
        return_value.append(c_ident);
        return_value.append("if (strcmp(ruleset_name, \"");
        return_value.append(ruleset);
        return_value.append("\") == 0)\n");
        return_value.append(c_ident);
        return_value.append("{\n");
        return_value.append(c_ident);
        return_value.append(c_ident);
        return_value.append("::");
        return_value.append(ruleset);
        return_value.append("::unsubscribe_ruleset_");
        return_value.append(ruleset);
        return_value.append("();\n");
        return_value.append(c_ident);
        return_value.append(c_ident);
        return_value.append("return;\n");
        return_value.append(c_ident);
        return_value.append("}\n");
    }

    return_value.append(c_ident);
    return_value.append("throw ruleset_not_found(ruleset_name);\n"
                        "}\n"
                        "extern \"C\" void initialize_rules()\n"
                        "{\n");

    for (const string& ruleset : g_rulesets)
    {
        return_value.append(c_ident);
        return_value.append("::");
        return_value.append(ruleset);
        return_value.append("::subscribe_ruleset_");
        return_value.append(ruleset);
        return_value.append("();\n");
    }
    return_value.append("}\n"
                        "} // namespace rules\n"
                        "} // namespace gaia\n");

    return return_value;
}

StringRef get_serial_group(const Decl* decl)
{
    const RulesetSerialGroupAttr* serial_attr = decl->getAttr<RulesetSerialGroupAttr>();
    if (serial_attr != nullptr)
    {
        if (serial_attr->group_size() == 1)
        {
            const IdentifierInfo* id = *(serial_attr->group_begin());
            return id->getName();
        }

        // If we see the serial_group attribute without an argument
        // then we just use the ruleset name as the serial_group name.
        // This is guaranteed to be unique as we do not allow duplicate
        // ruleset names.
        return g_current_ruleset;
    }

    // No serial_group() attribute so return an empty string
    return StringRef();
}

StringRef get_table_name(const Decl* decl)
{
    const FieldTableAttr* table_attr = decl->getAttr<FieldTableAttr>();
    if (table_attr != nullptr)
    {
        return table_attr->getTable()->getName();
    }
    return StringRef();
}

// The function parses a rule  attribute e.g.
// Employee
// Employee.name_last
// E:Employee
// E:Employee.name_last
bool parse_attribute(StringRef attribute, string& table, string& field, string& tag)
{
    StringRef tagless_attribute;
    size_t tag_position = attribute.find(':');
    if (tag_position != StringRef::npos)
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
    if (dot_position != StringRef::npos)
    {
        table = tagless_attribute.substr(0, dot_position);
        field = tagless_attribute.substr(dot_position + 1);
        return true;
    }
    validate_table_data();

    if (GaiaCatalog::getCatalogTableData().find(tagless_attribute) == GaiaCatalog::getCatalogTableData().end())
    {
        // Might be a field.
        for (const auto& tbl : GaiaCatalog::getCatalogTableData())
        {
            if (tbl.second.fieldData.find(tagless_attribute) != tbl.second.fieldData.end())
            {
                table = tbl.first();
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
bool validate_and_add_active_field(StringRef table_name, StringRef field_name, bool is_active_from_field = false)
{
    if (g_is_rule_prolog_specified && is_active_from_field)
    {
        gaiat::diag().emit(diag::err_active_field_not_supported);
        g_is_generation_error = true;
        return false;
    }

    validate_table_data();

    if (g_is_generation_error)
    {
        return false;
    }

    if (GaiaCatalog::getCatalogTableData().find(table_name) == GaiaCatalog::getCatalogTableData().end())
    {
        gaiat::diag().emit(diag::err_table_not_found) << table_name;
        g_is_generation_error = true;
        return false;
    }

    const auto& fields = GaiaCatalog::getCatalogTableData().find(table_name)->second.fieldData;
    const auto& field_iterator = fields.find(field_name);
    if (field_iterator == fields.end())
    {
        gaiat::diag().emit(diag::err_field_not_found) << field_name << table_name;
        g_is_generation_error = true;
        return false;
    }

    if (field_iterator->second.isDeprecated)
    {
        gaiat::diag().emit(diag::err_field_deprecated) << field_name << table_name;
        g_is_generation_error = true;
        return false;
    }

    // TODO[GAIAPLAT-622] If we ever add a "strict" mode to the database, then we
    // should reinstate checking for active fields.

    g_active_fields[table_name].insert(field_name);
    return true;
}

StringRef get_table_name(StringRef table, const llvm::StringMap<string>& tag_map)
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

string db_namespace(StringRef db_name)
{
    return db_name == catalog::c_empty_db_name ? "" : Twine(db_name, "::").str();
}

void generate_navigation(StringRef anchor_table, Rewriter& rewriter)
{
    if (g_is_generation_error)
    {
        return;
    }

    for (auto& insert_data : g_insert_data)
    {
        llvm::SmallString<32> class_qualification_string = StringRef("gaia::");
        class_qualification_string.append(db_namespace(GaiaCatalog::getCatalogTableData().find(insert_data.table_name)->second.dbName));
        class_qualification_string.append(insert_data.table_name);
        class_qualification_string.append("_t::");
        llvm::SmallString<64> replacement_string = class_qualification_string.str();
        replacement_string.append("get(");
        replacement_string.append(class_qualification_string);
        replacement_string.append("insert_row(");
        llvm::SmallVector<string, 16> function_arguments = table_navigation_t::get_table_fields(insert_data.table_name);
        const auto& table_data_iterator = GaiaCatalog::getCatalogTableData().find(insert_data.table_name);
        // Generate call arguments.
        for (const auto& call_argument : function_arguments)
        {
            const auto& argument_map_iterator = insert_data.argument_map.find(call_argument);
            if (argument_map_iterator == insert_data.argument_map.end())
            {
                // Provide default parameter value.
                const auto& field_data_iterator = table_data_iterator->second.fieldData.find(call_argument);
                switch (field_data_iterator->second.fieldType)
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
                replacement_string.append(insert_data.argument_replacement_map[argument_map_iterator->second]);
                replacement_string.append(",");
            }
        }
        replacement_string.resize(replacement_string.size() - 1);
        replacement_string.append("))");

        rewriter.ReplaceText(insert_data.expression_range, replacement_string);
    }

    for (const auto& explicit_path_data_iterator : g_expression_explicit_path_data)
    {
        SourceRange nomatch_range;
        // Find correct nomatch segment for a current declarative expression.
        // This segment will be used later to inject nomatch code at
        // the end of navigation code wrapping the declarative expression.
        for (const auto& nomatch_source_range : g_nomatch_location_list)
        {
            if (explicit_path_data_iterator.first.getEnd() == nomatch_source_range.getEnd())
            {
                nomatch_range = nomatch_source_range;
                break;
            }
        }
        const auto& break_label_iterator = g_break_label_map.find(explicit_path_data_iterator.first);
        const auto& continue_label_iterator = g_continue_label_map.find(explicit_path_data_iterator.first);
        StringRef break_label;
        StringRef continue_label;
        if (break_label_iterator != g_break_label_map.end())
        {
            break_label = break_label_iterator->second;
        }
        if (continue_label_iterator != g_continue_label_map.end())
        {
            continue_label = continue_label_iterator->second;
        }
        for (auto data_iterator = explicit_path_data_iterator.second.rbegin(); data_iterator != explicit_path_data_iterator.second.rend(); data_iterator++)
        {
            StringRef anchor_table_name = get_table_name(
                get_table_from_expression(anchor_table), data_iterator->tag_table_map);

            SourceRange variable_declaration_range;
            for (const auto& variable_declaration_range_iterator : g_variable_declaration_location)
            {
                StringRef variable_name = variable_declaration_range_iterator.second;

                if (is_range_contained_in_another_range(
                        explicit_path_data_iterator.first, variable_declaration_range_iterator.first))
                {
                    if (data_iterator->tag_table_map.find(variable_name) != data_iterator->tag_table_map.end()
                        || is_tag_defined(data_iterator->defined_tags, variable_name))
                    {
                        gaiat::diag().emit(variable_declaration_range_iterator.first.getBegin(), diag::err_tag_hidden) << variable_name;
                        g_is_generation_error = true;
                        return;
                    }

                    StringRef insert_table;

                    // Find a table on which insert method is invoked.
                    for (const auto& insert_data : g_insert_data)
                    {
                        if (insert_data.expression_range.getEnd() == variable_declaration_range_iterator.first.getEnd()
                            && insert_data.argument_table_names.find(insert_data.table_name) == insert_data.argument_table_names.end())
                        {
                            insert_table = insert_data.table_name;
                        }
                    }

                    if (g_variable_declaration_init_location.find(variable_declaration_range_iterator.first)
                        != g_variable_declaration_init_location.end())
                    {
                        StringRef table_name = get_table_name(
                            get_table_from_expression(
                                data_iterator->path_components.front()),
                            data_iterator->tag_table_map);

                        //Check if a declaration has table references that is not an anchor table.
                        if ((!insert_table.empty() && insert_table == table_name)
                            || (data_iterator->path_components.size() == 1
                                && table_name == anchor_table_name && !data_iterator->is_absolute_path))
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
                            gaiat::diag().emit(diag::err_dac_init);
                            g_is_generation_error = true;
                            return;
                        }
                    }
                }
            }

            if (variable_declaration_range.isValid())
            {
                continue;
            }

            if (data_iterator->skip_implicit_path_generation && data_iterator->path_components.size() == 1
                && break_label.empty() && continue_label.empty() && nomatch_range.isInvalid())
            {
                continue;
            }

            // Set the source location to print out diagnostics on a line close to where an error
            // might be reported.
            gaiat::diag().set_location(explicit_path_data_iterator.first.getBegin());
            navigation_code_data_t navigation_code;
            if (!(data_iterator->skip_implicit_path_generation && data_iterator->path_components.size() == 1))
            {
                navigation_code = table_navigation_t::generate_explicit_navigation_code(
                    anchor_table, *data_iterator);
                if (navigation_code.prefix.empty())
                {
                    g_is_generation_error = true;
                    return;
                }
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

            if (!break_label.empty() && &(*data_iterator) == &explicit_path_data_iterator.second.back())
            {
                navigation_code.postfix += (Twine("\n") + break_label + ":;\n").str();
            }
            if (!continue_label.empty() && &(*data_iterator) == &explicit_path_data_iterator.second.back())
            {
                navigation_code.postfix = (Twine("\n") + continue_label + ":;\n" + navigation_code.postfix).str();
            }

            if (nomatch_range.isValid())
            {
                string variable_name = table_navigation_t::get_variable_name(StringRef(), llvm::StringMap<string>());
                string nomatch_prefix = (Twine("{\nbool ") + variable_name + " = false;\n").str();
                rewriter.InsertTextBefore(explicit_path_data_iterator.first.getBegin(), (nomatch_prefix + navigation_code.prefix).str());
                rewriter.InsertTextAfter(explicit_path_data_iterator.first.getBegin(), Twine(variable_name, " = true;\n").str());
                rewriter.RemoveText(SourceRange(get_previous_token_location(nomatch_range.getBegin(), rewriter), nomatch_range.getBegin().getLocWithOffset(-1)));
                rewriter.InsertTextBefore(nomatch_range.getBegin(), (Twine(navigation_code.postfix) + "\nif (!" + variable_name + ")\n").str());
                rewriter.InsertTextAfterToken(nomatch_range.getEnd(), "}\n");
                nomatch_range = SourceRange();
            }
            else
            {
                rewriter.InsertTextBefore(explicit_path_data_iterator.first.getBegin(), navigation_code.prefix);
                rewriter.InsertTextAfterToken(explicit_path_data_iterator.first.getEnd(), navigation_code.postfix);
            }
        }
    }
}

void generate_table_subscription(
    StringRef table,
    StringRef field_subscription_code,
    int rule_count,
    bool subscribe_update,
    llvm::DenseMap<uint32_t, string>& rule_line_numbers,
    Rewriter& rewriter)
{
    llvm::SmallString<256> common_subscription_code;
    if (GaiaCatalog::getCatalogTableData().find(table) == GaiaCatalog::getCatalogTableData().end())
    {
        gaiat::diag().emit(diag::err_table_not_found) << table;
        g_is_generation_error = true;
        return;
    }
    string rule_name
        = (Twine(g_current_ruleset) + "_" + g_current_rule_declaration->getName() + "_" + Twine(rule_count)).str();
    string rule_name_log = (Twine(g_current_ruleset_rule_number) + "_" + table).str();

    string rule_line_var = rule_line_numbers[g_current_ruleset_rule_number];

    string serial_group = "nullptr";
    if (!g_current_ruleset_serial_group.empty())
    {
        serial_group = (Twine("\"") + g_current_ruleset_serial_group + "\"").str();
    }

    // Declare a constant for the line number of the rule if this is the first
    // time we've seen this rule.  Note that we may see a rule multiple times if
    // the rule has multiple anchor rows.
    if (rule_line_var.empty())
    {
        rule_line_var = (Twine("c_rule_line_") + Twine(g_current_ruleset_rule_number)).str();
        rule_line_numbers[g_current_ruleset_rule_number] = rule_line_var;

        (
            Twine(c_ident)
            + "const uint32_t "
            + rule_line_var
            + " = "
            + Twine(g_current_ruleset_rule_line_number)
            + ";\n")
            .toVector(common_subscription_code);
    }

    common_subscription_code.append(c_ident);
    common_subscription_code.append(c_nolint_identifier_naming);
    common_subscription_code.append("\n");
    common_subscription_code.append(c_ident);
    common_subscription_code.append("gaia::rules::rule_binding_t ");
    common_subscription_code.append(rule_name);
    common_subscription_code.append("binding(\"");
    common_subscription_code.append(g_current_ruleset);
    common_subscription_code.append("\",\"");
    common_subscription_code.append(rule_name_log);
    common_subscription_code.append("\",");
    common_subscription_code.append(g_current_ruleset);
    common_subscription_code.append("::");
    common_subscription_code.append(rule_name);
    common_subscription_code.append(",");
    common_subscription_code.append(rule_line_var);
    common_subscription_code.append(",");
    common_subscription_code.append(serial_group);
    common_subscription_code.append(");\n");

    g_current_ruleset_subscription += common_subscription_code;
    g_current_ruleset_unsubscription += common_subscription_code;

    if (field_subscription_code.empty())
    {
        g_current_ruleset_subscription.append(c_ident);
        g_current_ruleset_subscription.append("gaia::rules::subscribe_rule(gaia::");
        g_current_ruleset_subscription.append(db_namespace(GaiaCatalog::getCatalogTableData().find(table)->second.dbName));
        g_current_ruleset_subscription.append(table);
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
        g_current_ruleset_subscription.append(rule_name);
        g_current_ruleset_subscription.append("binding);\n");

        g_current_ruleset_unsubscription.append(c_ident);
        g_current_ruleset_unsubscription.append("gaia::rules::unsubscribe_rule(gaia::");
        g_current_ruleset_unsubscription.append(db_namespace(GaiaCatalog::getCatalogTableData().find(table)->second.dbName));
        g_current_ruleset_unsubscription.append(table);
        g_current_ruleset_unsubscription.append("_t::s_gaia_type, gaia::db::triggers::event_type_t::row_insert, gaia::rules::empty_fields,");
        g_current_ruleset_unsubscription.append(rule_name);
        g_current_ruleset_unsubscription.append("binding);\n");
    }
    else
    {
        g_current_ruleset_subscription.append(field_subscription_code);
        g_current_ruleset_subscription.append(c_ident);
        g_current_ruleset_subscription.append("gaia::rules::subscribe_rule(gaia::");
        g_current_ruleset_subscription.append(db_namespace(GaiaCatalog::getCatalogTableData().find(table)->second.dbName));
        g_current_ruleset_subscription.append(table);
        g_current_ruleset_subscription.append("_t::s_gaia_type, gaia::db::triggers::event_type_t::row_update, fields_");
        g_current_ruleset_subscription.append(rule_name);
        g_current_ruleset_subscription.append(",");
        g_current_ruleset_subscription.append(rule_name);
        g_current_ruleset_subscription.append("binding);\n");
        g_current_ruleset_unsubscription.append(field_subscription_code);
        g_current_ruleset_unsubscription.append(c_ident);
        g_current_ruleset_unsubscription.append("gaia::rules::unsubscribe_rule(gaia::");
        g_current_ruleset_unsubscription.append(db_namespace(GaiaCatalog::getCatalogTableData().find(table)->second.dbName));
        g_current_ruleset_unsubscription.append(table);
        g_current_ruleset_unsubscription.append("_t::s_gaia_type, gaia::db::triggers::event_type_t::row_update, fields_");
        g_current_ruleset_unsubscription.append(rule_name);
        g_current_ruleset_unsubscription.append(",");
        g_current_ruleset_unsubscription.append(rule_name);
        g_current_ruleset_unsubscription.append("binding);\n");
    }

    SmallString<64> function_prologue;
    (
        Twine("\n")
        + c_nolint_identifier_naming
        + "\nvoid "
        + rule_name)
        .toVector(function_prologue);
    bool is_absolute_path_only = true;
    for (const auto& explicit_path_data_iterator : g_expression_explicit_path_data)
    {
        if (!is_absolute_path_only)
        {
            break;
        }
        for (const auto& data_iterator : explicit_path_data_iterator.second)
        {
            if (!data_iterator.is_absolute_path)
            {
                StringRef first_component_table = get_table_from_expression(data_iterator.path_components.front());
                const auto& tag_iterator = data_iterator.tag_table_map.find(first_component_table);
                if (tag_iterator == data_iterator.tag_table_map.end() || tag_iterator->first() == tag_iterator->second || is_tag_defined(data_iterator.defined_tags, first_component_table) || g_attribute_tag_map.find(first_component_table) != g_attribute_tag_map.end())
                {
                    is_absolute_path_only = false;
                    break;
                }
            }
        }
    }

    bool is_anchor_generation_required = true;
    if (!g_is_rule_context_rule_name_referenced && (is_absolute_path_only || g_expression_explicit_path_data.empty()))
    {
        function_prologue.append("(const gaia::rules::rule_context_t*)\n");
        is_anchor_generation_required = false;
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
                (Twine("\nstatic const char gaia_rule_name[] = \"") + rule_name_log + "\";\n").str());
        }
        if (is_anchor_generation_required)
        {
            const auto& table_data = GaiaCatalog::getCatalogTableData();
            auto anchor_table_data_itr = table_data.find(table);

            if (anchor_table_data_itr == table_data.end())
            {
                return;
            }
            SmallString<256> anchor_code;
            (
                Twine("\nauto ")
                + table
                + " = gaia::"
                + db_namespace(anchor_table_data_itr->second.dbName)
                + table
                + "_t::get(context->record);\n"
                + "{\n")
                .toVector(anchor_code);

            for (const auto& attribute_tag_iterator : g_attribute_tag_map)
            {
                anchor_code.append("auto ");
                anchor_code.append(attribute_tag_iterator.first());
                anchor_code.append(" = ");
                anchor_code.append(table);
                anchor_code.append(";\n");
            }
            rewriter.InsertTextAfterToken(g_current_rule_declaration->getLocation(), anchor_code);
            rewriter.InsertTextBefore(g_current_rule_declaration->getEndLoc(), "\n}\n");
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
                (function_prologue + copy_rewriter.getRewrittenText(g_current_rule_declaration->getSourceRange())).str());
        }
        else
        {
            rewriter.InsertTextBefore(
                g_current_rule_declaration->getLocation(),
                (function_prologue + copy_rewriter.getRewrittenText(g_current_rule_declaration->getSourceRange())).str());
        }
    }
}

void optimize_subscription(StringRef table, int rule_count)
{
    // This is to reuse the same rule function and rule_binding_t
    // for the same table in case update and insert operation.
    if (g_insert_tables.find(table) != g_insert_tables.end())
    {
        string rule_name
            = (Twine(g_current_ruleset) + "_" + g_current_rule_declaration->getName() + "_" + Twine(rule_count)).str();
        g_current_ruleset_subscription.append(c_ident);
        g_current_ruleset_subscription.append("gaia::rules::subscribe_rule(gaia::");
        g_current_ruleset_subscription.append(db_namespace(GaiaCatalog::getCatalogTableData().find(table)->second.dbName));
        g_current_ruleset_subscription.append(table);
        g_current_ruleset_subscription.append("_t::s_gaia_type, gaia::db::triggers::event_type_t::row_insert, gaia::rules::empty_fields,");
        g_current_ruleset_subscription.append(rule_name);
        g_current_ruleset_subscription.append("binding);\n");

        g_current_ruleset_unsubscription.append(c_ident);
        g_current_ruleset_unsubscription.append("gaia::rules::unsubscribe_rule(gaia::");
        g_current_ruleset_unsubscription.append(db_namespace(GaiaCatalog::getCatalogTableData().find(table)->second.dbName));
        g_current_ruleset_unsubscription.append(table);
        g_current_ruleset_unsubscription.append("_t::s_gaia_type, gaia::db::triggers::event_type_t::row_insert, gaia::rules::empty_fields,");
        g_current_ruleset_unsubscription.append(rule_name);
        g_current_ruleset_unsubscription.append("binding);\n");

        g_insert_tables.erase(table);
    }
}

// [GAIAPLAT-799]:  For the preview release we do not allow a rule to have
// multiple anchor rows. They are not allowed to reference more than a single table or
// reference fields from multiple tables.  Note that the g_active_fields map is a
// map of <table, field_list> so the number of entries in the map is the number of unique
// tables used by all active fields.
bool has_multiple_anchors()
{
    if (g_insert_tables.size() > 1 || g_update_tables.size() > 1)
    {
        gaiat::diag().emit(g_last_rule_location, diag::err_multi_anchor_tables);
        return true;
    }

    if (g_active_fields.size() > 1)
    {
        gaiat::diag().emit(g_last_rule_location, diag::err_multi_anchor_fields);
        return true;
    }

    // Handle the special case of on_update(table1, table2.field)
    if (g_active_fields.size() == 1
        && g_update_tables.size() == 1
        && g_active_fields.find((g_update_tables.begin()->first())) == g_active_fields.end())
    {
        gaiat::diag().emit(g_last_rule_location, diag::err_multi_anchor_tables);
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
        gaiat::diag().emit(g_last_rule_location, diag::err_no_active_fields);
        g_is_generation_error = true;
        return;
    }
    int rule_count = 1;
    llvm::DenseMap<uint32_t, string> rule_line_numbers;

    // Optimize active fields by removing field subscriptions
    // if entire table was subscribed in rule prolog.
    for (const auto& table : g_update_tables)
    {
        g_active_fields.erase(table.first());
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

        StringRef table = field_description.first();

        if (field_description.second.empty())
        {
            gaiat::diag().emit(diag::err_no_fields_referenced_by_table) << table;
            g_is_generation_error = true;
            return;
        }

        SmallString<256> field_subscription_code;
        string rule_name
            = (Twine(g_current_ruleset) + "_" + g_current_rule_declaration->getName() + "_" + Twine(rule_count)).str();

        field_subscription_code.append(c_ident);
        field_subscription_code.append(c_nolint_identifier_naming);
        field_subscription_code.append("\n");
        field_subscription_code.append(c_ident);
        field_subscription_code.append("gaia::common::field_position_list_t fields_");
        field_subscription_code.append(rule_name);
        field_subscription_code.append(";\n");

        const auto& fields = GaiaCatalog::getCatalogTableData().find(table)->second.fieldData;

        for (const auto& field : field_description.second)
        {
            const auto& field_iterator = fields.find(field.first());
            if (field_iterator == fields.end())
            {
                gaiat::diag().emit(diag::err_field_not_found) << field.first() << table;
                g_is_generation_error = true;
                return;
            }
            field_subscription_code.append(
                (Twine(c_ident) + "fields_" + rule_name + ".push_back(" + Twine(field_iterator->second.position) + ");\n").str());
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

        generate_table_subscription(table.first(), StringRef(), rule_count, true, rule_line_numbers, rewriter);

        optimize_subscription(table.first(), rule_count);

        rule_count++;
    }

    for (const auto& table : g_insert_tables)
    {
        if (g_is_generation_error)
        {
            return;
        }

        generate_table_subscription(table.first(), StringRef(), rule_count, false, rule_line_numbers, rewriter);
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
        else if (const auto* expression = node_parents_iterator.get<CXXThrowExpr>())
        {
            SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
            update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<CXXOperatorCallExpr>())
        {
            SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
            update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<BinaryOperator>())
        {
            SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
            update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<UnaryOperator>())
        {
            SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
            update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<ConditionalOperator>())
        {
            SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
            update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<BinaryConditionalOperator>())
        {
            SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
            update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<CompoundAssignOperator>())
        {
            SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
            update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<CXXMemberCallExpr>())
        {
            SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
            update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<CallExpr>())
        {
            SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
            update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            return get_expression_source_range(context, *expression, return_value, rewriter);
        }
        else if (const auto* expression = node_parents_iterator.get<IfStmt>())
        {
            if (is_range_contained_in_another_range(expression->getCond()->getSourceRange(), return_value)
                || is_range_contained_in_another_range(return_value, expression->getCond()->getSourceRange()))
            {
                SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
                update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            }
            return return_value;
        }
        else if (const auto* expression = node_parents_iterator.get<SwitchStmt>())
        {
            SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
            update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            return return_value;
        }
        else if (const auto* expression = node_parents_iterator.get<WhileStmt>())
        {
            if (is_range_contained_in_another_range(expression->getCond()->getSourceRange(), return_value)
                || is_range_contained_in_another_range(return_value, expression->getCond()->getSourceRange()))
            {
                SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
                update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            }
            return return_value;
        }
        else if (const auto* expression = node_parents_iterator.get<DoStmt>())
        {
            if (is_range_contained_in_another_range(expression->getCond()->getSourceRange(), return_value)
                || is_range_contained_in_another_range(return_value, expression->getCond()->getSourceRange()))
            {
                SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
                update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
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
                SourceRange source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
                update_expression_location(return_value, source_range.getBegin(), source_range.getEnd());
            }
            return return_value;
        }
        else if (const auto* expression = node_parents_iterator.get<GaiaForStmt>())
        {
            SourceRange for_condition_source_range = SourceRange(expression->getLParenLoc().getLocWithOffset(1), expression->getRParenLoc().getLocWithOffset(-1));
            if (is_range_contained_in_another_range(for_condition_source_range, return_value)
                || is_range_contained_in_another_range(return_value, for_condition_source_range))
            {
                SourceRange for_source_range = get_statement_source_range(expression, rewriter.getSourceMgr(), rewriter.getLangOpts());
                update_expression_location(return_value, for_source_range.getBegin(), for_source_range.getEnd());
            }
            return return_value;
        }
        else if (const auto* declaration = node_parents_iterator.get<VarDecl>())
        {
            return_value.setBegin(declaration->getBeginLoc());
            return_value.setEnd(declaration->getEndLoc());

            SourceLocation end_location = Lexer::findLocationAfterToken(return_value.getEnd(), tok::semi, rewriter.getSourceMgr(), rewriter.getLangOpts(), true);
            if (end_location.isValid())
            {
                return_value.setEnd(end_location.getLocWithOffset(-1));
            }
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
    llvm::SmallVector<explicit_path_data_t, 8> path_data;
    SourceRange expression_source_range = get_expression_source_range(context, *node, source_range, rewriter);
    if (expression_source_range.isInvalid())
    {
        return;
    }
    const auto& explicit_path_data_iterator = g_expression_explicit_path_data.find(expression_source_range);
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
                optimize_path(expression_explicit_path_data_iterator.second, data, explicit_path_data_iterator != g_expression_explicit_path_data.end());
                if (expression_explicit_path_data_iterator.first == expression_source_range
                    || should_expression_location_be_merged(context, *node))
                {
                    expression_explicit_path_data_iterator.second.push_back(data);
                    return;
                }
            }
            else
            {
                StringRef first_component = get_table_from_expression(data.path_components.front());
                const auto& tag_iterator = data.tag_table_map.find(first_component);
                if (tag_iterator != data.tag_table_map.end()
                    && (tag_iterator->second != tag_iterator->first()
                        || (explicit_path_data_iterator != g_expression_explicit_path_data.end()
                            && can_path_be_optimized(first_component, expression_explicit_path_data_iterator.second))))
                {
                    data.skip_implicit_path_generation = true;
                }
                for (const auto& defined_tag_iterator : expression_explicit_path_data_iterator.second.front().defined_tags)
                {
                    data.tag_table_map[defined_tag_iterator.second] = defined_tag_iterator.first();
                    if (first_component == defined_tag_iterator.second)
                    {
                        data.skip_implicit_path_generation = true;
                    }
                }
            }
        }
    }
    path_data.push_back(data);

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
    StringRef table,
    StringRef variable_name,
    const SourceRange& source_range,
    Rewriter& rewriter)
{
    explicit_path_data_t path_data;
    StringRef table_name = get_table_from_expression(table);
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
    data.is_absolute_path = explicit_path_attribute->getPath().startswith("/") || explicit_path_attribute->getPath().startswith("@/");
    path_source_range.setBegin(SourceLocation::getFromRawEncoding(explicit_path_attribute->getPathStart()));
    path_source_range.setEnd(SourceLocation::getFromRawEncoding(explicit_path_attribute->getPathEnd()));
    llvm::SmallVector<string, 8> path_components;
    llvm::StringMap<string> tag_map;
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
    llvm::SmallVector<string, 8> tag_map_keys, tag_map_values, defined_tag_map_keys, defined_tag_map_values;

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
        data.tag_table_map[attribute_tag_map_iterator.first()] = attribute_tag_map_iterator.second;
    }
    return true;
}

void update_used_dbs(const explicit_path_data_t& explicit_path_data)
{
    for (const auto& component : explicit_path_data.path_components)
    {
        StringRef table_name = get_table_from_expression(component);

        auto tag_table_iterator = explicit_path_data.tag_table_map.find(table_name);
        if (tag_table_iterator != explicit_path_data.tag_table_map.end())
        {
            table_name = tag_table_iterator->second;
        }
        g_used_dbs.insert(GaiaCatalog::getCatalogTableData().find(table_name)->second.dbName);
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
            field_name = decl->getName();
            variable_name = expression->getNameInfo().getAsString();
            if (!get_explicit_path_data(decl, explicit_path_data, expression_source_range))
            {
                variable_name = table_name;
                explicit_path_present = false;
                expression_source_range = SourceRange(expression->getLocation(), expression->getEndLoc());
                g_used_dbs.insert(GaiaCatalog::getCatalogTableData().find(table_name)->second.dbName);
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
                gaiat::diag().set_location(decl->getBeginLoc());
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
                    explicit_path_present = false;
                    expression_source_range
                        = SourceRange(
                            member_expression->getBeginLoc(),
                            member_expression->getEndLoc());
                    g_used_dbs.insert(GaiaCatalog::getCatalogTableData().find(table_name)->second.dbName);
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
                    gaiat::diag().set_location(expression_source_range.getBegin());
                    if (!validate_and_add_active_field(table_name, field_name, true))
                    {
                        return;
                    }
                }
            }
            else
            {
                gaiat::diag().emit(member_expression->getBeginLoc(), diag::err_incorrect_base_type);
                g_is_generation_error = true;
                return;
            }
        }
        else
        {
            gaiat::diag().emit(diag::err_incorrect_matched_expression);
            g_is_generation_error = true;
            return;
        }
        if (expression_source_range.isValid())
        {
            string replacement = (Twine(variable_name) + "." + field_name + "()").str();
            for (auto& insert_data : g_insert_data)
            {
                for (auto& insert_data_argument_range_iterator : insert_data.argument_replacement_map)
                {
                    if (is_range_contained_in_another_range(expression_source_range, insert_data_argument_range_iterator.first))
                    {
                        insert_data_argument_range_iterator.second = replacement;
                        insert_data.argument_table_names.insert(table_name);
                    }
                }
            }
            g_used_dbs.insert(GaiaCatalog::getCatalogTableData().find(table_name)->second.dbName);
            m_rewriter.ReplaceText(expression_source_range, replacement);
            g_rewriter_history.push_back({expression_source_range, replacement, replace_text});
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
            // TODO:  Can we find a better location here?  If we don't have
            // our operator, then we'll just report the rule location.
            gaiat::diag().emit(g_last_rule_location, diag::err_incorrect_matched_operator);
            g_is_generation_error = true;
            return;
        }
        const Expr* operator_expression = op->getLHS();
        if (operator_expression == nullptr)
        {
            gaiat::diag().emit(op->getExprLoc(), diag::err_incorrect_operator_expression);
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
            gaiat::diag().emit(operator_expression->getExprLoc(), diag::err_incorrect_operator_expression_type);
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
            variable_name = table_name;
            if (!get_explicit_path_data(operator_declaration, explicit_path_data, set_source_range))
            {
                explicit_path_present = false;
                set_source_range.setBegin(left_declaration_expression->getLocation());
                g_used_dbs.insert(GaiaCatalog::getCatalogTableData().find(table_name)->second.dbName);
            }
            else
            {
                variable_name = get_table_from_expression(explicit_path_data.path_components.back());
                get_variable_name(variable_name, table_name, explicit_path_data);
                update_used_dbs(explicit_path_data);
            }
            if (operator_declaration->hasAttr<GaiaFieldValueAttr>())
            {
                if (!explicit_path_present)
                {
                    set_source_range = SourceRange(set_source_range.getBegin().getLocWithOffset(-1), set_source_range.getEnd());
                }
                gaiat::diag().set_location(set_source_range.getBegin());
                if (!validate_and_add_active_field(table_name, field_name, true))
                {
                    return;
                }
            }
        }
        else
        {
            auto* declaration_expression = dyn_cast<DeclRefExpr>(member_expression->getBase());
            if (declaration_expression == nullptr)
            {
                gaiat::diag().emit(member_expression->getExprLoc(), diag::err_incorrect_base_type);
                g_is_generation_error = true;
                return;
            }
            const ValueDecl* decl = declaration_expression->getDecl();
            field_name = member_expression->getMemberNameInfo().getName().getAsString();
            table_name = get_table_name(decl);
            variable_name = declaration_expression->getNameInfo().getAsString();

            if (!get_explicit_path_data(decl, explicit_path_data, set_source_range))
            {
                explicit_path_present = false;
                set_source_range.setBegin(member_expression->getBeginLoc());
                g_used_dbs.insert(GaiaCatalog::getCatalogTableData().find(table_name)->second.dbName);
            }
            else
            {
                variable_name = get_table_from_expression(explicit_path_data.path_components.back());
                get_variable_name(variable_name, table_name, explicit_path_data);
                update_used_dbs(explicit_path_data);
            }
            if (decl->hasAttr<GaiaFieldValueAttr>())
            {
                if (!explicit_path_present)
                {
                    set_source_range = SourceRange(set_source_range.getBegin().getLocWithOffset(-1), set_source_range.getEnd());
                }
                gaiat::diag().set_location(set_source_range.getBegin());
                if (!validate_and_add_active_field(table_name, field_name, true))
                {
                    return;
                }
            }
        }
        tok::TokenKind token_kind;
        string writer_variable = table_navigation_t::get_variable_name("writer", llvm::StringMap<string>());
        string replacement_text = (Twine("[&]() mutable {auto ")
                                   + writer_variable
                                   + " = "
                                   + variable_name
                                   + ".writer(); "
                                   + writer_variable
                                   + "."
                                   + field_name)
                                      .str();

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
            gaiat::diag().emit(op->getOperatorLoc(), diag::err_incorrect_operator_type);
            g_is_generation_error = true;
            return;
        }

        replacement_text.append(convert_compound_binary_opcode(op));

        if (left_declaration_expression != nullptr)
        {
            set_source_range.setEnd(Lexer::findLocationAfterToken(
                                        set_source_range.getBegin(), token_kind, m_rewriter.getSourceMgr(),
                                        m_rewriter.getLangOpts(), true)
                                        .getLocWithOffset(-1));
        }
        else
        {
            set_source_range.setEnd(Lexer::findLocationAfterToken(
                                        member_expression->getExprLoc(), token_kind, m_rewriter.getSourceMgr(),
                                        m_rewriter.getLangOpts(), true)
                                        .getLocWithOffset(-1));
        }
        m_rewriter.ReplaceText(set_source_range, replacement_text);
        g_rewriter_history.push_back({set_source_range, replacement_text, replace_text});
        replacement_text = (Twine("; ")
                            + writer_variable
                            + ".update_row(); return "
                            + writer_variable
                            + "."
                            + field_name
                            + ";}()")
                               .str();
        SourceLocation operator_end_location = op->getEndLoc();
        SourceRange operator_source_range = get_statement_source_range(op, m_rewriter.getSourceMgr(), m_rewriter.getLangOpts());
        if (operator_source_range.isValid() && operator_source_range.getEnd() < operator_end_location)
        {
            operator_end_location = operator_source_range.getEnd().getLocWithOffset(-2);
        }

        m_rewriter.InsertTextAfterToken(operator_end_location, replacement_text);
        g_rewriter_history.push_back({SourceRange(operator_end_location), replacement_text, insert_text_after_token});

        auto offset = Lexer::MeasureTokenLength(operator_end_location, m_rewriter.getSourceMgr(), m_rewriter.getLangOpts()) + 1;
        if (!explicit_path_present)
        {
            update_expression_used_tables(
                result.Context,
                op,
                table_name,
                variable_name,
                SourceRange(set_source_range.getBegin(), operator_end_location.getLocWithOffset(offset)),
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
    string convert_compound_binary_opcode(const BinaryOperator* op)
    {
        switch (op->getOpcode())
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
            gaiat::diag().emit(op->getOperatorLoc(), diag::err_incorrect_operator_code) << op->getOpcode();
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
            gaiat::diag().emit(g_last_rule_location, diag::err_incorrect_matched_operator);
            g_is_generation_error = true;
            return;
        }
        const Expr* operator_expression = op->getSubExpr();
        if (operator_expression == nullptr)
        {
            gaiat::diag().emit(op->getExprLoc(), diag::err_incorrect_operator_expression);
            g_is_generation_error = true;
            return;
        }
        const auto* declaration_expression = dyn_cast<DeclRefExpr>(operator_expression);
        const auto* member_expression = dyn_cast<MemberExpr>(operator_expression);

        if (declaration_expression == nullptr && member_expression == nullptr)
        {
            gaiat::diag().emit(operator_expression->getExprLoc(), diag::err_incorrect_operator_expression_type);
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
                variable_name = table_name;
                explicit_path_present = false;
                g_used_dbs.insert(GaiaCatalog::getCatalogTableData().find(table_name)->second.dbName);
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
                gaiat::diag().emit(member_expression->getExprLoc(), diag::err_incorrect_base_type);
                g_is_generation_error = true;
                return;
            }
            const ValueDecl* operator_declaration = declaration_expression->getDecl();
            field_name = member_expression->getMemberNameInfo().getName().getAsString();
            table_name = get_table_name(operator_declaration);
            variable_name = declaration_expression->getNameInfo().getAsString();
            if (!get_explicit_path_data(operator_declaration, explicit_path_data, operator_source_range))
            {
                explicit_path_present = false;
                g_used_dbs.insert(GaiaCatalog::getCatalogTableData().find(table_name)->second.dbName);
            }
            else
            {
                variable_name = get_table_from_expression(explicit_path_data.path_components.back());
                get_variable_name(variable_name, table_name, explicit_path_data);
                update_used_dbs(explicit_path_data);
            }
        }
        string writer_variable = table_navigation_t::get_variable_name("writer", llvm::StringMap<string>());
        if (op->isPostfix())
        {
            string temp_variable = table_navigation_t::get_variable_name("temp", llvm::StringMap<string>());
            if (op->isIncrementOp())
            {
                replace_string
                    = (Twine("[&]() mutable {auto ") + temp_variable + " = "
                       + variable_name + "." + field_name + "(); auto " + writer_variable + " = "
                       + variable_name + ".writer(); " + writer_variable + "." + field_name + "++; "
                       + writer_variable + ".update_row(); return " + temp_variable + ";}()")
                          .str();
            }
            else if (op->isDecrementOp())
            {
                replace_string
                    = (Twine("[&]() mutable {auto ") + temp_variable + " = "
                       + variable_name + "." + field_name + "(); auto " + writer_variable + " = "
                       + variable_name + ".writer(); " + writer_variable + "." + field_name + "--; "
                       + writer_variable + ".update_row(); return " + temp_variable + ";}()")
                          .str();
            }
        }
        else
        {
            if (op->isIncrementOp())
            {
                replace_string
                    = (Twine("[&]() mutable {auto ") + writer_variable + " = " + variable_name
                       + ".writer(); ++ " + writer_variable + "." + field_name
                       + ";" + writer_variable + ".update_row(); return " + writer_variable
                       + "." + field_name + ";}()")
                          .str();
            }
            else if (op->isDecrementOp())
            {
                replace_string
                    = (Twine("[&]() mutable {auto ") + writer_variable + " = " + variable_name
                       + ".writer(); -- " + writer_variable + "." + field_name
                       + ";" + writer_variable + ".update_row(); return " + writer_variable
                       + "." + field_name + ";}()")
                          .str();
            }
        }

        for (auto& insert_data : g_insert_data)
        {
            for (auto& insert_data_argument_range_iterator : insert_data.argument_replacement_map)
            {
                if (is_range_contained_in_another_range(SourceRange(op->getBeginLoc().getLocWithOffset(-1), op->getEndLoc().getLocWithOffset(1)), insert_data_argument_range_iterator.first))
                {
                    insert_data_argument_range_iterator.second = replace_string;
                    insert_data.argument_table_names.insert(table_name);
                }
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
        g_last_rule_location = rule_declaration->getSourceRange().getBegin();
        const GaiaOnUpdateAttr* update_attribute = rule_declaration->getAttr<GaiaOnUpdateAttr>();
        const GaiaOnInsertAttr* insert_attribute = rule_declaration->getAttr<GaiaOnInsertAttr>();
        const GaiaOnChangeAttr* change_attribute = rule_declaration->getAttr<GaiaOnChangeAttr>();
        gaiat::diag().set_location(rule_declaration->getSourceRange().getBegin());
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
        g_nomatch_location_list.clear();
        g_insert_data.clear();
        g_variable_declaration_location.clear();
        g_variable_declaration_init_location.clear();
        g_is_rule_prolog_specified = false;
        g_rule_attribute_source_range = SourceRange();
        g_is_rule_context_rule_name_referenced = false;

        if (update_attribute != nullptr)
        {
            g_rule_attribute_source_range = update_attribute->getRange();
            g_is_rule_prolog_specified = true;
            gaiat::diag().set_location(g_rule_attribute_source_range.getBegin());
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
                    if (!field.empty())
                    {
                        gaiat::diag().emit(diag::err_illegal_field_reference);
                        g_is_generation_error = true;
                        return;
                    }
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
            gaiat::diag().set_location(g_rule_attribute_source_range.getBegin());
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
        const auto* ruleset_declaration = result.Nodes.getNodeAs<RulesetDecl>("rulesetDecl");
        if (ruleset_declaration)
        {
            gaiat::diag().set_location(ruleset_declaration->getBeginLoc());
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
        g_nomatch_location_list.clear();
        g_insert_data.clear();
        g_variable_declaration_location.clear();
        g_variable_declaration_init_location.clear();
        g_is_rule_prolog_specified = false;
        g_rule_attribute_source_range = SourceRange();

        if (ruleset_declaration == nullptr)
        {
            return;
        }

        if (ruleset_declaration->getSourceRange().isInvalid())
        {
            g_is_generation_error = true;
            return;
        }

        if (!g_current_ruleset.empty())
        {
            g_generated_subscription_code.append("\nnamespace ");
            g_generated_subscription_code.append(g_current_ruleset);
            g_generated_subscription_code.append("{\nvoid subscribe_ruleset_");
            g_generated_subscription_code.append(g_current_ruleset);
            g_generated_subscription_code.append("()\n{\n");
            g_generated_subscription_code.append(g_current_ruleset_subscription);
            g_generated_subscription_code.append("}\nvoid unsubscribe_ruleset_");
            g_generated_subscription_code.append(g_current_ruleset);
            g_generated_subscription_code.append("()\n{\n");
            g_generated_subscription_code.append(g_current_ruleset_unsubscription);
            g_generated_subscription_code.append("}\n}\n");
        }
        g_current_ruleset = ruleset_declaration->getName().str();
        g_current_ruleset_serial_group = get_serial_group(ruleset_declaration);

        // Make sure each new ruleset name is unique.
        for (const auto& r : g_rulesets)
        {
            if (r == g_current_ruleset)
            {
                gaiat::diag().emit(ruleset_declaration->getBeginLoc(), diag::err_duplicate_ruleset) << g_current_ruleset;
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
            string replace_string = (Twine("namespace ") + g_current_ruleset
                                     + "\n{\n} // namespace " + g_current_ruleset + "\n")
                                        .str();
            m_rewriter.ReplaceText(
                SourceRange(
                    ruleset_declaration->getBeginLoc(),
                    ruleset_declaration->getEndLoc()),
                replace_string);
            g_rewriter_history.push_back(
                {SourceRange(ruleset_declaration->getBeginLoc(), ruleset_declaration->getEndLoc()),
                 replace_string,
                 replace_text});
        }
        else
        {
            string replace_start_string = (Twine("namespace ") + g_current_ruleset + "\n{\n").str();
            string replace_end_string = (Twine("} // namespace ") + g_current_ruleset + "\n").str();
            // Replace ruleset declaration that may include attributes with namespace declaration
            m_rewriter.ReplaceText(
                SourceRange(
                    ruleset_declaration->getBeginLoc(),
                    ruleset_declaration->decls_begin()->getBeginLoc().getLocWithOffset(c_declaration_to_ruleset_offset)),
                replace_start_string);

            // Replace closing brace with namespace comment.
            m_rewriter.ReplaceText(SourceRange(ruleset_declaration->getEndLoc()), replace_end_string);

            g_rewriter_history.push_back(
                {SourceRange(ruleset_declaration->getBeginLoc(), ruleset_declaration->decls_begin()->getBeginLoc().getLocWithOffset(c_declaration_to_ruleset_offset)),
                 replace_start_string, replace_text});
            g_rewriter_history.push_back(
                {SourceRange(ruleset_declaration->getEndLoc()),
                 replace_end_string, replace_text});
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
            gaiat::diag().set_location(variable_declaration->getSourceRange().getBegin());
            if (GaiaCatalog::getCatalogTableData().find(variable_name) != GaiaCatalog::getCatalogTableData().end())
            {
                gaiat::diag().emit(diag::warn_table_hidden) << variable_name;
                return;
            }

            for (const auto& table_data : GaiaCatalog::getCatalogTableData())
            {
                if (table_data.second.fieldData.find(variable_name) != table_data.second.fieldData.end())
                {
                    gaiat::diag().emit(diag::warn_field_hidden) << variable_name;
                    return;
                }
            }

            if (g_attribute_tag_map.find(variable_name) != g_attribute_tag_map.end())
            {
                gaiat::diag().emit(diag::err_tag_hidden) << variable_name;
                g_is_generation_error = true;
                return;
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

        string replacement_text;
        SourceRange expression_source_range;

        if (ruleset_expression != nullptr)
        {
            expression_source_range = SourceRange(ruleset_expression->getBeginLoc(), ruleset_expression->getEndLoc());
            replacement_text = (Twine("\"") + g_current_ruleset + "\"").str();
        }

        if (rule_expression != nullptr)
        {
            expression_source_range = SourceRange(rule_expression->getBeginLoc(), rule_expression->getEndLoc());
            replacement_text = "gaia_rule_name";
            g_is_rule_context_rule_name_referenced = true;
        }

        if (event_expression != nullptr)
        {
            expression_source_range = SourceRange(event_expression->getBeginLoc(), event_expression->getEndLoc());
            replacement_text = "context->event_type";
        }

        if (type_expression != nullptr)
        {
            expression_source_range = SourceRange(type_expression->getBeginLoc(), type_expression->getEndLoc());
            replacement_text = "context->gaia_type";
        }

        if (expression_source_range.isValid())
        {
            m_rewriter.ReplaceText(expression_source_range, replacement_text);
            g_rewriter_history.push_back(
                {expression_source_range, replacement_text, replace_text});

            for (const auto& insert_data : g_insert_data)
            {
                for (auto insert_data_argument_range_iterator : insert_data.argument_replacement_map)
                {
                    if (is_range_contained_in_another_range(expression_source_range, insert_data_argument_range_iterator.first))
                    {
                        insert_data_argument_range_iterator.second = replacement_text;
                    }
                }
            }
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
            gaiat::diag().emit(g_last_rule_location, diag::err_incorrect_matched_expression);
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
            explicit_path_present = false;
            expression_source_range = SourceRange(expression->getLocation(), expression->getEndLoc());
            g_used_dbs.insert(GaiaCatalog::getCatalogTableData().find(table_name)->second.dbName);
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
            if (g_insert_call_locations.find(expression->getBeginLoc()) != g_insert_call_locations.end())
            {
                gaiat::diag().set_location(expression->getBeginLoc());
                if (explicit_path_present)
                {
                    gaiat::diag().emit(diag::err_insert_with_explicit_nav);
                    g_is_generation_error = true;
                    return;
                }

                if (table_name != variable_name)
                {
                    gaiat::diag().emit(diag::err_insert_with_tag);
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
        if (g_is_generation_error)
        {
            return;
        }
        const auto* expression = result.Nodes.getNodeAs<IfStmt>("NoMatchIf");
        if (expression != nullptr)
        {
            SourceRange nomatch_location = get_statement_source_range(expression->getNoMatch(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts(), true);
            if (nomatch_location.isValid())
            {
                g_nomatch_location_list.push_back(nomatch_location);
            }
        }
        else
        {
            gaiat::diag().emit(g_last_rule_location, diag::err_incorrect_matched_expression);
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
        if (g_is_generation_error)
        {
            return;
        }
        const auto* expression = result.Nodes.getNodeAs<CXXMemberCallExpr>("removeCall");
        if (expression != nullptr)
        {
            m_rewriter.ReplaceText(SourceRange(expression->getExprLoc(), expression->getEndLoc()), "delete_row()");
            g_rewriter_history.push_back({SourceRange(expression->getExprLoc(), expression->getEndLoc()), "delete_row()", replace_text});
        }
        else
        {
            gaiat::diag().emit(g_last_rule_location, diag::err_incorrect_matched_expression);
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
        if (g_is_generation_error)
        {
            return;
        }
        const auto* expression = result.Nodes.getNodeAs<CXXMemberCallExpr>("insertCall");
        const auto* expression_declaration = result.Nodes.getNodeAs<DeclRefExpr>("tableCall");
        if (expression == nullptr || expression_declaration == nullptr)
        {
            gaiat::diag().emit(g_last_rule_location, diag::err_incorrect_matched_expression);
            g_is_generation_error = true;
            return;
        }
        insert_data_t insert_data;
        insert_data.expression_range = SourceRange(expression->getBeginLoc(), expression->getEndLoc());
        SourceLocation argument_start_location;
        const ValueDecl* decl = expression_declaration->getDecl();
        insert_data.table_name = get_table_name(decl);
        // Parse insert call arguments to build name value map.
        for (const auto& argument : expression->arguments())
        {
            argument_start_location = get_previous_token_location(
                get_previous_token_location(argument->getSourceRange().getBegin(), m_rewriter), m_rewriter);

            string raw_argument_name = m_rewriter.getRewrittenText(
                SourceRange(argument_start_location, argument->getSourceRange().getEnd()));
            size_t argument_name_end_position = raw_argument_name.find(':');
            string argument_name = raw_argument_name.substr(0, argument_name_end_position);
            // Trim the argument name of whitespaces.
            argument_name.erase(
                argument_name.begin(),
                find_if(argument_name.begin(), argument_name.end(), [](unsigned char ch) { return !isspace(ch); }));
            argument_name.erase(
                find_if(argument_name.rbegin(), argument_name.rend(), [](unsigned char ch) { return !isspace(ch); }).base(),
                argument_name.end());
            insert_data.argument_map[argument_name] = argument->getSourceRange();
            insert_data.argument_replacement_map[argument->getSourceRange()] = m_rewriter.getRewrittenText(argument->getSourceRange());
        }
        g_insert_data.push_back(insert_data);
        g_insert_call_locations.insert(expression->getBeginLoc());
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
        if (g_is_generation_error)
        {
            return;
        }
        const auto* expression = result.Nodes.getNodeAs<GaiaForStmt>("DeclFor");
        if (expression == nullptr)
        {
            gaiat::diag().emit(g_last_rule_location, diag::err_incorrect_matched_expression);
            g_is_generation_error = true;
            return;
        }

        string table_name;
        string variable_name;
        SourceRange expression_source_range;
        explicit_path_data_t explicit_path_data;
        bool explicit_path_present = true;

        gaiat::diag().set_location(expression->getBeginLoc());

        const auto* path = dyn_cast<DeclRefExpr>(expression->getPath());
        if (path == nullptr)
        {
            gaiat::diag().emit(diag::err_incorrect_for_expression);
            g_is_generation_error = true;
            return;
        }
        const ValueDecl* decl = path->getDecl();
        table_name = get_table_name(decl);
        if (!get_explicit_path_data(decl, explicit_path_data, expression_source_range))
        {
            g_used_dbs.insert(GaiaCatalog::getCatalogTableData().find(table_name)->second.dbName);
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
        SourceRange nomatch_location = get_statement_source_range(expression->getNoMatch(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts(), true);
        if (nomatch_location.isValid())
        {
            g_nomatch_location_list.push_back(nomatch_location);
        }
    }

private:
    Rewriter& m_rewriter;
};

// AST handler that is called when declarative break or continue are used in the rule.
class declarative_break_continue_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit declarative_break_continue_handler_t(Rewriter& r)
        : m_rewriter(r)
    {
    }
    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_is_generation_error)
        {
            return;
        }
        const auto* break_expression = result.Nodes.getNodeAs<BreakStmt>("DeclBreak");
        const auto* continue_expression = result.Nodes.getNodeAs<ContinueStmt>("DeclContinue");
        if (break_expression == nullptr && continue_expression == nullptr)
        {
            gaiat::diag().emit(g_last_rule_location, diag::err_incorrect_matched_expression);
            g_is_generation_error = true;
            return;
        }
        LabelDecl* decl;
        SourceRange expression_source_range;
        if (break_expression != nullptr)
        {
            decl = break_expression->getLabel();
            expression_source_range = break_expression->getSourceRange();
        }
        else
        {
            decl = continue_expression->getLabel();
            expression_source_range = continue_expression->getSourceRange();
        }

        // Handle non-declarative break/continue.
        if (decl == nullptr)
        {
            return;
        }

        const LabelStmt* label_statement = decl->getStmt();
        if (label_statement == nullptr)
        {
            g_is_generation_error = true;
            return;
        }

        const Stmt* statement = label_statement->getSubStmt();
        if (statement == nullptr)
        {
            g_is_generation_error = true;
            return;
        }
        SourceRange statement_source_range = get_statement_source_range(statement, m_rewriter.getSourceMgr(), m_rewriter.getLangOpts());

        string label_name;
        if (break_expression != nullptr)
        {
            auto break_label_iterator = g_break_label_map.find(statement_source_range);
            if (break_label_iterator == g_break_label_map.end())
            {
                label_name = table_navigation_t::get_variable_name("break", llvm::StringMap<string>());
                g_break_label_map[statement_source_range] = label_name;
            }
            else
            {
                label_name = break_label_iterator->second;
            }
        }
        else
        {
            auto continue_label_iterator = g_continue_label_map.find(statement_source_range);
            if (continue_label_iterator == g_continue_label_map.end())
            {
                label_name = table_navigation_t::get_variable_name("continue", llvm::StringMap<string>());
                g_continue_label_map[statement_source_range] = label_name;
            }
            else
            {
                label_name = continue_label_iterator->second;
            }
        }

        auto offset
            = Lexer::MeasureTokenLength(expression_source_range.getEnd(), m_rewriter.getSourceMgr(), m_rewriter.getLangOpts()) + 1;
        expression_source_range.setEnd(expression_source_range.getEnd().getLocWithOffset(offset));
        string replacement_string = (Twine("goto ") + label_name).str();
        m_rewriter.ReplaceText(expression_source_range, replacement_string);
        g_rewriter_history.push_back({expression_source_range, replacement_string, replace_text});
    }

private:
    Rewriter& m_rewriter;
};

// Handles the connect/disconnect calls. Mostly the code figures out what link is between
// table1.connect(table2) and inserts it: table1.link().connect(table2)
class declarative_connect_disconnect_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit declarative_connect_disconnect_handler_t(Rewriter& r)
        : m_rewriter(r){};

    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_is_generation_error)
        {
            return;
        }

        string src_table_name;
        string dest_table_name;
        string link_name;
        bool need_link_field = false;

        const auto* method_call_expr = result.Nodes.getNodeAs<CXXMemberCallExpr>("connectDisconnectCall");

        const auto* table_call = result.Nodes.getNodeAs<DeclRefExpr>("tableCall");
        src_table_name = get_table_name(table_call->getDecl());

        const auto* param = result.Nodes.getNodeAs<DeclRefExpr>("connectDisconnectParam");
        const auto* table_attr = param->getType()->getAsRecordDecl()->getAttr<GaiaTableAttr>();
        dest_table_name = table_attr->getTable()->getName().str();

        const auto* link_expr = result.Nodes.getNodeAs<MemberExpr>("tableFieldGet");

        const llvm::StringMap<CatalogTableData>& table_data = GaiaCatalog::getCatalogTableData();

        gaiat::diag().set_location(table_call->getLocation());

        auto src_table_iter = table_data.find(src_table_name);
        if (src_table_iter == table_data.end())
        {
            gaiat::diag().emit(diag::err_table_not_found) << src_table_name;
            g_is_generation_error = true;
            return;
        }
        const CatalogTableData& src_table_data = src_table_iter->second;

        auto dest_table_iter = table_data.find(dest_table_name);
        if (dest_table_iter == table_data.end())
        {
            gaiat::diag().emit(param->getLocation(), diag::err_table_not_found) << dest_table_name;
            g_is_generation_error = true;
            return;
        }

        // If the link_expr is not null this is a call in the form table.link.connect()
        // hence we don't need to look up the name. Otherwise, this is in the form
        // table.connect() and we have to infer the link name from the catalog.
        if (link_expr)
        {
            link_name = link_expr->getMemberNameInfo().getName().getAsString();
        }
        else
        {
            // Search what link connect src_table to dest_table.
            // We can assume that the link is unique because the check
            // has already been performed in SemaGaia.
            for (const auto& link_data_pair : src_table_data.linkData)
            {
                if (link_data_pair.second.targetTable == dest_table_name)
                {
                    link_name = link_data_pair.first();
                }
            }

            // We will need to add the field name.
            need_link_field = true;
        }

        if (link_name.empty())
        {
            gaiat::diag().emit(diag::err_no_path) << src_table_name << dest_table_name;
            g_is_generation_error = true;
            return;
        }

        auto link_data_iter = src_table_data.linkData.find(link_name);
        if (link_data_iter == src_table_data.linkData.end())
        {
            gaiat::diag().emit(diag::err_no_link) << src_table_name << link_name;
            g_is_generation_error = true;
            return;
        }

        if (need_link_field)
        {
            // Inserts the link name between the table name and the connect/disconnect method:
            // table.link_name().connect().
            m_rewriter.InsertTextBefore(method_call_expr->getExprLoc(), link_name + "().");
        }
    }

private:
    Rewriter& m_rewriter;
};

class unused_label_handler_t : public MatchFinder::MatchCallback
{
public:
    explicit unused_label_handler_t(Rewriter& r)
        : m_rewriter(r){};

    void run(const MatchFinder::MatchResult& result) override
    {
        if (g_is_generation_error)
        {
            return;
        }

        const auto* label_statement = result.Nodes.getNodeAs<LabelStmt>("labelDeclaration");
        if (label_statement == nullptr)
        {
            gaiat::diag().emit(diag::err_incorrect_matched_expression);
            g_is_generation_error = true;
            return;
        }

        const auto* label_declaration = label_statement->getDecl();
        if (label_declaration == nullptr)
        {
            gaiat::diag().emit(diag::err_incorrect_matched_expression);
            g_is_generation_error = true;
            return;
        }

        if (!label_declaration->isUsed())
        {
            SourceRange label_source_range = label_declaration->getSourceRange();
            SourceLocation end_location = Lexer::findLocationAfterToken(label_source_range.getEnd(), tok::colon, m_rewriter.getSourceMgr(), m_rewriter.getLangOpts(), true);
            if (end_location.isValid())
            {
                label_source_range.setEnd(end_location.getLocWithOffset(-1));
            }
            m_rewriter.RemoveText(label_source_range);
        }
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
        , m_declarative_for_match_handler(r)
        , m_declarative_break_continue_handler(r)
        , m_declarative_delete_handler(r)
        , m_declarative_insert_handler(r)
        , m_declarative_connect_disconnect_handler(r)
        , m_unused_label_handler(r)
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
                                    to(varDecl(
                                        anyOf(
                                            hasAttr(attr::GaiaField),
                                            hasAttr(attr::FieldTable),
                                            hasAttr(attr::GaiaFieldValue)),
                                        unless(hasAttr(attr::GaiaFieldLValue)))))
                                    .bind("tableCall")))
                  .bind("tableFieldGet");

        StatementMatcher table_field_set_matcher
            = binaryOperator(
                  allOf(
                      hasAncestor(ruleset_matcher),
                      isAssignmentOperator(),
                      hasLHS(
                          memberExpr(
                              hasDescendant(
                                  declRefExpr(
                                      to(varDecl(hasAttr(attr::GaiaFieldLValue)))))))))
                  .bind("fieldSet");

        StatementMatcher table_field_unary_operator_matcher
            = unaryOperator(
                  allOf(
                      hasAncestor(ruleset_matcher),
                      anyOf(
                          hasOperatorName("++"),
                          hasOperatorName("--")),
                      hasUnaryOperand(
                          memberExpr(
                              hasDescendant(
                                  declRefExpr(
                                      to(varDecl(hasAttr(attr::GaiaFieldLValue)))))))))
                  .bind("fieldUnaryOp");

        StatementMatcher if_no_match_matcher
            = ifStmt(allOf(
                         hasAncestor(rule_matcher),
                         hasNoMatch(anything())))
                  .bind("NoMatchIf");

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

        StatementMatcher declarative_break_matcher = breakStmt().bind("DeclBreak");
        StatementMatcher declarative_continue_matcher = continueStmt().bind("DeclContinue");
        StatementMatcher declarative_delete_matcher
            = cxxMemberCallExpr(
                  hasAncestor(ruleset_matcher),
                  callee(cxxMethodDecl(hasName("remove"))),
                  hasDescendant(table_call_matcher))
                  .bind("removeCall");

        StatementMatcher declarative_insert_matcher
            = cxxMemberCallExpr(
                  hasAncestor(ruleset_matcher),
                  callee(cxxMethodDecl(hasName("insert"))),
                  hasDescendant(table_call_matcher))
                  .bind("insertCall");

        // Matches an expression in the form: table.connect().
        StatementMatcher declarative_table_connect_disconnect_matcher
            = cxxMemberCallExpr(
                  hasAncestor(ruleset_matcher),
                  callee(cxxMethodDecl(
                      anyOf(
                          hasName(c_connect_keyword),
                          hasName(c_disconnect_keyword)))),
                  hasArgument(
                      0,
                      anyOf(
                          // table.connect(s1)
                          declRefExpr().bind("connectDisconnectParam"),
                          // table.connect(table2.insert())
                          hasDescendant(declRefExpr().bind("connectDisconnectParam")))),
                  on(table_call_matcher))
                  .bind("connectDisconnectCall");

        // Matches an expression in the form: table.link.connect().
        StatementMatcher declarative_link_connect_disconnect_matcher
            = cxxMemberCallExpr(
                  hasAncestor(ruleset_matcher),
                  callee(cxxMethodDecl(
                      anyOf(
                          hasName(c_connect_keyword),
                          hasName(c_disconnect_keyword)))),
                  hasArgument(
                      0,
                      anyOf(
                          // table.link.connect(s1)
                          declRefExpr().bind("connectDisconnectParam"),
                          // table.link.connect(table2.insert())
                          hasDescendant(declRefExpr().bind("connectDisconnectParam")))),
                  on(table_field_get_matcher))
                  .bind("connectDisconnectCall");

        StatementMatcher unused_label_matcher = labelStmt(hasAncestor(rule_matcher)).bind("labelDeclaration");

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
        m_matcher.addMatcher(declarative_for_matcher, &m_declarative_for_match_handler);
        m_matcher.addMatcher(declarative_break_matcher, &m_declarative_break_continue_handler);
        m_matcher.addMatcher(declarative_continue_matcher, &m_declarative_break_continue_handler);
        m_matcher.addMatcher(declarative_delete_matcher, &m_declarative_delete_handler);
        m_matcher.addMatcher(declarative_insert_matcher, &m_declarative_insert_handler);

        m_matcher.addMatcher(declarative_table_connect_disconnect_matcher, &m_declarative_connect_disconnect_handler);
        m_matcher.addMatcher(declarative_link_connect_disconnect_matcher, &m_declarative_connect_disconnect_handler);
        m_matcher.addMatcher(unused_label_matcher, &m_unused_label_handler);
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
    declarative_for_match_handler_t m_declarative_for_match_handler;
    declarative_break_continue_handler_t m_declarative_break_continue_handler;
    declarative_delete_handler_t m_declarative_delete_handler;
    declarative_insert_handler_t m_declarative_insert_handler;
    declarative_connect_disconnect_handler_t m_declarative_connect_disconnect_handler;
    unused_label_handler_t m_unused_label_handler;
};

// This class allows us to generate diagnostics with source file information
// right up to the point where we are about to call EndSourceFile() for the DiagnosticConsumer.
// The Translation Engine will generate code for the last rule when it gets a
// ASTFrontEndAction::EndSourceFileAction() call.  Unfortunately, this callback occurs after the DiagnosticConsumer
// EndSourceFile() gets called so we were losing all source line information for the last
// ruleset.  By creating a diagnostic consumer, we can override the EndSourceFile() call,
// generate the code for the last rule, and ensure we have good source info.
// See DiagnosticConsumer.h for information on BeginSourceFile and EndSourceFile.
class gaiat_diagnostic_consumer_t : public clang::TextDiagnosticPrinter
{
public:
    gaiat_diagnostic_consumer_t()
        : TextDiagnosticPrinter(llvm::errs(), new DiagnosticOptions())
    {
    }

    void set_rewriter(Rewriter* rewriter)
    {
        m_rewriter = rewriter;
    }

    void EndSourceFile() override
    {
        if (g_is_generation_error)
        {
            return;
        }

        if (!g_translation_engine_output_option.empty())
        {
            std::remove(g_translation_engine_output_option.c_str());
        }
        Rewriter& rewriter = *m_rewriter;

        // Always call the TextDiagnosticPrinter's EndSourceFile() method.
        auto call_end_source_file = ::gaia::common::scope_guard::make_scope_guard(
            [this] { TextDiagnosticPrinter::EndSourceFile(); });

        generate_rules(rewriter);
        if (g_is_generation_error)
        {
            return;
        }

        g_generated_subscription_code.append("namespace ");
        g_generated_subscription_code.append(g_current_ruleset);
        g_generated_subscription_code.append("{\nvoid subscribe_ruleset_");
        g_generated_subscription_code.append(g_current_ruleset);
        g_generated_subscription_code.append("()\n{\n");
        g_generated_subscription_code.append(g_current_ruleset_subscription);
        g_generated_subscription_code.append("}\n");
        g_generated_subscription_code.append("void unsubscribe_ruleset_");
        g_generated_subscription_code.append(g_current_ruleset);
        g_generated_subscription_code.append("()\n{\n");
        g_generated_subscription_code.append(g_current_ruleset_unsubscription);
        g_generated_subscription_code.append("}\n");
        g_generated_subscription_code.append("} // namespace ");
        g_generated_subscription_code.append(g_current_ruleset);
        g_generated_subscription_code.append("\n");
        g_generated_subscription_code.append(generate_general_subscription_code());

        if (!m_rewriter->getSourceMgr().getDiagnostics().hasErrorOccurred() && !g_is_generation_error && !g_translation_engine_output_option.empty())
        {
            std::error_code error_code;
            llvm::raw_fd_ostream output_file(g_translation_engine_output_option, error_code, llvm::sys::fs::F_None);

            if (!output_file.has_error())
            {
                output_file << "#include <cstring>\n";
                output_file << "\n";
                output_file << "#include \"gaia/rules/rules.hpp\"\n";
                output_file << "\n";
                for (const auto& db_iterator : g_used_dbs)
                {
                    const string& db = db_iterator.first();
                    if (db == catalog::c_empty_db_name)
                    {
                        output_file << "#include \"gaia.h\"\n";
                    }
                    else
                    {
                        output_file << "#include \"gaia_" << db << ".h\"\n";
                    }
                }

                m_rewriter->getEditBuffer(m_rewriter->getSourceMgr().getMainFileID())
                    .write(output_file);
                output_file << g_generated_subscription_code;
            }

            output_file.close();
        }
    }

private:
    // Pointer to the Rewriter instance owned by the
    // translation_engine_action_t class. Do not free.
    // It is guaranteed to live until EndSourceFileAction() which
    // occurs after EndSourceFile().
    Rewriter* m_rewriter;
};

gaiat_diagnostic_consumer_t g_diagnostic_consumer;

class translation_engine_action_t : public clang::ASTFrontendAction
{
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& compiler, llvm::StringRef) override
    {
        m_rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
        g_diagnostic_consumer.set_rewriter(&m_rewriter);

        DiagnosticsEngine& compiler_diagnostic_engine = compiler.getSourceManager().getDiagnostics();
        m_diagnostics_engine = std::make_unique<DiagnosticsEngine>(
            compiler_diagnostic_engine.getDiagnosticIDs(),
            &compiler_diagnostic_engine.getDiagnosticOptions(),
            compiler_diagnostic_engine.getClient(),
            false);
        m_diagnostics_source_manager = std::make_unique<SourceManager>(*m_diagnostics_engine, compiler.getFileManager(), false);

        g_diag_ptr = std::make_unique<diagnostic_context_t>(m_diagnostics_source_manager->getDiagnostics());
        return std::unique_ptr<clang::ASTConsumer>(
            new translation_engine_consumer_t(&compiler.getASTContext(), m_rewriter));
    }

private:
    Rewriter m_rewriter;
    std::unique_ptr<SourceManager> m_diagnostics_source_manager;
    std::unique_ptr<DiagnosticsEngine> m_diagnostics_engine;
};

int main(int argc, const char** argv)
{
    cl::SetVersionPrinter(print_version);
    cl::ResetAllOptionOccurrences();
    cl::HideUnrelatedOptions(g_translation_engine_category);

    std::string error_msg;

    // This loads compilation commands after "--" in the command line: gaiat <sourceFile> -- <compileCommands>
    // Errors in these commands will be visible later when the ClangTool is run.
    std::unique_ptr<CompilationDatabase> compilation_database
        = FixedCompilationDatabase::loadFromCommandLine(argc, argv, error_msg);

    llvm::raw_string_ostream error_msg_stream(error_msg);

    if (!cl::ParseCommandLineOptions(argc, argv, "A tool to generate C++ rule and rule subscription code from declarative rulesets.", &error_msg_stream))
    {
        // Since the ClangTool has not run yet, we must show errors from FixedCompilationDatabase::loadFromCommandLine()
        // and cl::ParseCommandLineOptions() or else errors from the former will be invisible.
        error_msg_stream.flush();
        llvm::errs() << error_msg;
        return EXIT_FAILURE;
    }

    cl::PrintOptionValues();

    if (g_help_option_alias || argc == 1)
    {
        // -help-list is omitted from the output because the categorized mode of PrintHelpMessage() behaves the same as -help-list.
        // This is the only way -h and -help differ.
        cl::PrintHelpMessage(false, true);
        return EXIT_SUCCESS;
    }

    if (g_source_files.empty())
    {
        // This is considered success instead of failure because it happens if a new user explores gaiat by
        // typing "gaiat" into their terminal with no file arguments. They didn't do anything bad
        // to deserve an EXIT_FAILURE.
        cl::PrintHelpMessage();
        return EXIT_SUCCESS;
    }

    if (g_source_files.size() > 1)
    {
        llvm::errs() << c_err_multiple_ruleset_files;
        return EXIT_FAILURE;
    }

    if (!g_instance_name.empty())
    {
        ::gaia::db::config::session_options_t session_options = ::gaia::db::config::get_default_session_options();
        session_options.db_instance_name = g_instance_name.getValue();
        session_options.skip_catalog_integrity_check = false;
        ::gaia::db::config::set_default_session_options(session_options);
    }

    if (!compilation_database)
    {
        compilation_database = llvm::make_unique<clang::tooling::FixedCompilationDatabase>(
            ".", std::vector<std::string>());
    }

    // Create a new Clang Tool instance (a LibTooling environment).
    ClangTool tool(*compilation_database, g_source_files);
    tool.setDiagnosticConsumer(&g_diagnostic_consumer);

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
