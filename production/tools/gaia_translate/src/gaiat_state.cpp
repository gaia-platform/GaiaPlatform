//
// Created by simone on 8/8/21.
//

#include "gaiat_state.hpp"

namespace std
{

constexpr int c_encoding_shift = 16;
constexpr int c_encoding_mask = 0xFFFF;

std::size_t std::hash<clang::SourceRange>::operator()(const clang::SourceRange& range) const noexcept
{
    return std::hash<unsigned int>{}(
        (range.getBegin().getRawEncoding() << c_encoding_shift)
        | (range.getEnd().getRawEncoding() & c_encoding_mask));
}

std::size_t std::hash<clang::SourceLocation>::operator()(const clang::SourceLocation& location) const noexcept
{
    return std::hash<unsigned int>{}(location.getRawEncoding());
}

} // namespace std

namespace gaia
{
namespace translation
{

std::string g_current_ruleset;
bool g_is_generation_error = false;
int g_current_ruleset_rule_number = 1;
unsigned int g_current_ruleset_rule_line_number = 1;
bool g_is_rule_context_rule_name_referenced = false;
clang::SourceRange g_rule_attribute_source_range;
bool g_is_rule_prolog_specified = false;
std::vector<std::string> g_rulesets;
std::unordered_map<std::string, std::unordered_set<std::string>> g_active_fields;
std::unordered_set<std::string> g_insert_tables;
std::unordered_set<std::string> g_update_tables;
std::unordered_map<std::string, std::string> g_attribute_tag_map;
std::unordered_set<clang::SourceLocation> g_insert_call_locations;
std::unordered_map<clang::SourceRange, vector<explicit_path_data_t>> g_expression_explicit_path_data;
std::unordered_set<std::string> g_used_dbs;
const clang::FunctionDecl* g_current_rule_declaration = nullptr;
std::string g_current_ruleset_subscription;
std::string g_generated_subscription_code;
std::string g_current_ruleset_unsubscription;
std::vector<rewriter_history_t> g_rewriter_history;
std::vector<clang::SourceRange> g_nomatch_location;
std::unordered_map<clang::SourceRange, std::string> g_variable_declaration_location;
std::unordered_set<clang::SourceRange> g_variable_declaration_init_location;
std::unordered_map<clang::SourceRange, clang::SourceLocation> g_nomatch_location_map;
std::unordered_map<clang::SourceRange, std::string> g_break_label_map;
std::unordered_map<clang::SourceRange, std::string> g_continue_label_map;

} // namespace translation
} // namespace gaia
