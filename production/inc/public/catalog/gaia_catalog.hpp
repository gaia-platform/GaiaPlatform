/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_object.hpp"
#include "gaia_exception.hpp"
#include <string>
#include <memory>

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
    e_float32,
    e_float64,
    e_string,
    e_references
};

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
    e_hash,
    e_range
};

namespace ddl {
/**
 * \addtogroup ddl
 * @{
 *
 * Definitions for parse result bindings
 */

enum class statement_type_t : uint8_t {
    e_create,
    e_drop,
    e_alter
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
};

using field_def_list_t = vector<unique_ptr<field_definition_t>>;

enum class create_type_t : uint8_t {
    e_create_table,
};

struct create_statement_t : statement_t {
    create_statement_t(create_type_t type)
        : statement_t(statement_type_t::e_create), type(type){};

    virtual ~create_statement_t() {}

    create_type_t type;

    string table_name;

    field_def_list_t fields;
};

/*@}*/
} // namespace ddl

/**
 * Thrown when creating a table that already exists.
 */
class table_already_exists : public gaia_exception {
  public:
    table_already_exists(const string &name) {
        stringstream message;
        message << "The table \"" << name << "\" already exists.";
        m_message = message.str();
    }
};

/**
 * Thrown when a referenced table does not exists.
 */
class table_not_exists : public gaia_exception {
  public:
    table_not_exists(const string &name) {
        stringstream message;
        message << "The table \"" << name << "\" does not exist.";
        m_message = message.str();
    }
};

/**
 * Thrown when a field is specified more than once
 */
class duplicate_field : public gaia_exception {
  public:
    duplicate_field(const string &name) {
        stringstream message;
        message << "The field \"" << name << "\" is specified more than once.";
        m_message = message.str();
    }
};

/**
 * Create a table definition in the catalog.
 *
 * @param name table name
 * @param fields fields of the table
 * @return id of the new table
 * @throw table_already_exists
 */
gaia_id_t create_table(const string &name, const ddl::field_def_list_t &fields);

/**
 * List all tables defined in the catalog.
 *
 * @return a set of tables ids in the catalog.
 */
const set<gaia_id_t> &list_tables();

/**
 * List all data payload fields for a given table defined in the catalog.
 * The fields returned here do not include references type fields.
 * Reference type fields defines foreign key relationship between tables.
 * They are not part of the data payload and are ordered separately.
 *
 * Use list_references() API to get a list of all references for a given table.
 *
 * @param table_id id of the table
 * @return a list of field ids in the order of their positions.
 */
const vector<gaia_id_t> &list_fields(gaia_id_t table_id);

/**
 * List all references for a given table defined in the catalog.
 * References are foreign key constraints or table links.
 * They defines relationships between tables.
 *
 * @param table_id id of the table
 * @return a list of ids of the table references in the order of their positions.
 */
const vector<gaia_id_t> &list_references(gaia_id_t table_id);

/**
 * Generate FlatBuffers schema (fbs) for a catalog table.
 * The given table is the root type of the generated schema.
 *
 * @return generated fbs string
 */
string generate_fbs(gaia_id_t table_id);

/**
 * Generate FlatBuffers schema (fbs) for all catalog tables.
 * No root type is specified in the generated schema.
 *
 * @return generated fbs string
 */
string generate_fbs();

/**
 * Generate the Extended Data Classes header file.
 *
 * @return generated source
 */
string gaia_generate(string);

/**
 * Retrieve the binary FlatBuffers schema (bfbs) for a given table.
 *
 * @param table_id id of the table
 * @return bfbs
 */
string get_bfbs(gaia_id_t table_id);

/*@}*/
} // namespace catalog
/*@}*/
} // namespace gaia
