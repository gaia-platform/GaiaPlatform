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
    table_navigation_t();
    const unordered_map<string, table_data_t>& get_table_data()
    {
        if (!m_is_initialized)
        {
            fill_table_data();
            m_is_initialized = true;
        }
        return m_table_data;
    }

    navigation_code_data_t generate_navigation_code(const string& anchor_table, unordered_set<string> tables);

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
    void fill_table_data();
    string get_closest_table(const unordered_map<string, int>& table_distance) const;
    bool find_navigation_path(const string& src, const string& dst, vector<navigation_data_t>& current_path) const;
    string generate_random_string(string::size_type length) const;
    bool m_is_initialized;
    unordered_map<string, table_data_t> m_table_data;
    unordered_multimap<string, table_link_data_t> m_table_relationship_1;
    unordered_multimap<string, table_link_data_t> m_table_relationship_n;
};
}
}
