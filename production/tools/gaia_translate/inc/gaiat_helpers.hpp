//
// Created by simone on 8/8/21.
//

#pragma once

#include <string>
#include <unordered_map>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/raw_ostream.h>
#pragma clang diagnostic pop

#include "table_navigation.h"

namespace gaia
{
namespace translation
{

void print_version(llvm::raw_ostream& stream);

clang::SourceRange get_statement_source_range(const clang::Stmt* expression, const clang::SourceManager& source_manager, const clang::LangOptions& options);

clang::SourceRange get_if_statement_source_range(const clang::IfStmt* expression, const clang::SourceManager& source_manager, const clang::LangOptions& options);

void get_variable_name(std::string& variable_name, std::string& table_name, explicit_path_data_t& explicit_path_data);

bool is_range_contained_in_another_range(const clang::SourceRange& range1, const clang::SourceRange& range2);

std::string get_table_from_expression(const std::string& expression);

bool is_tag_defined(const std::unordered_map<std::string, std::string>& tag_map, const std::string& tag);

void optimize_path(vector<explicit_path_data_t>& path, explicit_path_data_t& path_segment);

bool is_path_segment_contained_in_another_path(
    const vector<explicit_path_data_t>& path,
    const explicit_path_data_t& path_segment);

void validate_table_data();

std::string generate_general_subscription_code();

std::string get_table_name(const clang::Decl* decl);

// The function parses a rule  attribute e.g.
// Employee
// Employee.name_last
// E:Employee
// E:Employee.name_last
bool parse_attribute(const std::string& attribute, std::string& table, std::string& field, std::string& tag);

// This function adds a field to active fields list if it is marked as active in the catalog;
// it returns true if there was no error and false otherwise.
bool validate_and_add_active_field(const std::string& table_name, const std::string& field_name, bool is_active_from_field = false);

std::string get_table_name(const std::string& table, const unordered_map<std::string, std::string>& tag_map);

void generate_navigation(const std::string& anchor_table, clang::Rewriter& rewriter);

void generate_table_subscription(
    const std::string& table,
    const std::string& field_subscription_code,
    int rule_count,
    bool subscribe_update,
    unordered_map<uint32_t, std::string>& rule_line_numbers,
    clang::Rewriter& rewriter);

void optimize_subscription(const std::string& table, int rule_count);

// [GAIAPLAT-799]:  For the preview release we do not allow a rule to have
// multiple anchor rows. They are not allowed to reference more than a single table or
// reference fields from multiple tables.  Note that the g_active_fields map is a
// map of <table, field_list> so the number of entries in the map is the number of unique
// tables used by all active fields.
bool has_multiple_anchors();

void generate_rules(clang::Rewriter& rewriter);

void update_expression_location(clang::SourceRange& source, clang::SourceLocation start, clang::SourceLocation end);

clang::SourceRange get_expression_source_range(clang::ASTContext* context, const clang::Stmt& node, const clang::SourceRange& source_range, clang::Rewriter& rewriter);

bool is_expression_from_body(clang::ASTContext* context, const clang::Stmt& node);

bool should_expression_location_be_merged(clang::ASTContext* context, const clang::Stmt& node, bool special_parent = false);

void update_expression_explicit_path_data(
    clang::ASTContext* context,
    const clang::Stmt* node,
    explicit_path_data_t data,
    const clang::SourceRange& source_range,
    clang::Rewriter& rewriter);

void update_expression_used_tables(
    clang::ASTContext* context,
    const clang::Stmt* node,
    const std::string& table,
    const std::string& variable_name,
    const clang::SourceRange& source_range,
    clang::Rewriter& rewriter);

bool get_explicit_path_data(const clang::Decl* decl, explicit_path_data_t& data, clang::SourceRange& path_source_range);

void update_used_dbs(const explicit_path_data_t& explicit_path_data);

} // namespace translation
} // namespace gaia
