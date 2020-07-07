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
namespace ddl {
/**
 * \addtogroup ddl
 * @{
 *
 * Definitions for parse result bindings
 */

enum class data_type_t : unsigned int {
    BOOL,
    INT8,
    UINT8,
    INT16,
    UINT16,
    INT32,
    UINT32,
    INT64,
    UINT64,
    FLOAT32,
    FLOAT64,
    STRING,
    TABLE
};

enum class statment_type_t : unsigned int {
    CREATE,
    DROP,
    ALTER
};

struct statement_t {

    statement_t(statment_type_t type) : m_type(type){};

    statment_type_t type() const { return m_type; };

    bool is_type(statment_type_t type) const { return m_type == type; };

    virtual ~statement_t(){};

  private:
    statment_type_t m_type;
};

struct field_type_t {
    field_type_t(data_type_t type) : type(type){};

    data_type_t type;
    string name;
};

struct field_definition_t {
    field_definition_t(string name, data_type_t type, uint16_t length)
        : name(move(name)), type(type), length(length){};

    string name;
    data_type_t type;
    uint16_t length;

    string table_type_name;
};

using field_def_list_t = vector<unique_ptr<field_definition_t>>;

enum class create_type_t : unsigned int {
    CREATE_TABLE,
};

struct create_statement_t : statement_t {
    create_statement_t(create_type_t type)
        : statement_t(statment_type_t::CREATE), type(type) {};

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
        message << "The table " << name << " already exists.";
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
