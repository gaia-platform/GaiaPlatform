/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/exceptions.hpp"

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

// Last element of the namespace for all Gaia generated code that is not meant
// for public usage. This helps avoiding conflicts between generated names
// and application code.
const std::string c_internal_suffix = "internal";

// When users create tables without specifying a database, the tables are
// created in the following database.
const std::string c_empty_db_name = "";

// The character used to connect a database name and a table name to form fully
// qualified name for a table defined in a given database.
constexpr char c_db_table_name_connector = '.';

const std::string c_catalog_db_name = "catalog";
const std::string c_event_log_db_name = "event_log";
const std::string c_event_log_table_name = "event_log";
const std::string c_gaia_database_table_name = "gaia_database";
const std::string c_gaia_table_table_name = "gaia_table";
const std::string c_gaia_field_table_name = "gaia_field";
const std::string c_gaia_relationship_table_name = "gaia_relationship";
const std::string c_gaia_index_table_name = "gaia_index";
const std::string c_gaia_ref_anchor_table_name = "gaia_ref_anchor";
const std::string c_gaia_ruleset_table_name = "gaia_ruleset";
const std::string c_gaia_rule_table_name = "gaia_rule";

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
enum class index_type_t : uint8_t
{
    hash,
    range,
};

/*
 * Cardinality of a relationship
 */
enum class relationship_cardinality_t : uint8_t
{
    one,
    many,
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
    alter,
    use,
    create_list,
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

enum class constraint_type_t : uint8_t
{
    active,
    unique,
    optional,
};

struct constraint_t
{
    explicit constraint_t(constraint_type_t type)
        : type(type)
    {
    }

    constraint_type_t type;
};

struct active_constraint_t : constraint_t
{
    explicit active_constraint_t()
        : constraint_t(constraint_type_t::active)
    {
    }
};

struct unique_constraint_t : constraint_t
{
    explicit unique_constraint_t()
        : constraint_t(constraint_type_t::unique)
    {
    }
};

struct optional_constraint_t : constraint_t
{
    explicit optional_constraint_t()
        : constraint_t(constraint_type_t::optional)
    {
    }
};

using constraint_list_t = std::vector<std::unique_ptr<constraint_t>>;

enum class field_type_t : uint8_t
{
    data,
    reference,
};

struct base_field_def_t
{
    base_field_def_t(std::string name, field_type_t field_type)
        : name(move(name)), field_type(field_type)
    {
        ASSERT_PRECONDITION(!(this->name.empty()), "base_field_def_t::name must not be empty.");
    }

    std::string name;
    field_type_t field_type;

    virtual ~base_field_def_t() = default;
};

struct data_field_def_t : base_field_def_t
{
    data_field_def_t(std::string name, data_type_t type, uint16_t length)
        : base_field_def_t(name, field_type_t::data), data_type(type), length(length)
    {
    }

    data_field_def_t(
        std::string name,
        data_type_t type,
        uint16_t length,
        const std::optional<constraint_list_t>& opt_constraint_list)
        : base_field_def_t(name, field_type_t::data), data_type(type), length(length)
    {
        if (opt_constraint_list)
        {
            ASSERT_INVARIANT(opt_constraint_list.value().size() > 0, "The constraint list should not be empty.");

            for (const auto& constraint : opt_constraint_list.value())
            {
                if (constraint->type == constraint_type_t::active)
                {
                    this->active = true;
                }
                else if (constraint->type == constraint_type_t::unique)
                {
                    this->unique = true;
                }
                else if (constraint->type == constraint_type_t::optional)
                {
                    this->optional = true;
                }
            }
        }
    }

    data_type_t data_type;

    uint16_t length;

    bool active = false;

    bool unique = false;

    bool optional = false;
};

using composite_name_t = std::pair<std::string, std::string>;

using field_def_list_t = std::vector<std::unique_ptr<base_field_def_t>>;

struct link_def_t
{
    std::string from_database;
    std::string from_table;

    std::string name;

    std::string to_database;
    std::string to_table;

    relationship_cardinality_t cardinality;
};

struct table_field_list_t
{
    std::string database;
    std::string table;
    std::vector<std::string> fields;

    bool operator==(const table_field_list_t& other) const
    {
        if (database == other.database && table == other.table && fields == other.fields)
        {
            return true;
        }
        return false;
    }
};

using table_field_map_t = std::pair<table_field_list_t, table_field_list_t>;

// The class represents reference fields in parsing results. The matching
// references in the two corresponding table definitions will be translated to
// relationship definitions during execution.
struct ref_field_def_t : base_field_def_t
{
    ref_field_def_t(std::string name, std::string table_name)
        : base_field_def_t(name, field_type_t::reference), table(std::move(table_name))
    {
    }

    // The referenced table.
    std::string table;

    relationship_cardinality_t cardinality;

    // Optional, the field name of the referenced table.
    std::string field;

    // Optional, see the `create_relationship_t` for more details.
    std::optional<table_field_map_t> field_map;
};

enum class create_type_t : uint8_t
{
    create_database,
    create_table,
    create_relationship,
    create_index,
};

struct use_statement_t : statement_t
{
    explicit use_statement_t(std::string name)
        : statement_t(statement_type_t::use), name(std::move(name))
    {
    }

    std::string name;
};

struct create_statement_t : statement_t
{
    create_statement_t(create_type_t type, std::string name)
        : statement_t(statement_type_t::create), type(type), name(std::move(name))
    {
        has_if_not_exists = false;
        auto_drop = false;
    }

    create_type_t type;

    std::string name;

    bool has_if_not_exists;

    bool auto_drop;
};

struct create_database_t : create_statement_t
{
    explicit create_database_t(std::string name)
        : create_statement_t(create_type_t::create_database, name)
    {
    }
};

struct create_table_t : create_statement_t
{
    explicit create_table_t(std::string name)
        : create_statement_t(create_type_t::create_table, name)
    {
    }

    std::string database;

    field_def_list_t fields;
};

struct create_relationship_t : create_statement_t
{
    explicit create_relationship_t(std::string name)
        : create_statement_t(create_type_t::create_relationship, name)
    {
    }

    // A relationship is defined by a pair of links because we only allow
    // bi-directional relationships.
    std::pair<link_def_t, link_def_t> relationship;

    // Track the optional mapping from one table's field(s) to the other table's
    // field(s). The field map must be bijective from one table to the other,
    // and the field types must match. When defined, the relationship will be
    // established using the fields instead of the two tables' `gaia_id`s.
    std::optional<table_field_map_t> field_map;
};

struct create_index_t : create_statement_t
{
    explicit create_index_t(std::string name)
        : create_statement_t(create_type_t::create_index, name)
    {
    }

    std::string database;

    bool unique_index;

    index_type_t index_type;

    std::string index_table;

    std::vector<std::string> index_fields;
};

struct create_list_t : statement_t
{
    explicit create_list_t()
        : statement_t(statement_type_t::create_list)
    {
    }

    std::vector<std::unique_ptr<gaia::catalog::ddl::create_statement_t>> statements;
};

enum class drop_type_t : uint8_t
{
    drop_table,
    drop_database,
    drop_relationship,
    drop_index,
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

    bool if_exists;
};

/*@}*/
} // namespace ddl

/**
 * Initialize the catalog.
 */
void initialize_catalog();

/**
 * Switch to the database.
 *
 * @param name database name
 * @throw db_does_not_exist_internal
 */
void use_database(const std::string& name);

/**
 * Create a database in the catalog.
 *
 * @param name database name
 * @return id of the new database
 * @throw db_already_exists_internal
 */
gaia::common::gaia_id_t create_database(
    const std::string& name,
    bool throw_on_exists = true,
    bool auto_drop = false);

/**
 * Create a table definition in a given database.
 *
 * @param name database name
 * @param name table name
 * @param fields fields of the table
 * @return id of the new table
 * @throw table_already_exists_internal
 */
gaia::common::gaia_id_t
create_table(
    const std::string& db_name,
    const std::string& name,
    const ddl::field_def_list_t& fields,
    bool throw_on_exists = true,
    bool auto_drop = false);

/**
 * Create a table definition in the catalog's global database.
 *
 * @param name table name
 * @param fields fields of the table
 * @return id of the new table
 * @throw table_already_exists_internal
 */
gaia::common::gaia_id_t create_table(const std::string& name, const ddl::field_def_list_t& fields);

/**
 * Create an index
 *
 * @param name index name
 * @param unique indicator if the index is unique
 * @param type index type
 * @param db_name database name of the table to be indexed
 * @param table_name name of the table to be indexed
 * @param field_names name of the table fields to be indexed
 * @return id of the new index
 * @throw db_does_not_exist_internal
 * @throw table_does_not_exist_internal
 * @throw field_does_not_exist_internal
 * @throw duplicate_field_internal
 */
gaia::common::gaia_id_t create_index(
    const std::string& index_name,
    bool unique,
    index_type_t type,
    const std::string& db_name,
    const std::string& table_name,
    const std::vector<std::string>& field_names,
    bool throw_on_exist = true,
    bool auto_drop = false);

/**
 * Delete a database.
 *
 * This method will only remove the database definition from the catalog. There
 * is no way to remove the database's data, other than through the direct access
 * API, which is not available to the catalog implementation.
 *
 * @param name database name
 * @param name table name
 * @param throw_unless_exists throw an execption unless the database exists
 * @throw table_does_not_exist_internal
 */
void drop_database(const std::string& name, bool throw_unless_exists = true);

/**
 * Delete a table in a given database.
 *
 * This method will only remove the table definition from the catalog. There is
 * no way to delete the table's data, other than through the direct access API,
 * which is not available to the catalog implementation.
 *
 * @param db_name database name
 * @param name table name
 * @param throw_unless_exists throw an execption unless the table exists
 * @throw table_does_not_exist_internal
 */
void drop_table(const std::string& db_name, const std::string& name, bool throw_unless_exists = true);

/**
 * Delete a table from the catalog's global database.
 *
 * This method will only remove the table definition from the catalog. There is
 * no way to delete the table's data, other than through the direct access API,
 * which is not available to the catalog implementation.
 *
 * @param name table name
 * @param throw_unless_exists throw an execption unless the table exists
 * @throw table_does_not_exist_internal
 */
void drop_table(const std::string& name, bool throw_unless_exists = true);

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
 * Create a relationship between tables given the link definitions.
 *
 * @param name of the relationship
 * @param link1, link2 link definitions of the relationship
 * @return gaia id of the created relationship
 */
gaia::common::gaia_id_t create_relationship(
    const std::string& name,
    const ddl::link_def_t& link1,
    const ddl::link_def_t& link2,
    bool throw_on_exist = true,
    bool auto_drop = false);

/**
 * Create a relationship between tables.
 *
 * @param name of the relationship
 * @param link1, link2 link definitions of the relationship
 * @param field_map field map of the relationship
 * @return gaia id of the created relationship
 */
gaia::common::gaia_id_t create_relationship(
    const std::string& name,
    const ddl::link_def_t& link1,
    const ddl::link_def_t& link2,
    const std::optional<ddl::table_field_map_t>& field_map,
    bool throw_on_exist,
    bool auto_drop);

/**
 * Delete a given relationship.
 *
 * @param name of the relationship
 * @param throw_unless_exists throw an execption unless the relationship exists
 * @throw relationship_does_not_exist_internal
 */
void drop_relationship(const std::string& name, bool throw_unless_exists = true);

/**
 * Delete a given index.
 *
 * @param name of the index
 * @param throw_unless_exists throw an execption unless the index exists
 * @throw index_does_not_exist_internal
 */
void drop_index(const std::string& name, bool throw_unless_exists = true);

/**
 * Find the database id given its name
 *
 * @param dbname database name
 * @return database id (or INVALID_ID if the db name does not exist)
 */
gaia::common::gaia_id_t find_db_id(const std::string& dbname);

/**
 * Generate the Direct Access Classes header file.
 *
 * @param dbname database name
 * @return generated source
 */
std::string generate_dac_header(const std::string& dbname);

/**
 * Generate the Direct Access Classes implementation file.
 *
 * @param dbname database name
 * @param header_file_name name of the corresponding header to
 *        include at the beginning of the file
 * @return generated source
 */
std::string generate_dac_cpp(const std::string& dbname, const std::string& header_file_name);

/**
 * Generate FlatBuffers schema (fbs) for all catalog tables in a given database.
 * No root type is specified in the generated schema.
 *
 * @param dbname database name
 * @return generated fbs string
 */
std::string generate_fbs(const std::string& dbname);

/**
 * Generate a foreign table DDL from parsing the definition of a table.
 *
 * @param table_id table id
 * @param server_name FDW server name
 * @return DDL string
 */
std::string generate_fdw_ddl(
    common::gaia_id_t table_id, const std::string& server_name);

inline void check_not_system_db(const std::string& name)
{
    if (name == c_catalog_db_name || name == c_event_log_db_name)
    {
        throw forbidden_system_db_operation_internal(name);
    }
}

/*@}*/
} // namespace catalog
/*@}*/
} // namespace gaia
