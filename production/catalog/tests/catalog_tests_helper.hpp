/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "catalog.hpp"
#include "gaia_catalog.h"

using gaia::catalog::create_database;
using gaia::catalog::create_table;
using gaia::catalog::data_type_t;
using gaia::catalog::ddl::field_def_list_t;
using gaia::catalog::ddl::field_definition_t;

/**
 * Facilitate the creation of tables.
 */
class table_builder_t
{
public:
    static table_builder_t new_table(const std::string& table_name)
    {
        auto builder = table_builder_t();
        builder.m_table_name = table_name;
        return builder;
    }

    table_builder_t& database(const std::string& name)
    {
        m_db_name = name;
        return *this;
    }

    table_builder_t& table(const std::string& name)
    {
        m_table_name = name;
        return *this;
    }

    table_builder_t& field(const std::string& field_name, data_type_t data_type)
    {
        m_fields.emplace_back(field_name, data_type);
        return *this;
    }

    table_builder_t& reference(const std::string& field_name, const std::string& referenced_table_name)
    {
        m_references.emplace_back(field_name, referenced_table_name);
        return *this;
    }

    table_builder_t& reference(const std::string& referenced_table_name)
    {
        m_references.emplace_back("", referenced_table_name);
        return *this;
    }

    table_builder_t& fail_on_exists(bool fail_on_exists)
    {
        m_fail_on_exists = fail_on_exists;
        return *this;
    }

    gaia_id_t create()
    {
        field_def_list_t fields;

        if (m_table_name.empty())
        {
            throw invalid_argument("table_name must be set!");
        }

        for (const auto& field : m_fields)
        {
            fields.emplace_back(make_unique<field_definition_t>(field.first, field.second, 1));
        }

        for (const auto& reference : m_references)
        {
            fields.emplace_back(make_unique<field_definition_t>(reference.first, data_type_t::e_references, 1, reference.second));
        }

        if (!m_db_name.empty())
        {
            create_database(m_db_name, m_fail_on_exists);
        }

        return create_table(m_db_name, m_table_name, fields, m_fail_on_exists);
    }

private:
    std::string m_table_name{};
    std::string m_db_name{};
    std::vector<std::pair<std::string, data_type_t>> m_fields{};
    std::vector<std::pair<std::string, std::string>> m_references{};
    bool m_fail_on_exists = false;
};
