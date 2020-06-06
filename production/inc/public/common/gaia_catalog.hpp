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

enum class DataType : unsigned int {
    BOOLEAN,
    BYTE,
    UBYTE,
    SHORT,
    USHORT,
    INT,
    UINT,
    LONG,
    ULONG,
    FLOAT,
    DOUBLE,
    STRING,
    TABLE
};

enum class StatementType : unsigned int { kStmtCreate, kStmtDrop, kStmtAlter };

struct Statement {

    Statement(StatementType type) : _type(type){};

    StatementType type() const { return _type; };

    bool isType(StatementType type) const { return _type == type; };

  private:
    StatementType _type;
};

struct FieldType {
    FieldType(DataType type) : type(type){};

    DataType type;
    std::string name;
};

struct FieldDefinition {
    FieldDefinition(std::string name, DataType type, uint16_t length)
        : name(std::move(name)), type(type), length(length){};

    std::string name;
    DataType type;
    uint16_t length;

    std::string tableTypeName;
};

enum class CreateType : unsigned int {
    kCreateTable,
    kCreateTableOf,
    kCreateType
};

struct CreateStatement : Statement {
    CreateStatement(CreateType type)
        : Statement(StatementType::kStmtCreate), type(type), fields(nullptr){};

    CreateType type;

    std::string tableName;

    std::string typeName;

    std::vector<FieldDefinition *> *fields;
};

/*@}*/
} // namespace ddl

void initialize_catalog(bool is_engine);

void create_type(std::string name, std::vector<ddl::FieldDefinition *> *fields);

void create_table_of(std::string tableName, std::string typeName);

void create_table(std::string name,
                  std::vector<ddl::FieldDefinition *> *fields);

/*@}*/
} // namespace catalog
/*@}*/
} // namespace gaia
