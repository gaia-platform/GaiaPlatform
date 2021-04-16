#include <climits>
#include <iostream>
#include <random>
#include "TableNavigation.h"

using namespace std;
using namespace gaia::translation;

static const char c_nolint_range_copy[] = "// NOLINTNEXTLINE(performance-for-range-copy)";

table_navigation_t::table_navigation_t()
{
    fill_table_data();
}

navigation_code_data_t table_navigation_t::generate_navigation_code(const string& anchor_table, unordered_set<string> tables) const
{
     navigation_code_data_t return_value;
    if (m_table_data.empty())
    {
        return navigation_code_data_t();
    }
    auto anchor_table_data_itr = m_table_data.find(anchor_table);
    if (anchor_table_data_itr == m_table_data.end())
    {
        return navigation_code_data_t();
    }
    return_value.prefix
        .append("\n{\nauto ")
        .append(anchor_table)
        .append(" = gaia::")
        .append(anchor_table_data_itr->second.db_name)
        .append("::")
        .append(anchor_table)
        .append("_t::get(context->record);\n");
    return_value.postfix = "\n}\n";

    if (tables.size() == 1 && tables.find(anchor_table) != tables.end())
    {
        return return_value;
    }
    if (m_table_relationship_1.find(anchor_table) == m_table_relationship_1.end()
        && m_table_relationship_n.find(anchor_table) == m_table_relationship_n.end())
    {
        cerr << "No path between '" << anchor_table << "' and other tables." << endl;
        return navigation_code_data_t();
    }
    auto parent_itr = m_table_relationship_1.equal_range(anchor_table);
    auto child_itr = m_table_relationship_n.equal_range(anchor_table);
    unordered_set<string> processed_tables;
    for (const string& table : tables)
    {
        if (table == anchor_table)
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
            if (it != m_table_relationship_1.end() && it->second.table == table)
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
            if (it != m_table_relationship_n.end() && it->second.table == table)
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
            if (find_navigation_path(anchor_table, table, path))
            {
                string source_table = anchor_table;
                for (const auto& p : path)
                {
                    if (processed_tables.find(p.name) == processed_tables.end())
                    {
                        processed_tables.insert(p.name);
                        if (p.is_parent)
                        {
                            if (p.linking_field.empty())
                            {
                                return_value.prefix
                                    .append("auto ")
                                    .append(p.name)
                                    .append(" = ")
                                    .append(source_table)
                                    .append(".")
                                    .append(p.name)
                                    .append("();\n");
                            }
                            else
                            {
                                return_value.prefix
                                    .append("auto ")
                                    .append(p.name)
                                    .append(" = ")
                                    .append(source_table)
                                    .append(".")
                                    .append(p.linking_field)
                                    .append("_")
                                    .append(p.name)
                                    .append("();\n");
                            }
                        }
                        else
                        {
                            if (p.linking_field.empty())
                            {
                                return_value.prefix
                                    .append(c_nolint_range_copy)
                                    .append("\nfor (auto ")
                                    .append(p.name)
                                    .append(" : ")
                                    .append(source_table)
                                    .append(".")
                                    .append(p.name)
                                    .append("_list())\n{\n");
                            }
                            else
                            {
                                return_value.prefix
                                    .append(c_nolint_range_copy)
                                    .append("\nfor (auto ")
                                    .append(p.name)
                                    .append(" : ")
                                    .append(source_table)
                                    .append(".")
                                    .append(p.linking_field)
                                    .append("_")
                                    .append(p.name)
                                    .append("_list())\n{\n");
                            }

                            return_value.postfix.append("}\n");
                        }
                    }
                    source_table = p.name;
                }
            }
            else
            {
                cerr << "No path between tables '" << anchor_table << "' and '" << table << "'." << endl;
                return navigation_code_data_t();
            }
        }
        else
        {
            if (is_1_relationship)
            {
                if (linking_field.empty())
                {
                    return_value.prefix
                        .append("auto ")
                        .append(table)
                        .append(" = ")
                        .append(anchor_table)
                        .append(".")
                        .append(table)
                        .append("();\n");
                }
                else
                {
                    return_value.prefix
                        .append("auto ")
                        .append(table)
                        .append(" = ")
                        .append(anchor_table)
                        .append(".")
                        .append(linking_field)
                        .append("_")
                        .append(table)
                        .append("();\n");
                }
            }
            else
            {
                if (linking_field.empty())
                {
                    return_value.prefix
                        .append(c_nolint_range_copy)
                        .append("\nfor (auto ")
                        .append(table)
                        .append(" : ")
                        .append(anchor_table)
                        .append(".")
                        .append(table)
                        .append("_list())\n{\n");
                }
                else
                {
                    return_value.prefix
                        .append(c_nolint_range_copy)
                        .append("\nfor (auto ")
                        .append(table)
                        .append(" : ")
                        .append(anchor_table)
                        .append(".")
                        .append(linking_field)
                        .append("_")
                        .append(table)
                        .append("_list())\n{\n");
                }

                return_value.postfix.append("}\n");
            }
        }
        processed_tables.insert(table);
    }

    return return_value;
}

void table_navigation_t::fill_table_data()
{
    try
    {
        db_monitor_t monitor;

        for (const auto& field : catalog::gaia_field_t::list())
        {
            catalog::gaia_table_t tbl = field.gaia_table();
            if (!tbl)
            {
                cerr << "Incorrect table for field '" << field.name() << "'." << endl;
                m_table_data.clear();
                return;
            }

            table_data_t table_data = m_table_data[tbl.name()];
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
            table_data.db_name = tbl.gaia_database().name();
            table_data.field_data[field.name()] = field_data;
            m_table_data[tbl.name()] = table_data;
        }

        for (const auto& relationship : catalog::gaia_relationship_t::list())
        {
            catalog::gaia_table_t child_table = relationship.child_gaia_table();
            if (!child_table)
            {
                cerr << "Incorrect child table in the relationship '" << relationship.name() << "'." << endl;
                m_table_data.clear();
                return;
            }

            catalog::gaia_table_t parent_table = relationship.parent_gaia_table();
            if (!parent_table)
            {
                cerr << "Incorrect parent table in the relationship '" << relationship.name() << "'." << endl;
                m_table_data.clear();
                return;
            }
            table_link_data_t link_data_1;
            link_data_1.table = parent_table.name();
            link_data_1.field = relationship.name();
            table_link_data_t link_data_n;
            link_data_n.table = child_table.name();
            link_data_n.field = relationship.name();

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

string table_navigation_t::get_closest_table(const unordered_map<string, int>& table_distance) const
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

bool table_navigation_t::find_navigation_path(const string& src, const string& dst, vector<navigation_data_t>& current_path) const
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
                        navigation_data_t data = {tbl, it->second.field, true};
                        table_navigation[tbl] = data;
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
                        navigation_data_t data = {tbl, it->second.field, false};
                        table_navigation[tbl] = data;
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

string table_navigation_t::generate_random_string(string::size_type length) const
{
    const char chrs[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
        "abcdefghijklmnopqrstuvwxyz0123456789";

    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<string::size_type> dis;

    string s;

    s.reserve(length);
    while(length--)
    {
        s += chrs[dis(gen)%(sizeof(chrs) - 1)];
    }

    return s;
}
