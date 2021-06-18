/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <clang/AST/Type.h>

#include <gaia/db/db.hpp>

#include <gaia_internal/catalog/gaia_catalog.h>

/*
 * Contains classes that make it simpler to generate code from the
 * catalog types. Each Catalog class has its own "facade".
 * Eg. gaia_table_t -> table_facade_t.
 *
 * The only user of this class is edc_generator.
 */

namespace clang
{
namespace gaia_catalog
{

class table_facade_t;
class field_facade_t;
class link_facade_t;

class table_facade_t
{
public:
    explicit table_facade_t(gaia::catalog::gaia_table_t table)
        : m_table(std::move(table))
    {
        for (auto& field : m_table.gaia_fields())
        {
            m_fields.emplace_back(field);
        }
    };

    std::string table_name() const;
    std::string table_type() const;
    std::string class_name() const;
    std::vector<field_facade_t> fields() const;
    std::vector<link_facade_t> incoming_links() const;
    std::vector<link_facade_t> outgoing_links() const;

private:
    gaia::catalog::gaia_table_t m_table;
    std::vector<field_facade_t> m_fields;
};

class field_facade_t
{
public:
    explicit field_facade_t(gaia::catalog::gaia_field_t field)
        : m_field(std::move(field)){};

    //    static std::vector<field_facade_t> from_field(std::string field_name);

    std::string field_name() const;
    QualType field_type(ASTContext& context) const;
    std::string table_name() const;
    bool is_string() const;
    bool is_vector() const;

private:
    gaia::catalog::gaia_field_t m_field;
};

class link_facade_t
{
public:
    explicit link_facade_t(gaia::catalog::gaia_relationship_t relationship, bool is_from_parent)
        : m_relationship(std::move(relationship)), m_is_from_parent(is_from_parent){};

    std::string field_name() const;
    QualType field_type(ASTContext& context) const;

    std::string from_table() const;
    std::string to_table() const;

protected:
    gaia::catalog::gaia_relationship_t m_relationship;
    bool m_is_from_parent;
};

class gaia_catalog_context_t
{
public:
    gaia_catalog_context_t()
    {
        gaia::db::begin_session();
        gaia::db::begin_transaction();

        for (const auto& table : gaia::catalog::gaia_table_t::list())
        {
            m_tables_cache[table.name()].emplace_back(table);
        }

        for (const auto& field : gaia::catalog::gaia_field_t::list())
        {
            auto field_facade = field_facade_t{field};
            m_fields_cache[field.name()].push_back(field_facade);
            m_table_to_fields_cache[field_facade.table_name()].push_back(field_facade);
        }

        for (const auto& relationship : gaia::catalog::gaia_relationship_t::list())
        {
            auto from_parent_link = link_facade_t{relationship, true};
            auto from_child_link = link_facade_t{relationship, false};

            m_links_cache[from_parent_link.field_name()].push_back(from_parent_link);
            m_links_cache[from_child_link.field_name()].push_back(from_child_link);

            m_table_to_links_cache[from_parent_link.from_table()].push_back(from_parent_link);
            m_table_to_links_cache[from_child_link.from_table()].push_back(from_child_link);
        }

        gaia::db::rollback_transaction();
        gaia::db::end_session();
    }

    std::optional<table_facade_t> find_table(std::string name)
    {
        auto pair = m_tables_cache.find(name);

        if (pair == m_tables_cache.end())
        {
            return std::nullopt;
        }

        std::vector<table_facade_t> tables = pair->second;

        if (tables.empty())
        {
            return std::nullopt;
        }
        else if (tables.size() > 1)
        {
            // TODO I believe the translation engine does not deal well with
            //   multiple databases.
            throw std::exception();
        }

        return tables.front();
    }

    table_facade_t get_table(std::string name)
    {
        auto table = find_table(name);

        if (!table)
        {
            throw std::exception();
        }

        return *table;
    }

    std::vector<field_facade_t> find_fields(std::string name)
    {
        auto pair = m_fields_cache.find(name);

        if (pair == m_fields_cache.end())
        {
            return {};
        }

        return pair->second;
    }

    std::vector<field_facade_t> find_fields_in_tables(const std::vector<std::string>& table_names, const std::string& field_name)
    {
        std::vector<field_facade_t> result;

        for (std::string table_name : table_names)
        {
            auto fields = m_table_to_fields_cache[table_name];
            for (field_facade_t& field : fields)
            {
                if (field.field_name() == field_name)
                {
                    result.push_back(field);
                }
            }
        }

        return result;
    }

    std::vector<link_facade_t> find_links(std::string name)
    {
        auto pair = m_links_cache.find(name);

        if (pair == m_links_cache.end())
        {
            return {};
        }

        return pair->second;
    }

    std::vector<link_facade_t> find_relationships_in_tables(const std::vector<std::string>& table_names, const std::string& relationship_name)
    {
        std::vector<link_facade_t> result;

        for (std::string table_name : table_names)
        {
            auto relationships = m_table_to_links_cache[table_name];
            for (link_facade_t& relationship : relationships)
            {
                if (relationship.field_name() == relationship_name)
                {
                    result.push_back(relationship);
                }
            }
        }

        return result;
    }

    bool is_name_valid(std::string name)
    {
        return (m_tables_cache.count(name) > 0)
            || (m_fields_cache.count(name) > 0)
            || (m_links_cache.count(name) > 0);
    }

    bool is_name_unique(std::string name)
    {
        int hits = 0;

        auto table = find_table(name);

        if (table)
        {
            hits++;
        }

        auto fields = find_fields(name);

        if (!fields.empty())
        {
            hits += fields.size();
        }

        auto relationships = find_links(name);

        if (!relationships.empty())
        {
            hits += relationships.size();
        }

        return hits == 1;
    }

private:
    // TODO: once we have indexes we can get rid of all these caches.
    std::unordered_map<std::string, std::vector<table_facade_t>> m_tables_cache;
    std::unordered_map<std::string, std::vector<field_facade_t>> m_fields_cache;
    std::unordered_map<std::string, std::vector<link_facade_t>> m_links_cache;

    std::unordered_map<std::string, std::vector<field_facade_t>> m_table_to_fields_cache;
    std::unordered_map<std::string, std::vector<link_facade_t>> m_table_to_links_cache;
};

} // namespace gaia_catalog
} // namespace clang
