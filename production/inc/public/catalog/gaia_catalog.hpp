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
#include <vector>

#include "gaia_common.hpp"
#include "gaia_exception.hpp"

namespace gaia {
/**
 * \addtogroup Gaia
 * @{
 */
namespace catalog {
/**
 * \addtogroup catalog
 * @{
 */

// The top level namespace for all the Gaia generated code.
const string c_gaia_namespace = "gaia";

// Catalog's notion for the empty database similar to Epsilon for the empty
// string. Specifically, when a user create a table without specifying a
// database, it is created in this construct. Users cannot use '()' in database
// names so there will be no ambiguity, i.e. there will never exist a user
// created database called "()".
const string c_empty_db_name = "()";

/*
 * The following enum classes are shared cross the catalog usage.
 */

/*
 * Data types for Gaia field records.
 */
enum class data_type_t : uint8_t {
    e_bool,
    e_int8,
    e_uint8,
    e_int16,
    e_uint16,
    e_int32,
    e_uint32,
    e_int64,
    e_uint64,
    e_float,
    e_double,
    e_string,
    e_references
};

/**
 * Thrown when seeing an unknown data type
 */
class unknown_data_type : public gaia::common::gaia_exception {
public:
    unknown_data_type();
};

/**
 * Get the data type name
 *
 * @param catalog data type
 * @return fbs data type name
 * @throw unknown_data_type
 */
string get_data_type_name(data_type_t data_type);

/*
 * Trim action for log tables.
 */
enum class trim_action_type_t : uint8_t {
    e_none,
    e_delete,
    e_archive,
};

/*
 * Value index types.
 */
enum value_index_type_t : uint8_t {
    hash,
    range
};

namespace ddl {
/**
 * \addtogroup ddl
 * @{
 *
 * Definitions for parse result bindings
 */

enum class statement_type_t : uint8_t {
    create,
    drop,
    alter
};

struct statement_t {

    statement_t(statement_type_t type) : m_type(type){};

    statement_type_t type() const { return m_type; };

    bool is_type(statement_type_t type) const { return m_type == type; };

    virtual ~statement_t(){};

private:
    statement_type_t m_type;
};

struct field_type_t {
    field_type_t(data_type_t type) : type(type){};

    data_type_t type;
    string name;
};

struct field_definition_t {
    field_definition_t(string name, data_type_t type, uint16_t length)
        : name(move(name)), type(type), length(length){};

    field_definition_t(string name, data_type_t type, uint16_t length, string referenced_table_name)
        : name(move(name)), type(type), length(length), table_type_name(move(referenced_table_name)){};

    string name;
    data_type_t type;
    uint16_t length;

    string table_type_name;
    string table_type_database;
    bool active = false;
};

using field_def_list_t = vector<unique_ptr<field_definition_t>>;

enum class create_type_t : uint8_t {
    create_database,
    create_table,
};

struct create_statement_t : statement_t {
    create_statement_t(create_type_t type)
        : statement_t(statement_type_t::create), type(type){};

    create_statement_t(create_type_t type, string name)
        : statement_t(statement_type_t::create), type(type), name(move(name)){};

    virtual ~create_statement_t() {}

    create_type_t type;

    string name;

    string database;

    field_def_list_t fields;

    bool if_not_exists;
};

enum class drop_type_t : uint8_t {
    drop_table,
    drop_database,
};

struct drop_statement_t : statement_t {
    drop_statement_t(drop_type_t type)
        : statement_t(statement_type_t::drop), type(type){};

    drop_statement_t(drop_type_t type, string name)
        : statement_t(statement_type_t::drop), type(type), name(move(name)){};

    virtual ~drop_statement_t() {}

    drop_type_t type;

    string name;

    string database;
};

/*@}*/
} // namespace ddl

/**
 * Thrown when creating a database that already exists.
 */
class db_already_exists : public gaia::common::gaia_exception {
public:
    db_already_exists(const string& name) {
        stringstream message;
        message << "The database \"" << name << "\" already exists.";
        m_message = message.str();
    }
};

/**
 * Thrown when a specified database does not exists.
 */
class db_not_exists : public gaia::common::gaia_exception {
public:
    db_not_exists(const string& name) {
        stringstream message;
        message << "The database \"" << name << "\" does not exist.";
        m_message = message.str();
    }
};

/**
 * Thrown when creating a table that already exists.
 */
class table_already_exists : public gaia::common::gaia_exception {
public:
    table_already_exists(const string& name) {
        stringstream message;
        message << "The table \"" << name << "\" already exists.";
        m_message = message.str();
    }
};

/**
 * Thrown when a specified table does not exists.
 */
class table_not_exists : public gaia::common::gaia_exception {
public:
    table_not_exists(const string& name) {
        stringstream message;
        message << "The table \"" << name << "\" does not exist.";
        m_message = message.str();
    }
};

/**
 * Thrown when a field is specified more than once
 */
class duplicate_field : public gaia::common::gaia_exception {
public:
    duplicate_field(const string& name) {
        stringstream message;
        message << "The field \"" << name << "\" is specified more than once.";
        m_message = message.str();
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
gaia::common::gaia_id_t create_database(const string& name, bool throw_on_exists = true);

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
    const string& db_name, const string& name,
    const ddl::field_def_list_t& fields, bool throw_on_exist = true);

/**
 * Create a table definition in the catalog's global database.
 *
 * @param name table name
 * @param fields fields of the table
 * @return id of the new table
 * @throw table_already_exists
 */
gaia::common::gaia_id_t create_table(const string& name, const ddl::field_def_list_t& fields);

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
void drop_database(const string& name);

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
void drop_table(const string& db_name, const string& name);

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
void drop_table(const string& name);

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
vector<gaia::common::gaia_id_t> list_fields(gaia::common::gaia_id_t table_id);

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
vector<gaia::common::gaia_id_t> list_references(gaia::common::gaia_id_t table_id);

/**
 * Generate FlatBuffers schema (fbs) for a catalog table.
 * The given table is the root type of the generated schema.
 *
 * @return generated fbs string
 */
string generate_fbs(gaia::common::gaia_id_t table_id);

/**
 * Generate FlatBuffers schema (fbs) for all catalog tables in a given database.
 * No root type is specified in the generated schema.
 *
 * @param dbname database name
 * @return generated fbs string
 */
string generate_fbs(const string& dbname);

/**
 * Generate the Extended Data Classes header file.
 *
 * @param dbname database name
 * @return generated source
 */
string gaia_generate(const string& dbname);

/**
 * Retrieve the binary FlatBuffers schema (bfbs) for a given table.
 *
 * @param table_id id of the table
 * @return bfbs
 */
string get_bfbs(gaia::common::gaia_id_t table_id);

/**
 * Find the database id given its name
 *
 * @param dbname database name
 * @return database id (or INVALID_ID if the db name does not exist)
 */
gaia::common::gaia_id_t find_db_id(const string& dbname);

/*@}*/
} // namespace catalog
/*@}*/
} // namespace gaia
