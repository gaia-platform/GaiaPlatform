////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <climits>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include <clang/Basic/DiagnosticSema.h>
#include <llvm/ADT/Twine.h>
#pragma clang diagnostic pop

#include "gaia_internal/common/random.hpp"
#include "gaia_internal/gaiat/catalog_facade.hpp"

#include "diagnostics.h"
#include "table_navigation.h"

using namespace std;
using namespace clang;
using namespace clang::gaia::catalog;
using namespace ::gaia::translation;
using namespace ::gaia::catalog::gaiat;

constexpr char c_nolint_range_copy[] = "// NOLINTNEXTLINE(performance-for-range-copy)";
constexpr int c_variable_length = 15;

// Function that generates code to navigate between tables when explicit navigation path is specified.
navigation_code_data_t table_navigation_t::generate_explicit_navigation_code(llvm::StringRef anchor_table, explicit_path_data_t path_data)
{
    string last_variable_name;
    navigation_code_data_t return_value;
    const auto& table_data = getCatalogTableData();
    if (table_data.empty() || path_data.path_components.empty())
    {
        return {};
    }
    bool first_component = true;
    string source_table;
    string source_field;
    string source_table_type;
    // Iterate through explicit navigation path components
    for (StringRef path_component : path_data.path_components)
    {
        StringRef table, field;
        auto dot_position = path_component.find('.');
        if (dot_position != StringRef::npos)
        {
            table = path_component.substr(0, dot_position);
            field = path_component.substr(dot_position + 1);
        }
        else
        {
            table = path_component;
        }
        if (path_data.skip_implicit_path_generation
            && path_component == path_data.path_components.front()
            && (table == path_data.anchor_variable
                || (table == path_data.anchor_table
                    && path_data.tag_table_map.find(table) == path_data.tag_table_map.end())))
        {
            last_variable_name = path_data.anchor_variable;
            auto tag_iterator = path_data.tag_table_map.find(last_variable_name);
            if (tag_iterator != path_data.tag_table_map.end())
            {
                table = tag_iterator->second;
            }
        }
        else if (path_component == path_data.path_components.back() && !path_data.variable_name.empty())
        {
            last_variable_name = path_data.variable_name;
        }
        else
        {
            last_variable_name = get_variable_name(table, path_data.tag_table_map);
            if (table != last_variable_name)
            {
                // Path component is not a tag defined earlier. Check if it is a tag defined in the path
                auto defined_tag_iterator = path_data.defined_tags.find(table);
                if (defined_tag_iterator != path_data.defined_tags.end())
                {
                    last_variable_name = defined_tag_iterator->second;
                }
            }
            else
            {
                table = path_data.tag_table_map[table];
            }
        }

        if (first_component)
        {
            if (path_data.is_absolute_path)
            {
                auto table_data_itr = table_data.find(table);
                if (table_data_itr == table_data.end())
                {
                    return {};
                }
                string class_name = catalog::gaiat::table_facade_t::class_name(table);
                return_value.prefix.append("\n{\n");
                return_value.prefix.append(c_nolint_range_copy);
                return_value.prefix.append("\nfor (auto ");
                return_value.prefix.append(last_variable_name);
                return_value.prefix.append(" : gaia::");
                return_value.prefix.append(table_data_itr->second.dbName);
                return_value.prefix.append("::");
                return_value.prefix.append(class_name);
                return_value.prefix.append("::list())\n{\n");

                return_value.postfix = "\n}\n}\n";
            }
            else
            {
                if (!path_data.skip_implicit_path_generation)
                {
                    if (path_data.anchor_table.empty())
                    {
                        return_value = generate_navigation_code(anchor_table, anchor_table, path_data.used_tables, path_data.tag_table_map, last_variable_name);
                    }
                    else if (path_data.anchor_variable.empty())
                    {
                        return_value = generate_navigation_code(path_data.anchor_table, path_data.anchor_table, path_data.used_tables, path_data.tag_table_map, last_variable_name);
                    }
                    else
                    {
                        return_value = generate_navigation_code(path_data.anchor_table, path_data.anchor_variable, path_data.used_tables, path_data.tag_table_map, last_variable_name);
                    }
                }
            }
            first_component = false;
        }
        else
        {
            if (!generate_navigation_step(source_table_type, source_field, table, source_table, last_variable_name, return_value))
            {
                return {};
            }
        }
        source_table_type = table;
        source_table = last_variable_name;
        source_field = field;
    }

    return return_value;
}

// Function that generates  code to navigate between anchor table and set of tables and return more data about the generated path.
navigation_code_data_t table_navigation_t::generate_navigation_code(
    llvm::StringRef anchor_table, llvm::StringRef anchor_variable, const llvm::StringSet<>& tables, const llvm::StringMap<string>& tags, string& last_variable_name)
{
    navigation_code_data_t return_value;
    const auto& table_data = getCatalogTableData();
    if (table_data.empty() || tables.empty())
    {
        return {};
    }
    StringRef anchor_table_name = anchor_table;
    string variable_name = get_variable_name(anchor_table, tags);

    if (variable_name == anchor_table)
    {
        auto tag_iterator = tags.find(anchor_table);
        anchor_table_name = tag_iterator->second;
    }
    auto anchor_table_data_itr = table_data.find(anchor_table);
    if (anchor_table_data_itr == table_data.end())
    {
        return {};
    }

    StringRef table_name = tables.begin()->first();
    const auto table_iterator = tags.find(table_name);
    if (table_iterator != tags.end())
    {
        table_name = table_iterator->second;
    }

    if (table_name == anchor_table_name)
    {
        variable_name = last_variable_name;
    }

    if (variable_name != anchor_variable)
    {
        return_value.prefix.append("\n{\nauto ");
        return_value.prefix.append(variable_name);
        return_value.prefix.append(" = ");
        return_value.prefix.append(anchor_variable);
        return_value.prefix.append(";\n");
    }
    else
    {
        return_value.prefix = "\n{\n";
    }
    return_value.postfix = "\n}\n";
    if (tables.size() == 1 && table_name == anchor_table_name)
    {
        return return_value;
    }
    if (anchor_table_data_itr->second.linkData.empty())
    {
        gaiat::diag().emit(diag::err_no_anchor_path) << anchor_table;
        return {};
    }
    llvm::StringSet<> processed_tables;
    // Iterate through list of destination tables
    for (const auto& table_name_iterator : tables)
    {
        StringRef table = table_name_iterator.first();
        StringRef table_name = table;
        const auto table_iterator = tags.find(table);
        if (table_iterator != tags.end())
        {
            table_name = table_iterator->second;
        }
        if (table_name == anchor_table_name)
        {
            continue;
        }

        if (processed_tables.find(table) != processed_tables.end())
        {
            continue;
        }

        bool is_1_relationship = false, is_n_relationship = false;

        StringRef linking_field;
        for (const auto& it : anchor_table_data_itr->second.linkData)
        {
            if (it.second.targetTable == table_name)
            {
                if (is_1_relationship || is_n_relationship)
                {
                    gaiat::diag().emit(diag::err_ambiguous_path) << anchor_table << table;
                    return {};
                }
                if (it.second.cardinality == catalog::relationship_cardinality_t::one)
                {
                    is_1_relationship = true;
                }
                else
                {
                    is_n_relationship = true;
                }
                linking_field = it.first();
            }
        }

        if (!is_1_relationship && !is_n_relationship)
        {
            llvm::SmallVector<string, c_size_8> path;
            // Find topographically shortest path between anchor table and destination table.
            if (findNavigationPath(anchor_table_name, table_name, path))
            {
                string source_table = variable_name;
                string source_table_type = anchor_table_name;
                // Generate code for the path.
                for (const auto& path_component : path)
                {
                    if (path_component == anchor_table_name)
                    {
                        variable_name = anchor_table;
                    }
                    else if (path_component == table_name)
                    {
                        variable_name = last_variable_name;
                    }
                    else
                    {
                        variable_name = get_variable_name(path_component, tags);
                    }
                    if (!generate_navigation_step(source_table_type, "", path_component, source_table, variable_name, return_value))
                    {
                        return {};
                    }
                    source_table_type = path_component;
                    source_table = variable_name;
                }
                last_variable_name = variable_name;
            }
            else
            {
                return {};
            }
        }
        else
        {
            if (!generate_navigation_step(anchor_table_name, "", table_name, variable_name, last_variable_name, return_value))
            {
                return {};
            }
        }
        processed_tables.insert(last_variable_name);
    }

    return return_value;
}

// Get variable name for navigation code
string table_navigation_t::get_variable_name(StringRef variable, const llvm::StringMap<string>& tags)
{
    if (tags.find(variable) == tags.end())
    {
        return (Twine("_") + Twine(variable) + Twine("_") + Twine(common::gen_random_str(c_variable_length))).str();
    }
    else
    {
        return variable;
    }
}

// Function that generates a single navigation step code.
bool table_navigation_t::generate_navigation_step(StringRef source_table, StringRef source_field, StringRef destination_table, StringRef source_variable_name, StringRef variable_name, navigation_code_data_t& navigation_data)
{
    auto table_itr = getCatalogTableData().find(source_table);

    bool is_1_relationship = false, is_n_relationship = false;

    StringRef linking_field = source_field;
    for (const auto& it : table_itr->second.linkData)
    {
        if (it.second.targetTable == destination_table)
        {
            if (!source_field.empty())
            {
                if (it.first() == source_field)
                {
                    if (it.second.cardinality == catalog::relationship_cardinality_t::one)
                    {
                        is_1_relationship = true;
                    }
                    else
                    {
                        is_n_relationship = true;
                    }
                    break;
                }
            }
            else
            {
                if (is_1_relationship || is_n_relationship)
                {
                    gaiat::diag().emit(diag::err_ambiguous_path) << source_table << destination_table;
                    return false;
                }
                if (it.second.cardinality == catalog::relationship_cardinality_t::one)
                {
                    is_1_relationship = true;
                }
                else
                {
                    is_n_relationship = true;
                }
                linking_field = it.first();
            }
        }
    }

    if (!is_n_relationship && !is_1_relationship)
    {
        gaiat::diag().emit(diag::err_no_relationship) << source_table << destination_table;
        return false;
    }
    if (is_1_relationship)
    {
        navigation_data.prefix.append("{\nauto ");
        navigation_data.prefix.append(variable_name);
        navigation_data.prefix.append(" = ");
        navigation_data.prefix.append(source_variable_name);
        navigation_data.prefix.append(".");
        navigation_data.prefix.append(linking_field);
        navigation_data.prefix.append("();\n");
        navigation_data.prefix.append("if(");
        navigation_data.prefix.append(variable_name);
        navigation_data.prefix.append(")\n{\n");
        navigation_data.postfix.append("}\n}\n");
    }
    else
    {
        navigation_data.prefix.append(c_nolint_range_copy);
        navigation_data.prefix.append("\nfor (auto ");
        navigation_data.prefix.append(variable_name);
        navigation_data.prefix.append(" : ");
        navigation_data.prefix.append(source_variable_name);
        navigation_data.prefix.append(".");
        navigation_data.prefix.append(linking_field);
        navigation_data.prefix.append("())\n{\n");
        navigation_data.postfix.append("}\n");
    }
    return true;
}

llvm::SmallVector<string, c_size_16> table_navigation_t::get_table_fields(StringRef table)
{
    llvm::SmallVector<string, c_size_16> return_value;
    const auto& table_data = getCatalogTableData();
    if (table_data.empty())
    {
        return return_value;
    }
    const auto table_iterator = table_data.find(table);
    if (table_iterator == table_data.end())
    {
        gaiat::diag().emit(diag::err_table_not_found) << table;
        return return_value;
    }

    return_value.resize(table_iterator->second.fieldData.size());

    for (const auto& field_data : table_iterator->second.fieldData)
    {
        return_value[field_data.second.position] = field_data.first();
    }

    return return_value;
}
