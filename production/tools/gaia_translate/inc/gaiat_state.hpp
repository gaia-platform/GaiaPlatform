//
// Created by simone on 8/8/21.
//

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceLocation.h>
#pragma clang diagnostic pop

#include "table_navigation.h"

namespace std
{
template <>
struct hash<clang::SourceRange>
{
    std::size_t operator()(clang::SourceRange const& range) const noexcept;
};

template <>
struct hash<clang::SourceLocation>
{
    std::size_t operator()(clang::SourceLocation const& location) const noexcept;
};
} // namespace std

namespace gaia
{
namespace translation
{

enum rewriter_operation_t
{
    replace_text,
    insert_text_after_token,
    remove_text,
    insert_text_before,
};

struct rewriter_history_t
{
    clang::SourceRange range;
    string string_argument;
    rewriter_operation_t operation;
};

extern std::string g_current_ruleset;
extern bool g_is_generation_error;
extern int g_current_ruleset_rule_number;
extern unsigned int g_current_ruleset_rule_line_number;
extern bool g_is_rule_context_rule_name_referenced;
extern clang::SourceRange g_rule_attribute_source_range;
extern bool g_is_rule_prolog_specified;
extern std::vector<std::string> g_rulesets;
extern std::unordered_map<std::string, std::unordered_set<std::string>> g_active_fields;
extern std::unordered_set<std::string> g_insert_tables;
extern std::unordered_set<std::string> g_update_tables;
extern std::unordered_map<std::string, std::string> g_attribute_tag_map;
extern std::unordered_set<clang::SourceLocation> g_insert_call_locations;
extern std::unordered_map<clang::SourceRange, vector<explicit_path_data_t>> g_expression_explicit_path_data;
extern std::unordered_set<std::string> g_used_dbs;
extern const clang::FunctionDecl* g_current_rule_declaration;
extern std::string g_current_ruleset_subscription;
extern std::string g_generated_subscription_code;
extern std::string g_current_ruleset_unsubscription;
extern std::vector<rewriter_history_t> g_rewriter_history;
extern std::vector<clang::SourceRange> g_nomatch_location;
extern std::unordered_map<clang::SourceRange, std::string> g_variable_declaration_location;
extern std::unordered_set<clang::SourceRange> g_variable_declaration_init_location;
extern std::unordered_map<clang::SourceRange, clang::SourceLocation> g_nomatch_location_map;
extern std::unordered_map<clang::SourceRange, std::string> g_break_label_map;
extern std::unordered_map<clang::SourceRange, std::string> g_continue_label_map;

} // namespace translation
} // namespace gaia
