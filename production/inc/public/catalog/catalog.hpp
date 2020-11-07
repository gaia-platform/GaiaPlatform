/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "gaia_common.hpp"
#include "gaia_exception.hpp"

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace catalog
{
/**
 * \addtogroup catalog
 * @{
 */

// The top level namespace for all the Gaia generated code.
const std::string c_gaia_namespace = "gaia";

// Catalog's notion for the empty database similar to Epsilon for the empty
// string. Specifically, when a user create a table without specifying a
// database, it is created in this construct. Users cannot use '()' in database
// names so there will be no ambiguity, i.e. there will never exist a user
// created database called "()".
const std::string c_empty_db_name = "()";

// The character used to connect a database name and a table name to form fully
// qualified name for a table defined in a given database.
constexpr char c_db_table_name_connector = '.';

/*
 * The following enum classes are shared cross the catalog usage.
 */

/*
 * Data types for Gaia field records.
 */

using data_type_t = gaia::common::data_type_t;

/**
 * Get the data type name
 *
 * @param catalog data type
 * @return fbs data type name
 * @throw unknown_data_type
 */
std::string get_data_type_name(data_type_t data_type);

/*
 * Trim action for log tables.
 */
enum class trim_action_type_t : uint8_t
{
    e_none,
    e_delete,
    e_archive,
};

/*
 * Value index types.
 */
enum value_index_type_t : uint8_t
{
    hash,
    range
};

/*
 * Cardinality of a relationship
 */
enum relationship_cardinality_t : uint8_t
{
    one,
    many
};

namespace ddl
{
/**
 * \addtogroup ddl
 * @{
 *
 * Definitions for parse result bindings
 */

enum class statement_type_t : uint8_t
{
    create,
    drop,
    alter
};

struct statement_t
{
    explicit statement_t(statement_type_t type)
        : m_type(type){};

    [[nodiscard]] statement_type_t type() const
    {
        return m_type;
    };

    [[nodiscard]] bool is_type(statement_type_t type) const
    {
        return m_type == type;
    };

    virtual ~statement_t() = default;

private:
    statement_type_t m_type;
};

enum class field_type_t : uint8_t
{
    data,
    reference
};

struct field_def_t
{
    field_def_t(std::string name, field_type_t field_type)
        : name(move(name)), field_type(field_type)
    {
    }
    std::string name;
    field_type_t field_type;

    virtual ~field_def_t() = default;
};

struct data_field_def_t : field_def_t
{
    data_field_def_t(std::string name, data_type_t type, uint16_t length)
        : field_def_t(name, field_type_t::data), data_type(type), length(length)
    {
    }

    data_type_t data_type;
    uint16_t length;

    bool active = false;
};

using composite_name_t = std::pair<std::string, std::string>;

struct ref_field_def_t : field_def_t
{
    ref_field_def_t(std::string name, composite_name_t full_table_name)
        : field_def_t(name, field_type_t::reference), parent_table(move(full_table_name))
    {
    }

    composite_name_t parent_table;

    [[nodiscard]] std::string db_name() const
    {
        return parent_table.first;
    }

    [[nodiscard]] std::string table_name() const
    {
        return parent_table.second;
    }

    [[nodiscard]] std::string full_name() const
    {
        if (db_name().empty())
        {
            return table_name();
        }
        else
        {
            return db_name() + c_db_table_name_connector + table_name();
        }
    }
};

using field_def_list_t = std::vector<std::unique_ptr<field_def_t>>;

enum class create_type_t : uint8_t
{
    create_database,
    create_table,
};

struct create_statement_t : statement_t
{
    explicit create_statement_t(create_type_t type)
        : statement_t(statement_type_t::create), type(type)
    {
    }

    create_statement_t(create_type_t type, std::string name)
        : statement_t(statement_type_t::create), type(type), name(std::move(name))
    {
    }

    create_type_t type;

    std::string name;

    std::string database;

    field_def_list_t fields;

    bool if_not_exists;
};

enum class drop_type_t : uint8_t
{
    drop_table,
    drop_database,
};

struct drop_statement_t : statement_t
{
    explicit drop_statement_t(drop_type_t type)
        : statement_t(statement_type_t::drop), type(type)
    {
    }

    drop_statement_t(drop_type_t type, std::string name)
        : statement_t(statement_type_t::drop), type(type), name(std::move(name))
    {
    }

    drop_type_t type;

    std::string name;

    std::string database;
};

/*@}*/
} // namespace ddl

/**
 * Thrown when creating a database that already exists.
 */
class db_already_exists : public gaia::common::gaia_exception
{
public:
    explicit db_already_exists(const std::string& name)
    {
        std::stringstream message;
        message << "The database \"" << name << "\" already exists.";
        m_message = message.str();
    }
};

/**
 * Thrown when a specified database does not exists.
 */
class db_not_exists : public gaia::common::gaia_exception
{
public:
    explicit db_not_exists(const std::string& name)
    {
        std::stringstream message;
        message << "The database \"" << name << "\" does not exist.";
        m_message = message.str();
    }
};

/**
 * Thrown when creating a table that already exists.
 */
class table_already_exists : public gaia::common::gaia_exception
{
public:
    explicit table_already_exists(const std::string& name)
    {
        std::stringstream message;
        message << "The table \"" << name << "\" already exists.";
        m_message = message.str();
    }
};

/**
 * Thrown when a specified table does not exists.
 */
class table_not_exists : public gaia::common::gaia_exception
{
public:
    explicit table_not_exists(const std::string& name)
    {
        std::stringstream message;
        message << "The table \"" << name << "\" does not exist.";
        m_message = message.str();
    }
};

/**
 * Thrown when a field is specified more than once
 */
class duplicate_field : public gaia::common::gaia_exception
{
public:
    explicit duplicate_field(const std::string& name)
    {
        std::stringstream message;
        message << "The field \"" << name << "\" is specified more than once.";
        m_message = message.str();
    }
};

class referential_integrity_violation : public gaia::common::gaia_exception
{
public:
    explicit referential_integrity_violation(const std::string& message)
    {
        m_message = message;
    }

    static referential_integrity_violation drop_parent_table(
        const std::string& parent_table,
        const std::string& child_table)
    {
        std::stringstream message;
        message << "Cannot drop table \"" << parent_table << "\" because it is referenced by \"" << child_table << "\"";
        return referential_integrity_violation{message.str()};
    }
};

/**
 * Initialize the catalog.
*/
void initialize_catalog();

/**
 * Create a database in the catalog.
 *
 * @param name database name
 * @return id of the new database
 * @throw db_already_exists
 */
gaia::common::gaia_id_t create_database(const std::string& name, bool throw_on_exists = true);

/**
 * Create a table definition in a given database.
 *
 * @param name database name
 * @param name table name
 * @param fields fields of the table
 * @return id of the new table
 * @throw table_already_exists
 */
gaia::common::gaia_id_t create_table(
    const std::string& db_name, const std::string& name, const ddl::field_def_list_t& fields, bool throw_on_exist = true);

/**
 * Create a table definition in the catalog's global database.
 *
 * @param name table name
 * @param fields fields of the table
 * @return id of the new table
 * @throw table_already_exists
 */
gaia::common::gaia_id_t create_table(const std::string& name, const ddl::field_def_list_t& fields);

/**
 * Delete a database.
 *
 * This method will only remove the database definition from the catalog. There
 * is no way to remove the database's data, other than through the direct access
 * API, which is not available to the catalog implementation.
 *
 * @param name database name
 * @param name table name
 * @throw table_not_exists
 */
void drop_database(const std::string& name);

/**
 * Delete a table in a given database.
 *
 * This method will only remove the table definition from the catalog. There is
 * no way to delete the table's data, other than through the direct access API,
 * which is not available to the catalog implementation.
 *
 * @param db_name database name
 * @param name table name
 * @throw table_not_exists
 */
void drop_table(const std::string& db_name, const std::string& name);

/**
 * Delete a table from the catalog's global database.
 *
 * This method will only remove the table definition from the catalog. There is
 * no way to delete the table's data, other than through the direct access API,
 * which is not available to the catalog implementation.
 *
 * @param name table name
 * @throw table_not_exists
 */
void drop_table(const std::string& name);

/**
 * List all data payload fields for a given table defined in the catalog.
 * The fields returned here do not include references type fields.
 * Reference type fields defines foreign key relationship between tables.
 * They are not part of the data payload and are ordered separately.
 *
 * Use list_references() API to get a list of all references for a given table.
 * The method needs to be called inside a transaction.
 *
 * @param table_id id of the table
 * @return a list of field ids in the order of their positions.
 */
std::vector<gaia::common::gaia_id_t> list_fields(gaia::common::gaia_id_t table_id);

/**
 * List all references for a given table defined in the catalog.
 * References are foreign key constraints or table links.
 * They defines relationships between tables.
 *
 * The method needs to be called inside a transaction.
 *
 * @param table_id id of the table
 * @return a list of ids of the table references in the order of their positions.
 */
std::vector<gaia::common::gaia_id_t> list_references(gaia::common::gaia_id_t table_id);

/**
 * List all the tables that have a relationship with the given table where the
 * given table is the parent side of the relationship.
 *
 * @param table_id id of the table
 * @return a list of ids of the tables that have a child relationship with this table.
 */
std::vector<gaia::common::gaia_id_t> list_parent_relationships(gaia::common::gaia_id_t table_id);

/**
 * List all the tables that have a relationship with the given table where the
 * given table is the child side of the relationship.
 *
 * @param table_id id of the table
 * @return a list of ids of the tables that have a parent relationship with this table.
 */
std::vector<gaia::common::gaia_id_t> list_child_relationships(gaia::common::gaia_id_t table_id);

/**
 * Generate the Extended Data Classes header file.
 *
 * @param dbname database name
 * @return generated source
 */
std::string gaia_generate(const std::string& dbname);

/**
 * Find the database id given its name
 *
 * @param dbname database name
 * @return database id (or INVALID_ID if the db name does not exist)
 */
gaia::common::gaia_id_t find_db_id(const std::string& dbname);

/*@}*/
} // namespace catalog
/*@}*/
} // namespace gaia
