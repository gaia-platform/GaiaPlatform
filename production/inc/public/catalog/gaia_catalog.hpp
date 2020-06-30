/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_object.hpp"
#include "gaia_exception.hpp"
#include <string>

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

enum class create_type_t : unsigned int {
    CREATE_TABLE,
};

struct create_statement_t : statement_t {
    create_statement_t(create_type_t type)
        : statement_t(statment_type_t::CREATE), type(type), fields(nullptr){};

    create_type_t type;

    string tableName;

    vector<field_definition_t *> *fields;
};

/*@}*/
} // namespace ddl

/**
 * Thrown when creating a table that already exists.
 */
class table_already_exists: public gaia_exception {
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
gaia_id_t create_table(const string &name, const vector<ddl::field_definition_t *> &fields);

/**
 * List all tables defined in the catalog.
 *
 * @return a list of tables ids in the catalog.
 * The method makes no guarantees on the order of the table ids returned.
 */
const vector<gaia_id_t> &list_tables();

/**
 * List all fields for a give table defined in the catalog.
 *
 * @param table_id id of the table
 * @return a list of field ids in the order of their positions.
 */
const vector<gaia_id_t> &list_fields(gaia_id_t table_id);

/**
 * Generate FlatBuffers schema (fbs) from catalog table definitions
 *
 * @return generated fbs string
 */
string generate_fbs();

/*@}*/
} // namespace catalog
/*@}*/
} // namespace gaia
