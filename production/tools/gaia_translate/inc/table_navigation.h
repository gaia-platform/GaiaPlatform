/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include <clang/Catalog/GaiaCatalog.h>
#pragma clang diagnostic pop

#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/catalog/catalog.hpp"

using namespace gaia;
using namespace gaia::common;
using namespace std;

namespace gaia
{

namespace translation
{

struct explicit_path_data_t
{
    explicit_path_data_t() = default;

    // Path Components.
    vector<string> path_components;
    // Map from tag to table
    unordered_map<string, string> tag_table_map;
    bool is_absolute_path{false};
    unordered_set<string> used_tables;
    unordered_map<string, string> defined_tags;
    string variable_name;
    bool skip_implicit_path_generation{false};
};

struct navigation_code_data_t
{
    string prefix;
    string postfix;
};

class table_navigation_t
{
public:
    // Function that generates code to navigate between tables for explicit navigation.
    static navigation_code_data_t generate_explicit_navigation_code(const string& anchor_table, explicit_path_data_t path_data);
    // Function that generates variable name for navigation code variables.
    static string get_variable_name(const string& table, const unordered_map<string, string>& tags);
    // Function that retrieve fields for a table in DB defined order.
    static vector<string> get_table_fields(const string& table);

private:
    struct navigation_data_t
    {
        string table_name;
        string linking_field;
    };

private:
    static string get_closest_table(const unordered_map<string, int>& table_distance);
    static bool find_navigation_path(const string& src, const string& dst, vector<navigation_data_t>& current_path);
    static bool find_navigation_path(const string& src, const string& dst, vector<navigation_data_t>& current_path, const unordered_map<string, clang::gaia::catalog::CatalogTableData>& graph_data);
    static navigation_code_data_t generate_navigation_code(const string& anchor_table, const unordered_set<string>& tables, const unordered_map<string, string>& tags, string& last_table);
    static bool generate_navigation_step(const string& source_table, const string& source_field, const string& destination_table, const string& source_variable_name, const string& variable_name, navigation_code_data_t& navigation_data);
};
} // namespace translation
} // namespace gaia
