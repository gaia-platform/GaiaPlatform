#include "table_navigation.h"

#include <climits>

#include <iostream>
#include <random>

using namespace std;
using namespace gaia::translation;

static const char c_nolint_range_copy[] = "// NOLINTNEXTLINE(performance-for-range-copy)";
static const int c_variable_length = 15;

bool table_navigation_t::m_is_initialized = false;
unordered_map<string, table_data_t> table_navigation_t::m_table_data;
unordered_multimap<string, table_navigation_t::table_link_data_t> table_navigation_t::m_table_relationship_1;
unordered_multimap<string, table_navigation_t::table_link_data_t> table_navigation_t::m_table_relationship_n;

// Function that generates code to navigate between tables when explicit navigation path is specified.
navigation_code_data_t table_navigation_t::generate_explicit_navigation_code(const string& anchor_table, explicit_path_data_t path_data)
{
    string last_variable_name;
    ensure_initialization();
    navigation_code_data_t return_value;
    if (m_table_data.empty() || path_data.path_components.empty())
    {
        return navigation_code_data_t();
    }
    bool first_component = true;
    string source_table;
    string source_field;
    string source_table_type;
    // Iterate through explicit navigation path components
    for (const auto& path_component : path_data.path_components)
    {
        string table, field;
        auto dot_position = path_component.find('.');
        if (dot_position != string::npos)
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
            last_variable_name =  path_data.variable_name;
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
                table =  path_data.tag_table_map[table];
            }
        }

        if (first_component)
        {
            if (path_data.is_absolute_path)
            {
                auto table_data_itr = m_table_data.find(table);
                if (table_data_itr == m_table_data.end())
                {
                    return navigation_code_data_t();
                }
                return_value.prefix
                    .append("\n{\n")
                    .append(c_nolint_range_copy)
                    .append("\nfor (auto ")
                    .append(last_variable_name)
                    .append(" : gaia::")
                    .append(table_data_itr->second.db_name)
                    .append("::")
                    .append(table)
                    .append("_t::")
                    .append("list())\n{\n");

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
navigation_code_data_t table_navigation_t::generate_navigation_code(const string& anchor_table, const unordered_set<string>& tables,
    const unordered_map<string, string>& tags, string& last_variable_name)
{
    ensure_initialization();
    navigation_code_data_t return_value;
    if (m_table_data.empty() || tables.empty())
    {
        return navigation_code_data_t();
    }
    string anchor_table_name = anchor_table;
    string variable_name = get_variable_name(anchor_table, tags);

    if (variable_name == anchor_table)
    {
        auto tag_iterator = tags.find(anchor_table);
        anchor_table_name = tag_iterator->second;
    }
    auto anchor_table_data_itr = m_table_data.find(anchor_table);
    if (anchor_table_data_itr == m_table_data.end())
    {
        return navigation_code_data_t();
    }

    string table_name = *tables.begin();
    const auto table_iterator = tags.find(table_name);
    if (table_iterator != tags.end())
    {
        table_name = table_iterator->second;
    }

    if (tables.size() == 1 && table_name == anchor_table_name)
    {
        variable_name = last_variable_name;
    }
    return_value.prefix
        .append("\n{\nauto ")
        .append(variable_name)
        .append(" = gaia::")
        .append(anchor_table_data_itr->second.db_name)
        .append("::")
        .append(anchor_table)
        .append("_t::get(context->record);\n")
        .append("if (")
        .append(variable_name)
        .append(")\n{\n");
    return_value.postfix = "\n}\n}\n";

    if (tables.size() == 1 && table_name == anchor_table_name)
    {
        return return_value;
    }
    if (m_table_relationship_1.find(anchor_table_name) == m_table_relationship_1.end()
        && m_table_relationship_n.find(anchor_table_name) == m_table_relationship_n.end())
    {
        cerr << "No path between '" << anchor_table << "' and other tables." << endl;
        return navigation_code_data_t();
    }
    auto parent_itr = m_table_relationship_1.equal_range(anchor_table_name);
    auto child_itr = m_table_relationship_n.equal_range(anchor_table_name);
    unordered_set<string> processed_tables;
    // Iterate through list of destination tables
    for (const string& table : tables)
    {
        string table_name = table;
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

        string linking_field;
        for (auto it = parent_itr.first; it != parent_itr.second; ++it)
        {
            if (it != m_table_relationship_1.end() && it->second.table == table_name)
            {
                if (is_1_relationship)
                {
                    cerr << "There is more than one field that links '" << anchor_table << "' and '" << table << "'." << endl;
                    return navigation_code_data_t();
                }
                is_1_relationship = true;
                linking_field = it->second.field;
            }
        }

        for (auto it = child_itr.first; it != child_itr.second; ++it)
        {
            if (it != m_table_relationship_n.end() && it->second.table == table_name)
            {
                if (is_n_relationship)
                {
                    cerr << "There is more than one field that links '" << anchor_table << "' and '" << table << "'." << endl;
                    return navigation_code_data_t();
                }
                is_n_relationship = true;
                linking_field = it->second.field;
            }
        }

        if (is_1_relationship && is_n_relationship)
        {
            cerr << "Both relationships exist between tables '" << anchor_table << "' and '" << table << "'." << endl;
            return navigation_code_data_t();
        }

        if (!is_1_relationship && !is_n_relationship)
        {
            vector<navigation_data_t> path;
            // Find topographically shortest path between anchor table and destination table.
            if (find_navigation_path(anchor_table_name, table_name, path))
            {
                string source_table = variable_name;
                string source_table_type = anchor_table_name;
                // Generate code for the path.
                for (const auto& p : path)
                {
                    if (p.name == anchor_table_name)
                    {
                        variable_name = anchor_table;
                    }
                    else if (p.name == table_name)
                    {
                        variable_name = last_variable_name;
                    }
                    else
                    {
                        variable_name = get_variable_name(p.name, tags);
                    }
                    if (!generate_navigation_step(source_table_type, "", p.name, source_table, variable_name, return_value))
                    {
                        return navigation_code_data_t();
                    }
                    source_table_type = p.name;
                    source_table = variable_name;
                }
                last_variable_name = variable_name;
            }
            else
            {
                cerr << "No path between tables '" << anchor_table << "' and '" << table << "'." << endl;
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

// Fill internal data from catalog.
void table_navigation_t::fill_table_data()
{
    try
    {
        db_monitor_t monitor;

        for (const auto& field : catalog::gaia_field_t::list())
        {
            catalog::gaia_table_t table = field.table();
            if (!table)
            {
                cerr << "Incorrect table for field '" << field.name() << "'." << endl;
                m_table_data.clear();
                return;
            }

            table_data_t table_data = m_table_data[table.name()];
            if (table_data.field_data.find(field.name()) != table_data.field_data.end())
            {
                cerr << "Duplicate field '" << field.name() << "'." << endl;
                m_table_data.clear();
                return;
            }
            field_data_t field_data;
            field_data.is_active = field.active();
            field_data.position = field.position();
            field_data.is_deprecated = field.deprecated();
            table_data.db_name = table.database().name();
            table_data.field_data[field.name()] = field_data;
            m_table_data[table.name()] = table_data;
        }

        for (const auto& relationship : catalog::gaia_relationship_t::list())
        {
            catalog::gaia_table_t child_table = relationship.child();
            if (!child_table)
            {
                cerr << "Incorrect child table in the relationship '" << relationship.name() << "'." << endl;
                m_table_data.clear();
                return;
            }

            catalog::gaia_table_t parent_table = relationship.parent();
            if (!parent_table)
            {
                cerr << "Incorrect parent table in the relationship '" << relationship.name() << "'." << endl;
                m_table_data.clear();
                return;
            }
            table_link_data_t link_data_1;
            link_data_1.table = parent_table.name();
            link_data_1.field = relationship.to_parent_link_name();
            table_link_data_t link_data_n;
            link_data_n.table = child_table.name();
            link_data_n.field = relationship.to_child_link_name();

            m_table_relationship_1.emplace(child_table.name(), link_data_1);
            m_table_relationship_n.emplace(parent_table.name(), link_data_n);
        }
    }
    catch (const exception& e)
    {
        cerr << "An exception has occurred while processing the catalog: '" << e.what() << "'." << endl;
        m_table_data.clear();
        return;
    }
}

// Auxilary function find topologically closest table.
string table_navigation_t::get_closest_table(const unordered_map<string, int>& table_distance)
{
    int min_distance = INT_MAX;
    string return_value;
    for (const auto& d : table_distance)
    {
        if (d.second < min_distance)
        {
            min_distance = d.second;
            return_value = d.first;
        }
    }

    return return_value;
}

// Find shortest navigation path between 2 tables.
bool table_navigation_t::find_navigation_path(const string& src, const string& dst, vector<navigation_data_t>& current_path)
{
    if (src == dst)
    {
        return true;
    }

    unordered_map<string, int> table_distance;
    unordered_map<string, string> table_prev;
    unordered_map<string, navigation_data_t> table_navigation;

    for (const auto& table_description : m_table_data)
    {
        table_distance[table_description.first] = INT_MAX;
    }
    table_distance[src] = 0;

    string closest_table;

    while (closest_table != dst)
    {
        closest_table = get_closest_table(table_distance);
        if (closest_table == "")
        {
            return false;
        }

        if (closest_table == dst)
        {
            break;
        }

        int distance = table_distance[closest_table];

        table_distance.erase(closest_table);

        auto parent_itr = m_table_relationship_1.equal_range(closest_table);
        for (auto it = parent_itr.first; it != parent_itr.second; ++it)
        {
            if (it != m_table_relationship_1.end())
            {
                string tbl = it->second.table;
                if (table_distance.find(tbl) != table_distance.end())
                {
                    if (table_distance[tbl] > distance + 1)
                    {
                        table_distance[tbl] = distance + 1;
                        table_prev[tbl] = closest_table;
                        table_navigation[tbl] = {tbl, it->second.field, true};
                    }
                }
            }
        }

        auto child_itr = m_table_relationship_n.equal_range(closest_table);
        for (auto it = child_itr.first; it != child_itr.second; ++it)
        {
            if (it != m_table_relationship_n.end())
            {
                string tbl = it->second.table;
                if (table_distance.find(tbl) != table_distance.end())
                {
                    if (table_distance[tbl] > distance + 1)
                    {
                        table_distance[tbl] = distance + 1;
                        table_prev[tbl] = closest_table;
                        table_navigation[tbl] = {tbl, it->second.field, false};
                    }
                }
            }
        }
    }

    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    string tbl = dst;
    while (table_prev[tbl] != "")
    {
        current_path.insert(current_path.begin(), table_navigation[tbl]);
        tbl = table_prev[tbl];
    }
    return true;
}

// Get variable name for navigation code
string table_navigation_t::get_variable_name(const string& variable, const unordered_map<string, string>& tags)
{
    if (tags.find(variable) == tags.end())
    {
        return "_" + variable + "_" + generate_random_string(c_variable_length);
    }
    else
    {
        return variable;
    }
}

string table_navigation_t::generate_random_string(string::size_type length)
{
    const char chrs[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
                        "abcdefghijklmnopqrstuvwxyz0123456789";

    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<string::size_type> dis;

    string s;

    s.reserve(length);
    while (length--)
    {
        s += chrs[dis(gen) % (sizeof(chrs) - 1)];
    }

    return s;
}
// Function that generates a single navigation step code.
bool table_navigation_t::generate_navigation_step(const string& source_table, const string& source_field, const string& destination_table, const string& source_variable_name, const string& variable_name, navigation_code_data_t& navigation_data)
{
    auto parent_itr = m_table_relationship_1.equal_range(source_table);
    auto child_itr = m_table_relationship_n.equal_range(source_table);

    bool is_1_relationship = false, is_n_relationship = false;

    string linking_field = source_field;
    for (auto it = parent_itr.first; it != parent_itr.second; ++it)
    {
        if (it != m_table_relationship_1.end() && it->second.table == destination_table)
        {
            if (!source_field.empty())
            {
                if (it->second.field == source_field)
                {
                    is_1_relationship = true;
                    break;
                }
            }
            else
            {
                if (is_1_relationship)
                {
                    cerr << "There is more than one field that links '" << source_table << "' and '" << destination_table << "'." << endl;
                    return false;
                }
                is_1_relationship = true;
                linking_field = it->second.field;
            }
        }
    }

    for (auto it = child_itr.first; it != child_itr.second; ++it)
    {
        if (it != m_table_relationship_n.end() && it->second.table == destination_table)
        {
            if (!source_field.empty())
            {
                if (it->second.field == source_field)
                {
                    is_n_relationship = true;
                    break;
                }
            }
            else
            {
                if (is_n_relationship || is_1_relationship)
                {
                    cerr << "There is more than one field that links '" << source_table << "' and '" << destination_table << "'." << endl;
                    return false;
                }
                is_n_relationship = true;
                linking_field = it->second.field;
            }
        }
    }

    if (!is_n_relationship && !is_1_relationship)
    {
        cerr << "No relationship between '" << source_table << "' and '" << destination_table << "'." << endl;
        return false;
    }
    if (is_1_relationship)
    {
        navigation_data.prefix
            .append("auto ")
            .append(variable_name)
            .append(" = ")
            .append(source_variable_name)
            .append(".")
            .append(linking_field)
            .append("();\n")
            .append("if(")
            .append(variable_name)
            .append(")\n{\n");
        navigation_data.postfix.append("}\n");
    }
    else
    {
        navigation_data.prefix
            .append(c_nolint_range_copy)
            .append("\nfor (auto ")
            .append(variable_name)
            .append(" : ")
            .append(source_variable_name)
            .append(".")
            .append(linking_field)
            .append("())\n{\n");
        navigation_data.postfix.append("}\n");
    }
    return true;
}

void table_navigation_t::ensure_initialization()
{
    if (!m_is_initialized)
    {
        fill_table_data();
        m_is_initialized = true;
    }
}
