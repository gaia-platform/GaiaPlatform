/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include <clang/Catalog/GaiaCatalog.h>
#include "llvm/ADT/StringSet.h"
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
    llvm::SmallVector<string, 8> path_components;
    // Map from tag to table
    llvm::StringMap<string> tag_table_map;
    bool is_absolute_path{false};
    llvm::StringSet<> used_tables;
    llvm::StringMap<string> defined_tags;
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
    static string get_variable_name(const string& table, const llvm::StringMap<string>& tags);
    // Function that retrieve fields for a table in DB defined order.
    static llvm::SmallVector<string, 16> get_table_fields(const string& table);

private:
    struct navigation_data_t
    {
        string table_name;
        string linking_field;
    };

private:
    static string get_closest_table(const llvm::StringMap<int>& table_distance);
    static bool find_navigation_path(const string& src, const string& dst, llvm::SmallVector<navigation_data_t, 8>& current_path);
    static bool find_navigation_path(const string& src, const string& dst, llvm::SmallVector<navigation_data_t, 8>& current_path, const llvm::StringMap<clang::gaia::catalog::CatalogTableData>& graph_data);
    static navigation_code_data_t generate_navigation_code(const string& anchor_table, const llvm::StringSet<>& tables, const llvm::StringMap<string>& tags, string& last_table);
    static bool generate_navigation_step(const string& source_table, const string& source_field, const string& destination_table, const string& source_variable_name, const string& variable_name, navigation_code_data_t& navigation_data);
};
} // namespace translation
} // namespace gaia
