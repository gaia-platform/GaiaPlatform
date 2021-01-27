/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/db/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"

using gaia::catalog::create_database;
using gaia::catalog::create_table;
using gaia::catalog::data_type_t;
using gaia::catalog::ddl::base_field_def_t;
using gaia::catalog::ddl::composite_name_t;
using gaia::catalog::ddl::data_field_def_t;
using gaia::catalog::ddl::field_def_list_t;
using gaia::catalog::ddl::ref_field_def_t;

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

    table_builder_t& reference(
        const std::string& field_name,
        const std::string& referenced_table_db_name,
        const std::string& referenced_table_name)
    {
        m_references.emplace_back(field_name, make_pair(referenced_table_db_name, referenced_table_name));
        return *this;
    }

    table_builder_t& reference(
        const std::string& field_name,
        const std::string& referenced_table_name)
    {
        return reference(field_name, "", referenced_table_name);
    }

    table_builder_t& anonymous_reference(
        const std::string& referenced_table_db_name,
        const std::string& referenced_table_name)
    {
        m_references.emplace_back("", make_pair(referenced_table_db_name, referenced_table_name));
        return *this;
    }

    table_builder_t& anonymous_reference(const std::string& referenced_table_name)
    {
        return anonymous_reference("", referenced_table_name);
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
            fields.emplace_back(std::make_unique<data_field_def_t>(field.first, field.second, 1));
        }

        for (const auto& reference : m_references)
        {
            fields.emplace_back(std::make_unique<ref_field_def_t>(reference.first, reference.second));
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
    std::vector<std::pair<std::string, data_type_t>> m_fields{};
    std::vector<std::pair<std::string, composite_name_t>> m_references{};
    bool m_fail_on_exists = false;
};
