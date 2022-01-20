/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <tuple>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"

using gaia::catalog::create_database;
using gaia::catalog::create_table;
using gaia::catalog::data_type_t;
using gaia::catalog::ddl::base_field_def_t;
using gaia::catalog::ddl::composite_name_t;
using gaia::catalog::ddl::data_field_def_t;
using gaia::catalog::ddl::field_def_list_t;

/**
 * Facilitate the creation of tables.
 */
class table_builder_t
{
public:
    static constexpr bool c_optional = true;
    static constexpr bool c_non_optional = false;

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

    table_builder_t& field(const std::string& field_name, data_type_t data_type, bool optional = false)
    {
        m_fields.emplace_back(field_name, data_type, optional);
        return *this;
    }

    table_builder_t& fail_on_exists(bool fail_on_exists)
    {
        m_fail_on_exists = fail_on_exists;
        return *this;
    }

    gaia::common::gaia_id_t create()
    {
        field_def_list_t fields;

        if (m_table_name.empty())
        {
            throw std::invalid_argument("table_name must be set!");
        }

        for (const auto& field : m_fields)
        {
            fields.emplace_back(
                std::make_unique<data_field_def_t>(
                    std::get<0>(field), std::get<1>(field), 1, std::get<2>(field)));
        }

        if (!m_db_name.empty())
        {
            create_database(m_db_name, m_fail_on_exists);
        }

        return create_table(m_db_name, m_table_name, fields, m_fail_on_exists);
    }

    gaia::common::gaia_type_t create_type()
    {
        gaia::common::gaia_id_t table_id = create();
        gaia::db::begin_transaction();
        gaia::common::gaia_type_t type_id = gaia::catalog::gaia_table_t::get(table_id).type();
        gaia::db::commit_transaction();

        return type_id;
    }

private:
    std::string m_table_name{};
    std::string m_db_name{};
    std::vector<std::tuple<std::string, data_type_t, bool>> m_fields{};
    bool m_fail_on_exists = false;
};
