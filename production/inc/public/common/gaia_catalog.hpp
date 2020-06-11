/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_object.hpp"
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
    std::string name;
};

struct field_definition_t {
    field_definition_t(std::string name, data_type_t type, uint16_t length)
        : name(std::move(name)), type(type), length(length){};

    std::string name;
    data_type_t type;
    uint16_t length;

    std::string table_type_name;
};

enum class create_type_t : unsigned int {
    CREATE_TABLE,
};

struct create_statement_t : statement_t {
    create_statement_t(create_type_t type)
        : statement_t(statment_type_t::CREATE), type(type), fields(nullptr){};

    create_type_t type;

    std::string tableName;

    std::vector<field_definition_t *> *fields;
};

/*@}*/
} // namespace ddl

gaia_id_t create_table(std::string name, const std::vector<ddl::field_definition_t *> &fields);

std::string generate_fbs();

/*@}*/
} // namespace catalog
/*@}*/
} // namespace gaia
