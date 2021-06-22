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
    explicit table_facade_t(gaia::catalog::gaia_table_t table);

    std::string table_name() const;
    std::string table_type() const;
    std::string class_name() const;
    std::vector<field_facade_t> fields() const;
    std::vector<link_facade_t> outgoing_links() const;

private:
    gaia::catalog::gaia_table_t m_table;
    std::vector<field_facade_t> m_fields;
    std::vector<link_facade_t> m_outgoing_links;
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

    std::string from_table() const;
    std::string to_table() const;

protected:
    gaia::catalog::gaia_relationship_t m_relationship;
    bool m_is_from_parent;
};

class gaia_catalog_context_t
{
public:
    gaia_catalog_context_t();

    std::optional<table_facade_t> find_table(std::string name);
    table_facade_t get_table(std::string name);

    std::vector<field_facade_t> find_fields(std::string name);
    std::vector<field_facade_t> find_fields_in_tables(const std::vector<std::string>& table_names, const std::string& field_name);

    std::vector<link_facade_t> find_links(std::string name);
    std::vector<link_facade_t> find_links_in_tables(const std::vector<std::string>& table_names, const std::string& link_name);

    bool is_name_valid(std::string name);
    bool is_name_unique(std::string name);

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
