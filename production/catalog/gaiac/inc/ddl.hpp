#ifndef __DDL_H_
#define __DDL_H_
#include <cstdint>
#include <string>
#include <vector>

namespace gaia {
namespace catalog {
namespace ddl {

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
    STRING
};

enum class StatementType : unsigned int { kStmtCreate, kStmtDrop, kStmtAlter };

struct Statement {

    Statement(StatementType type) : _type(type){};

    StatementType type() const { return _type; };

    bool isType(StatementType type) const { return _type == type; };

  private:
    StatementType _type;
};

struct CompositeName {
    CompositeName(std::string schema, std::string name)
        : schema(std::move(schema)), name(std::move(name)){};

    CompositeName(std::string name) : name(std::move(name)){};

    std::string schema;
    std::string name;
};

struct FieldDefinition {
    FieldDefinition(std::string name, DataType type, uint16_t length)
        : name(std::move(name)), type(type), length(length){};

    std::string name;
    DataType type;
    uint16_t length;
};

enum class CreateType : unsigned int {
    kCreateSchema,
    kCreateTable,
    kCreateTableAs,
    kCreateType
};

struct CreateStatement : Statement {
    CreateStatement(CreateType type)
        : Statement(StatementType::kStmtCreate), type(type), fields(nullptr){};

    CreateType type;
    std::string tableSchema;
    std::string tableName;
    std::string typeSchema;
    std::string typeName;
    std::vector<FieldDefinition *> *fields;
};

} // namespace ddl
} // namespace catalog
} // namespace gaia

#endif // __DDL_H_
