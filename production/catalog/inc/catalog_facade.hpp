/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

#include <gaia_internal/catalog/gaia_catalog.h>

/*
 * Contains classes that make it simpler to generate code from the
 * catalog types. Each Catalog class has its own "facade".
 * Eg. gaia_table_t -> table_facade_t.
 */

namespace gaia
{
namespace catalog
{
namespace generate
{

class table_facade_t;
class field_facade_t;
class incoming_relationship_facade_t;
class outgoing_relationship_facade_t;

class database_facade_t
{
public:
    explicit database_facade_t(gaia_database_t database)
        : m_database(std::move(database)){};

    std::string database_name() const;
    bool is_default_database();
    std::vector<table_facade_t> tables() const;

private:
    gaia_database_t m_database;
};

class table_facade_t
{
public:
    explicit table_facade_t(gaia_table_t table)
        : m_table(std::move(table)){};

    std::string table_name() const;
    std::string table_type() const;
    std::string class_name() const;
    std::vector<field_facade_t> fields() const;
    std::vector<incoming_relationship_facade_t> incoming_relationships() const;
    std::vector<outgoing_relationship_facade_t> outgoing_relationships() const;
    bool has_string_or_vector() const;

private:
    gaia_table_t m_table;
};

class field_facade_t
{
public:
    explicit field_facade_t(gaia_field_t field)
        : m_field(std::move(field)){};

    std::string field_name() const;
    std::string field_type(bool is_function_parameter = false) const;
    std::string table_name() const;
    std::pair<std::string, std::string> generate_expr_variable() const;
    bool is_string() const;
    bool is_vector() const;

    // Generates code for static initialization of EDC expr variables for C++11 compliance.
    // pair.first = declaration
    // pair.second = initialization
    static std::pair<std::string, std::string> generate_expr_variable(
        const std::string& table, const std::string& type, const std::string& field);

private:
    gaia_field_t m_field;
};

class relationship_facade_t
{
public:
    explicit relationship_facade_t(gaia_relationship_t relationship)
        : m_relationship(std::move(relationship)){};

    std::string parent_table() const;
    std::string child_table() const;
    bool is_one_to_many() const;
    bool is_one_to_one() const;

protected:
    gaia_relationship_t m_relationship;
};

class incoming_relationship_facade_t : public relationship_facade_t
{
public:
    explicit incoming_relationship_facade_t(gaia_relationship_t relationship)
        : relationship_facade_t(std::move(relationship)){};

    std::string field_name() const;
    std::string target_type() const;
    std::string parent_offset() const;
    std::string parent_offset_value() const;
    std::string next_offset() const;
    std::string next_offset_value() const;
};

class outgoing_relationship_facade_t : public relationship_facade_t
{
public:
    explicit outgoing_relationship_facade_t(gaia_relationship_t relationship)
        : relationship_facade_t(std::move(relationship)){};

    std::string field_name() const;
    std::string target_type() const;
    std::string first_offset() const;
    std::string first_offset_value() const;
    std::string next_offset() const;
};

} // namespace generate
} // namespace catalog
} // namespace gaia
