/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

#include "gaia_internal/catalog/catalog_facade.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"

/*
 * Contains classes that make it simpler to generate code from the
 * catalog types. Each Catalog class has its own "facade".
 * Eg. gaia_table_t -> table_facade_t.
 *
 * Each generator (eg. gaiat, gaiac) may decide to subclass one of
 * these classes to provide more specific logic.
 */

namespace gaia
{
namespace catalog
{
namespace generate
{

class table_facade_t;
class field_facade_t;
class link_facade_t;

class database_facade_t
{
public:
    explicit database_facade_t(gaia_database_t database);

    std::string database_name() const;
    bool is_default_database();
    std::vector<table_facade_t> tables() const;

private:
    gaia_database_t m_database;
};

class table_facade_t
{
public:
    explicit table_facade_t(gaia_table_t table);

    std::string table_name() const;
    std::string table_type() const;
    std::string class_name() const;
    std::vector<field_facade_t> fields() const;
    bool has_string_or_vector() const;
    bool needs_reference_class() const;

    std::vector<link_facade_t> incoming_links() const;

    std::vector<link_facade_t> outgoing_links() const;

private:
    gaia_table_t m_table;
};

class field_facade_t
{
public:
    explicit field_facade_t(gaia_field_t field);

    std::string field_name() const;
    std::string field_type(bool is_function_parameter = false) const;
    std::string element_type() const;
    std::string table_name() const;
    std::pair<std::string, std::string> generate_expr_variable() const;
    bool is_string() const;
    bool is_vector() const;

    // Generates code for static initialization of DAC expr variables for C++11 compliance.
    // pair.first = declaration
    // pair.second = initialization
    static std::pair<std::string, std::string> generate_expr_variable(
        const std::string& table, const std::string& expr_accessor, const std::string& field);

private:
    gaia_field_t m_field;
};

class link_facade_t
{
public:
    link_facade_t(gaia::catalog::gaia_relationship_t relationship, bool is_from_parent);

    std::string field_name() const;
    std::string from_table() const;
    std::string to_table() const;

    std::string target_type() const;
    std::string expression_accessor() const;

    bool is_one_to_one() const;
    bool is_one_to_many() const;

    bool is_single_cardinality() const;
    bool is_multiple_cardinality() const;

    bool is_value_linked() const;

protected:
    gaia::catalog::gaia_relationship_t m_relationship;
    bool m_is_from_parent;
};

} // namespace generate
} // namespace catalog
} // namespace gaia
