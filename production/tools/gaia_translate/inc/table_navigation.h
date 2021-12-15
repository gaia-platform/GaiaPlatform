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
#include "llvm/ADT/SmallString.h"
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
    // Anchor table to implement meta-rule 3.
    string anchor_table;
    string anchor_variable;
};

struct navigation_code_data_t
{
    llvm::SmallString<256> prefix;
    llvm::SmallString<256> postfix;
};

class table_navigation_t
{
public:
    // Function that generates code to navigate between tables for explicit navigation.
    static navigation_code_data_t generate_explicit_navigation_code(llvm::StringRef anchor_table, explicit_path_data_t path_data);
    // Function that generates variable name for navigation code variables.
    static string get_variable_name(llvm::StringRef table, const llvm::StringMap<string>& tags);
    // Function that retrieve fields for a table in DB defined order.
    static llvm::SmallVector<string, 16> get_table_fields(llvm::StringRef table);

private:
    static navigation_code_data_t generate_navigation_code(llvm::StringRef anchor_table, llvm::StringRef anchor_variable, const llvm::StringSet<>& tables, const llvm::StringMap<string>& tags, string& last_table);
    static bool generate_navigation_step(llvm::StringRef source_table, llvm::StringRef source_field, llvm::StringRef destination_table, llvm::StringRef source_variable_name, llvm::StringRef variable_name, navigation_code_data_t& navigation_data);
};
} // namespace translation
} // namespace gaia
