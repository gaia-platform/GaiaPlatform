/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

#include "gaia_internal/common/retail_assert.hpp"

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

// Catalog's notion for the empty database similar to Epsilon for the empty
// string. Specifically, when a user create a table without specifying a
// database, it is created in this construct. Users cannot use '()' in database
// names so there will be no ambiguity, i.e. there will never exist a user
// created database called "()".
const std::string c_empty_db_name = "()";

// The character used to connect a database name and a table name to form fully
// qualified name for a table defined in a given database.
constexpr char c_db_table_name_connector = '.';

const std::string c_catalog_db_name = "catalog";
const std::string c_event_log_db_name = "event_log";

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

class forbidden_system_db_operation : public gaia::common::gaia_exception
{
public:
    explicit forbidden_system_db_operation(const std::string& name)
    {
        m_message = "Operations on the system database '" + name + "' are not allowed.";
    }
};

/**
 * Thrown when creating a database that already exists.
 */
class db_already_exists : public gaia::common::gaia_exception
{
public:
    explicit db_already_exists(const std::string& name)
    {
        std::stringstream message;
        message << "The database '" << name << "' already exists.";
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
        message << "The database '" << name << "' does not exist.";
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
        message << "The table '" << name << "' already exists.";
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
        message << "The table '" << name << "' does not exist.";
        m_message = message.str();
    }
};

/**
 * Thrown when a specified field does not exists.
 */
class field_not_exists : public gaia::common::gaia_exception
{
public:
    explicit field_not_exists(const std::string& name)
    {
        std::stringstream message;
        message << "The field \"" << name << "\" does not exist.";
        m_message = message.str();
    }
};

/**
 * Thrown when a field is specified more than once.
 */
class duplicate_field : public gaia::common::gaia_exception
{
public:
    explicit duplicate_field(const std::string& name)
    {
        std::stringstream message;
        message << "The field '" << name << "' is specified more than once.";
        m_message = message.str();
    }
};

/**
 * Thrown when the maximum number of references has been reached for a type.
 */
class max_reference_count_reached : public gaia::common::gaia_exception
{
public:
    explicit max_reference_count_reached()
    {
        m_message = "Cannot add any more relationships because the maximum number of references has been reached!";
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
        message
            << "Cannot drop table '" << parent_table
            << "' because it is referenced by table '" << child_table << "'.";
        return referential_integrity_violation{message.str()};
    }
};

/**
 * Thrown when creating a relationship that already exists.
 */
class relationship_already_exists : public gaia::common::gaia_exception
{
public:
    explicit relationship_already_exists(const std::string& name)
    {
        std::stringstream message;
        message << "The relationship '" << name << "' already exists.";
        m_message = message.str();
    }
};

/**
 * Thrown when a relationship not exists.
 */
class relationship_not_exists : public gaia::common::gaia_exception
{
public:
    explicit relationship_not_exists(const std::string& name)
    {
        std::stringstream message;
        message << "The relationship '" << name << "' does not exist.";
        m_message = message.str();
    }
};

/**
 * Thrown when the tables specified in the relationship definition do not match.
 */
class tables_not_match : public gaia::common::gaia_exception
{
public:
    explicit tables_not_match(
        const std::string& relationship,
        const std::string& name1,
        const std::string& name2)
    {
        std::stringstream message;
        message << "The table '" << name1 << "' does not match the table '" << name2 << "' "
                << "in the relationship '" << relationship << "' definition.";
        m_message = message.str();
    }
};

/**
 * Thrown when trying to create a many-to-many relationship.
 */
class many_to_many_not_supported : public gaia::common::gaia_exception
{
public:
    explicit many_to_many_not_supported(const std::string& relationship)
    {
        std::stringstream message;
        message << "The many to many relationship defined in '" << relationship << "' is not supported.";
        m_message = message.str();
    }

    explicit many_to_many_not_supported(const std::string& table1, const std::string& table2)
    {
        std::stringstream message;
        message << "The many to many relationship defined "
                << "in '" << table1 << "'  and '" << table2 << "' is not supported.";
        m_message = message.str();
    }
};

/**
 * Thrown when creating an index that already exists.
 */
class index_already_exists : public gaia::common::gaia_exception
{
public:
    explicit index_already_exists(const std::string& name)
    {
        std::stringstream message;
        message << "The index '" << name << "' already exists.";
        m_message = message.str();
    }
};

/**
 * Thrown when the index of the given name does not exists.
 */
class index_not_exists : public gaia::common::gaia_exception
{
public:
    explicit index_not_exists(const std::string& name)
    {
        std::stringstream message;
        message << "The index '" << name << "' does not exist.";
        m_message = message.str();
    }
};

/**
 * Thrown when the field map is invalid.
 */
class invalid_field_map : public gaia::common::gaia_exception
{
public:
    explicit invalid_field_map(const std::string& message)
    {
        m_message = message;
    }
};

/**
 * Thrown when the `references` definition can match multiple `references`
 * definitions elsewhere.
 */
class ambiguous_reference_definition : public gaia::common::gaia_exception
{
public:
    explicit ambiguous_reference_definition(const std::string& table, const std::string& ref_name)
    {
        std::stringstream message;
        message << "The reference '" << ref_name << "' definition "
                << "in table '" << table << "' has mutiple matching definitions.";
        m_message = message.str();
    }
};

/**
 * Thrown when the `references` definition does not have a matching definition.
 */
class orphaned_reference_definition : public gaia::common::gaia_exception
{
public:
    explicit orphaned_reference_definition(const std::string& table, const std::string& ref_name)
    {
        std::stringstream message;
        message << "The reference '" << ref_name << "' definition "
                << "in table '" << table << "' has no matching definition.";
        m_message = message.str();
    }
};

/**
 * Thrown when the create list is invalid.
 */
class invalid_create_list : public gaia::common::gaia_exception
{
public:
    explicit invalid_create_list(const std::string& message)
    {
        m_message = "Invalid create statment in a list: ";
        m_message += message;
    }
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
            }
        }
    }

    data_type_t data_type;

    uint16_t length;

    bool active = false;

    bool unique = false;
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
 * @throw db_not_exists
 */
void use_database(const std::string& name);

/**
 * Create a database in the catalog.
 *
 * @param name database name
 * @return id of the new database
 * @throw db_already_exists
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
 * @throw table_already_exists
 */
gaia::common::gaia_id_t create_table(
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
 * @throw table_already_exists
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
 * @throw db_not_exists
 * @throw table_not_exists
 * @throw field_not_exists
 * @throw duplicate_field
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
 * @throw table_not_exists
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
 * @throw table_not_exists
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
 * @throw table_not_exists
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
 * @throw relationship_not_exists
 */
void drop_relationship(const std::string& name, bool throw_unless_exists = true);

/**
 * Delete a given index.
 *
 * @param name of the index
 * @param throw_unless_exists throw an execption unless the index exists
 * @throw index_not_exists
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
 * Generate the Extended Data Classes header file.
 *
 * @param dbname database name
 * @return generated source
 */
std::string generate_edc_header(const std::string& dbname);

/**
 * Generate the Extended Data Classes implementation file.
 *
 * @param dbname database name
 * @param header_file_name name of the corresponding header to
 *        include at the beginning of the file
 * @return generated source
 */
std::string generate_edc_cpp(const std::string& dbname, const std::string& header_file_name);

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

/*@}*/
} // namespace catalog
/*@}*/
} // namespace gaia
