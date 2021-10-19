/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <climits>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include <clang/Basic/DiagnosticSema.h>
#include <llvm/ADT/Twine.h>
#pragma clang diagnostic pop

#include "gaia_internal/common/random.hpp"

#include "diagnostics.h"
#include "table_navigation.h"


using namespace std;
using namespace clang;
using namespace ::gaia::translation;
using namespace clang::gaia::catalog;

constexpr char c_nolint_range_copy[] = "// NOLINTNEXTLINE(performance-for-range-copy)";
constexpr int c_variable_length = 15;

// Function that generates code to navigate between tables when explicit navigation path is specified.
navigation_code_data_t table_navigation_t::generate_explicit_navigation_code(llvm::StringRef anchor_table, explicit_path_data_t path_data)
{
    string last_variable_name;
    navigation_code_data_t return_value;
    const auto& table_data = GaiaCatalog::getCatalogTableData();
    if (table_data.empty() || path_data.path_components.empty())
    {
        return navigation_code_data_t();
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

        if (path_component == path_data.path_components.back() && !path_data.variable_name.empty())
        {
            last_variable_name = path_data.variable_name;
        }
        else
        {
            last_variable_name = get_variable_name(table, path_data.tag_table_map);

            if (last_variable_name != table)
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
                    return navigation_code_data_t();
                }
                return_value.prefix.append("\n{\n");
                return_value.prefix.append(c_nolint_range_copy);
                return_value.prefix.append("\nfor (auto ");
                return_value.prefix.append(last_variable_name);
                return_value.prefix.append(" : gaia::");
                return_value.prefix.append(table_data_itr->second.dbName);
                return_value.prefix.append("::");
                return_value.prefix.append(table);
                return_value.prefix.append("_t::");
                return_value.prefix.append("list())\n{\n");

                return_value.postfix = "\n}\n}\n";
            }
            else
            {
                if (!path_data.skip_implicit_path_generation)
                {
                    return_value = generate_navigation_code(anchor_table, path_data.used_tables, path_data.tag_table_map, last_variable_name);
                }
            }
            first_component = false;
        }
        else
        {
            if (!generate_navigation_step(source_table_type, source_field, table, source_table, last_variable_name, return_value))
            {
                return navigation_code_data_t();
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
    llvm::StringRef anchor_table, const llvm::StringSet<>& tables, const llvm::StringMap<string>& tags, string& last_variable_name)
{
    navigation_code_data_t return_value;
    const auto& table_data =GaiaCatalog::getCatalogTableData();
    if (table_data.empty() || tables.empty())
    {
        return navigation_code_data_t();
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
        return navigation_code_data_t();
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
    if (variable_name != anchor_table_name)
    {
        return_value.prefix.append("\n{\nauto ");
        return_value.prefix.append(variable_name);
        return_value.prefix.append(" = ");
        return_value.prefix.append(anchor_table_name);
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
        return navigation_code_data_t();
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
                    return navigation_code_data_t();
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
            llvm::SmallVector<navigation_data_t, 8> path;
            // Find topographically shortest path between anchor table and destination table.
            if (find_navigation_path(anchor_table_name, table_name, path))
            {
                string source_table = variable_name;
                string source_table_type = anchor_table_name;
                // Generate code for the path.
                for (const auto& p : path)
                {
                    if (p.table_name == anchor_table_name)
                    {
                        variable_name = anchor_table;
                    }
                    else if (p.table_name == table_name)
                    {
                        variable_name = last_variable_name;
                    }
                    else
                    {
                        variable_name = get_variable_name(p.table_name, tags);
                    }
                    if (!generate_navigation_step(source_table_type, "", p.table_name, source_table, variable_name, return_value))
                    {
                        return navigation_code_data_t();
                    }
                    source_table_type = p.table_name;
                    source_table = variable_name;
                }
                last_variable_name = variable_name;
            }
            else
            {
                return navigation_code_data_t();
            }
        }
        else
        {
            if (!generate_navigation_step(anchor_table_name, "", table_name, variable_name, last_variable_name, return_value))
            {
                return navigation_code_data_t();
            }
        }
        processed_tables.insert(last_variable_name);
    }

    return return_value;
}

// Auxilary function find topologically closest table.
StringRef table_navigation_t::get_closest_table(const llvm::StringMap<int>& table_distance)
{
    int min_distance = INT_MAX;
    StringRef return_value;
    for (const auto& d : table_distance)
    {
        if (d.second < min_distance)
        {
            min_distance = d.second;
            return_value = d.first();
        }
    }

    return return_value;
}

// Find shortest navigation path between 2 tables. If multiple shortest paths exist, return an error.
bool table_navigation_t::find_navigation_path(StringRef src, StringRef dst, SmallVector<navigation_data_t, 8>& current_path)
{
    if (src == dst)
    {
        return true;
    }
    const auto& table_data = GaiaCatalog::getCatalogTableData();
    bool return_value = find_navigation_path(src, dst, current_path, table_data);
    if (!return_value)
    {
        gaiat::diag().emit(diag::err_no_path) << src << dst;
        return false;
    }

    const size_t path_length = current_path.size();
    // Remove edges from the original shortest path and check if a shortest path with the same length can be found.
    for (size_t path_index = 0; path_index < path_length - 1; ++path_index)
    {
        SmallVector<navigation_data_t, 8> path;
        llvm::StringMap<CatalogTableData> graph_data(table_data);
        const auto& edge_src = current_path[path_index];
        const auto& edge_dst = current_path[path_index + 1];
        const auto& graph_itr = graph_data.find(edge_src.table_name);

        for (auto it = graph_itr->second.linkData.begin(); it != graph_itr->second.linkData.end();)
        {
            if (it->second.targetTable == edge_dst.table_name)
            {
                auto toErase = it;
                graph_itr->second.linkData.erase(toErase);
                ++it;
            }
            else
            {
                ++it;
            }
        }

        if (find_navigation_path(src, dst, path, graph_data))
        {
            if (path.size() == path_length)
            {
                gaiat::diag().emit(diag::err_multiple_shortest_paths) << src << dst;
                return false;
            }
        }
    }

    return true;
}

// Find shortest navigation path between 2 tables.
bool table_navigation_t::find_navigation_path(StringRef src, StringRef dst, llvm::SmallVector<navigation_data_t, 8>& current_path, const llvm::StringMap<CatalogTableData>& graph_data)
{
    if (src == dst)
    {
        return true;
    }

    llvm::StringMap<int> table_distance;
    llvm::StringMap<string> table_prev;
    llvm::StringMap<navigation_data_t> table_navigation;

    for (const auto& table_description : GaiaCatalog::getCatalogTableData())
    {
        table_distance[table_description.first()] = INT_MAX;
    }
    table_distance[src] = 0;

    string closest_table;

    while (closest_table != dst)
    {
        closest_table = get_closest_table(table_distance);
        if (closest_table.empty())
        {
            return false;
        }

        if (closest_table == dst)
        {
            break;
        }

        int distance = table_distance[closest_table];

        table_distance.erase(closest_table);

        auto table_itr = graph_data.find(closest_table);
        for (const auto& it : table_itr->second.linkData)
        {
            StringRef table_name = it.second.targetTable;
            if (table_distance.find(table_name) != table_distance.end())
            {
                if (table_distance[table_name] > distance + 1)
                {
                    table_distance[table_name] = distance + 1;
                    table_prev[table_name] = closest_table;
                    table_navigation[table_name] = {it.second.targetTable, it.first()};
                }
            }
        }
    }

    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    StringRef tbl = dst;
    while (table_prev[tbl] != "")
    {
        current_path.insert(current_path.begin(), table_navigation[tbl]);
        tbl = table_prev[tbl];
    }
    return true;
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
    auto table_itr = GaiaCatalog::getCatalogTableData().find(source_table);

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

llvm::SmallVector<string, 16> table_navigation_t::get_table_fields(StringRef table)
{
    llvm::SmallVector<string, 16> return_value;
    const auto& table_data = GaiaCatalog::getCatalogTableData();
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

    for (const auto& fieldData : table_iterator->second.fieldData)
    {
        return_value[fieldData.second.position] = fieldData.first();
    }

    return return_value;
}
