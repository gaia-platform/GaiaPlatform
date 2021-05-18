/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////

#pragma once

#include <string>
#include <unordered_set>

#include "gaia_internal/catalog/gaia_catalog.h"

using namespace gaia;
using namespace gaia::common;
using namespace std;

namespace gaia
{

namespace translation
{

struct field_data_t
{
    bool is_deprecated;
    bool is_active;
    field_position_t position;
};

struct table_data_t
{
    unordered_map<string, field_data_t> field_data;
    string db_name;
};

struct navigation_code_data_t
{
    string prefix;
    string postfix;
};

class table_navigation_t
{
public:
    static const unordered_map<string, table_data_t>& get_table_data()
    {
        ensure_initialization();
        return m_table_data;
    }
    // Function that generates code to navigate between tables for explicit navigation.
    static navigation_code_data_t generate_explicit_navigation_code(const string& anchor_table, vector<string> path,
        unordered_map<string, string> tags, bool is_absolute, unordered_set<string> used_tables);
    // Function that generates variable name for navigation code variables.
    static string get_variable_name(const string& table, const unordered_map<string, string>& tags);
private:
    class db_monitor_t
    {
    public:
        db_monitor_t()
        {
            gaia::db::begin_session();
            gaia::db::begin_transaction();
        }

        ~db_monitor_t()
        {
            gaia::db::commit_transaction();
            gaia::db::end_session();
        }
    };

    struct table_link_data_t
    {
        string table;
        string field;
    };

    struct navigation_data_t
    {
        string name;
        string linking_field;
        bool is_parent;
    };
private:
    static void ensure_initialization();
    static void fill_table_data();
    static string get_closest_table(const unordered_map<string, int>& table_distance);
    static bool find_navigation_path(const string& src, const string& dst, vector<navigation_data_t>& current_path);
    static string generate_random_string(string::size_type length);
    static unordered_map<string, string> generate_dummy_tag_map (const unordered_set<string>& tables);
    static navigation_code_data_t generate_navigation_code(const string& anchor_table, unordered_set<string> tables,
        unordered_map<string, string> tags,  string& last_table);
    static bool generate_navigation_step(const string& source_table, const string& source_field, const string& destination_table,
        const string& source_variable_name, const string& variable_name, navigation_code_data_t& navigation_data);
    static bool m_is_initialized;
    static unordered_map<string, table_data_t> m_table_data;
    static unordered_multimap<string, table_link_data_t> m_table_relationship_1;
    static unordered_multimap<string, table_link_data_t> m_table_relationship_n;
};
}
}
