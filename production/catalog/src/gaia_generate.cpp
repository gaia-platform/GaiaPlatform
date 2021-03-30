/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////

#include <memory>
#include <set>
#include <vector>

#include "flatbuffers/code_generators.h"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/retail_assert.hpp"

#include "type_id_mapping.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;

namespace gaia
{
namespace catalog
{

const string c_indent_string("    ");

class relationship_strings_t
{
public:
    relationship_strings_t(const gaia_relationship_t& relationship)
        : relationship(relationship){};

    std::string parent_table()
    {
        return relationship.parent().name();
    }

    std::string child_table()
    {
        return relationship.child().name();
    }

protected:
    gaia_relationship_t relationship;
};

class child_relationship : public relationship_strings_t
{
public:
    child_relationship(const gaia_relationship_t& relationship)
        : relationship_strings_t(relationship){};

    std::string field_name()
    {
        return relationship.to_parent_link_name();
    }

    std::string parent_offset()
    {
        return "c_" + child_table() + "_parent_" + field_name();
    }

    std::string parent_offset_value()
    {
        return to_string(relationship.parent_offset());
    }

    std::string next_offset()
    {

        return "c_" + child_table() + "_next_" + field_name();
    }

    std::string next_offset_value()
    {

        return to_string(relationship.next_child_offset());
    }
};

class parent_relationship : public relationship_strings_t
{
public:
    parent_relationship(const gaia_relationship_t& relationship)
        : relationship_strings_t(relationship){};

    std::string field_name()
    {
        return relationship.to_child_link_name();
    }

    std::string first_offset()
    {
        return "c_" + parent_table() + "_first_" + field_name();
    }

    std::string first_offset_value()
    {
        return to_string(relationship.first_child_offset());
    }

    std::string next_offset()
    {

        return "c_" + child_table() + "_next_" + relationship.to_parent_link_name();
    }
};

typedef std::vector<gaia_field_t> field_vector_t;
typedef std::vector<gaia_relationship_t> relationship_vector_t;

static string field_cpp_type_string(
    const gaia_field_t& field, bool is_function_parameter = false)
{
    string type_str;

    switch (static_cast<data_type_t>(field.type()))
    {
    case data_type_t::e_bool:
        type_str = "bool";
        break;
    case data_type_t::e_int8:
        type_str = "int8_t";
        break;
    case data_type_t::e_uint8:
        type_str = "uint8_t";
        break;
    case data_type_t::e_int16:
        type_str = "int16_t";
        break;
    case data_type_t::e_uint16:
        type_str = "uint16_t";
        break;
    case data_type_t::e_int32:
        type_str = "int32_t";
        break;
    case data_type_t::e_uint32:
        type_str = "uint32_t";
        break;
    case data_type_t::e_int64:
        type_str = "int64_t";
        break;
    case data_type_t::e_uint64:
        type_str = "uint64_t";
        break;
    case data_type_t::e_float:
        type_str = "float";
        break;
    case data_type_t::e_double:
        type_str = "double";
        break;
    case data_type_t::e_string:
        type_str = "const char*";
        break;
    default:
        retail_assert(false, "Unknown type!");
    };

    if (field.repeated_count() == 0)
    {
        if (is_function_parameter)
        {
            type_str = "const std::vector<" + type_str + ">&";
        }
        else
        {
            type_str = "gaia::direct_access::edc_vector_t<" + type_str + ">";
        }
    }
    else if (field.repeated_count() > 1)
    {
        // We should report the input error to the user at data definition time.
        // If we find a fixed size array definition here, it will be either data
        // corruption or bugs in catching user input errors.
        retail_assert(false, "Fixed size array is not supported");
    }
    return type_str;
}

// List the relationships where table appear as parent, sorted by offset.
static relationship_vector_t list_parent_relationships(gaia_table_t table)
{
    relationship_vector_t relationships;

    for (const auto& relationship : table.gaia_relationships_parent())
    {
        relationships.push_back(relationship);
    }

    std::sort(relationships.begin(), relationships.end(), [](auto r1, auto r2) -> bool {
        return r1.first_child_offset() < r2.first_child_offset();
    });

    return relationships;
}

// List the relationships where table appear as child, sorted by offset.
static relationship_vector_t list_child_relationships(gaia_table_t table)
{
    relationship_vector_t relationships;

    for (const auto& relationship : table.gaia_relationships_child())
    {
        relationships.push_back(relationship);
    }

    std::sort(relationships.begin(), relationships.end(), [](auto r1, auto r2) -> bool {
        return r1.parent_offset() < r2.parent_offset();
    });

    return relationships;
}

static string generate_boilerplate_top(string dbname)
{
    flatbuffers::CodeWriter code(c_indent_string);
    code.SetValue("DBNAME", dbname);
    code += "/////////////////////////////////////////////";
    code += "// Copyright (c) Gaia Platform LLC";
    code += "// All rights reserved.";
    code += "/////////////////////////////////////////////";
    code += "";
    code += "// Automatically generated by the Gaia Data Classes code generator.";
    code += "// Do not modify.";
    code += "";
    code += "#include <iterator>";
    code += "";
    code += "#ifndef GAIA_GENERATED_{{DBNAME}}_H_";
    code += "#define GAIA_GENERATED_{{DBNAME}}_H_";
    code += "";
    code += "#include \"gaia/direct_access/edc_object.hpp\"";
    code += "#include \"{{DBNAME}}_generated.h\"";
    code += "#include \"gaia/direct_access/edc_iterators.hpp\"";
    code += "";
    code += "namespace " + c_gaia_namespace + " {";
    if (!dbname.empty())
    {
        code += "namespace {{DBNAME}} {";
    }
    string str = code.ToString();
    return str;
}

static string generate_boilerplate_bottom(string dbname)
{
    flatbuffers::CodeWriter code(c_indent_string);
    code.SetValue("DBNAME", dbname);
    if (!dbname.empty())
    {
        code += "}  // namespace {{DBNAME}}";
    }
    code += "}  // namespace " + c_gaia_namespace;
    code += "";
    code += "#endif  // GAIA_GENERATED_{{DBNAME}}_H_";
    string str = code.ToString();
    return str;
}

// Generate the list of constants referred to by the class definitions and templates.
static string generate_constant_list(const gaia_id_t db_id)
{
    flatbuffers::CodeWriter code(c_indent_string);
    // A fixed constant is used for the flatbuffer builder constructor.
    code += "";
    code += "// The initial size of the flatbuffer builder buffer.";
    code += "constexpr int c_flatbuffer_builder_size = 128;";
    code += "";

    for (auto& table_record : gaia_database_t::get(db_id).gaia_tables())
    {
        std::vector<parent_relationship> parent_relationships;
        std::vector<child_relationship> child_relationships;

        for (auto& relationship : list_parent_relationships(table_record))
        {
            parent_relationships.push_back(
                parent_relationship{relationship});
        }

        for (auto& relationship : list_child_relationships(table_record))
        {
            child_relationships.push_back(
                child_relationship{relationship});
        }

        code.SetValue("TABLE_NAME", table_record.name());
        code.SetValue("TABLE_TYPE", to_string(table_record.type()));
        code += "// Constants contained in the {{TABLE_NAME}} object.";
        code += "constexpr uint32_t c_gaia_type_{{TABLE_NAME}} = {{TABLE_TYPE}}u;";

        for (auto& relationship : child_relationships)
        {
            code.SetValue("PARENT_OFFSET", relationship.parent_offset());
            code.SetValue("PARENT_OFFSET_VALUE", relationship.parent_offset_value());
            code += "constexpr int {{PARENT_OFFSET}} = {{PARENT_OFFSET_VALUE}};";

            code.SetValue("NEXT_OFFSET", relationship.next_offset());
            code.SetValue("NEXT_OFFSET_VALUE", relationship.next_offset_value());
            code += "constexpr int {{NEXT_OFFSET}} = {{NEXT_OFFSET_VALUE}};";
        }

        for (auto& relationship : parent_relationships)
        {
            code.SetValue("FIRST_OFFSET", relationship.first_offset());
            code.SetValue("FIRST_OFFSET_VALUE", relationship.first_offset_value());
            code += "constexpr int {{FIRST_OFFSET}} = {{FIRST_OFFSET_VALUE}};";
        }

        code += "";
    }
    string str = code.ToString();
    return str;
}

static string generate_declarations(const gaia_id_t db_id)
{
    flatbuffers::CodeWriter code(c_indent_string);

    for (const auto& table : gaia_database_t::get(db_id).gaia_tables())
    {
        code.SetValue("TABLE_NAME", table.name());
        code += "struct {{TABLE_NAME}}_t;";
    }
    code += "";
    string str = code.ToString();
    return str;
}

static string generate_edc_struct(
    gaia_type_t table_type_id,
    gaia_table_t table_record,
    field_vector_t& field_records,
    std::vector<parent_relationship> parent_relationships,
    std::vector<child_relationship> child_relationships)
{
    flatbuffers::CodeWriter code(c_indent_string);

    // Struct statement.
    code.SetValue("TABLE_NAME", table_record.name());
    code.SetValue("POSITION", to_string(table_type_id));

    code += "typedef gaia::direct_access::edc_writer_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t, internal::{{TABLE_NAME}}, internal::{{TABLE_NAME}}T> "
            "{{TABLE_NAME}}_writer;";
    code += "struct {{TABLE_NAME}}_t : public gaia::direct_access::edc_object_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t, "
            "internal::{{TABLE_NAME}}, internal::{{TABLE_NAME}}T> {";

    code.IncrementIdentLevel();

    // Iterate over the relationships where the current table appear as parent
    for (auto& relationship : parent_relationships)
    {
        code.SetValue("FIELD_NAME", relationship.field_name());
        code.SetValue("CHILD_TABLE", relationship.child_table());

        code += "typedef gaia::direct_access::reference_chain_container_t<{{CHILD_TABLE}}_t> "
                "{{FIELD_NAME}}_list_t;";
    }

    // Default public constructor.
    code += "{{TABLE_NAME}}_t() : edc_object_t(\"{{TABLE_NAME}}_t\") {}";

    // Below, a flatbuffer method is invoked as Create{{TABLE_NAME}}() or
    // as Create{{TABLE_NAME}}Direct. The choice is determined by whether any of the
    // fields are strings. If at least one is a string, than the Direct variation
    // is used.
    // NOTE: There may be a third variation of this if any of the fields are vectors
    // or possibly arrays.
    bool has_string = false;
    // Accessors.
    for (const auto& f : field_records)
    {
        code.SetValue("TYPE", field_cpp_type_string(f));
        code.SetValue("FIELD_NAME", f.name());
        if (f.type() == static_cast<uint8_t>(data_type_t::e_string))
        {
            has_string = true;
            code.SetValue("FCN_NAME", "GET_STR");
        }
        else if (f.repeated_count() != 1)
        {
            code.SetValue("FCN_NAME", "GET_ARRAY");
        }
        else
        {
            code.SetValue("FCN_NAME", "GET");
        }
        code += "{{TYPE}} {{FIELD_NAME}}() const {return {{FCN_NAME}}({{FIELD_NAME}});}";
    }

    // The typed insert_row().
    string param_list("static gaia::common::gaia_id_t insert_row(");
    bool first = true;
    for (const auto& f : field_records)
    {
        if (!first)
        {
            param_list += ", ";
        }
        else
        {
            first = false;
        }
        bool is_function_parameter = true;
        param_list += field_cpp_type_string(f, is_function_parameter) + " ";
        param_list += f.name();
    }
    code += param_list + ") {";
    code.IncrementIdentLevel();
    code += "flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);";
    code.SetValue("DIRECT", has_string ? "Direct" : "");
    param_list = "b.Finish(internal::Create{{TABLE_NAME}}{{DIRECT}}(b";
    for (const auto& f : field_records)
    {
        param_list += ", ";
        if (f.repeated_count() != 1)
        {
            param_list += "&";
        }
        param_list += f.name();
    }
    param_list += "));";
    code += param_list;
    code += "return edc_object_t::insert_row(b);";
    code.DecrementIdentLevel();
    code += "}";

    // The table range.
    code += "static gaia::direct_access::edc_container_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t>& list() {";
    code.IncrementIdentLevel();
    code += "static gaia::direct_access::edc_container_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t> list;";
    code += "return list;";
    code.DecrementIdentLevel();
    code += "}";

    // Iterate over the relationships where the current table is the child
    for (auto& relationship : child_relationships)
    {
        code.SetValue("FIELD_NAME", relationship.field_name());
        code.SetValue("PARENT_TABLE", relationship.parent_table());
        code.SetValue("PARENT_OFFSET", relationship.parent_offset());

        code += "{{PARENT_TABLE}}_t {{FIELD_NAME}}() const {";
        code.IncrementIdentLevel();
        code += "return {{PARENT_TABLE}}_t::get(this->references()[{{PARENT_OFFSET}}]);";
        code.DecrementIdentLevel();
        code += "}";
    }

    // Iterate over the relationships where the current table appear as parent
    for (auto& relationship : parent_relationships)
    {
        code.SetValue("FIELD_NAME", relationship.field_name());
        code.SetValue("FIRST_OFFSET", relationship.first_offset());
        code.SetValue("NEXT_OFFSET", relationship.next_offset());

        code += "{{FIELD_NAME}}_list_t {{FIELD_NAME}}() const {";
        code.IncrementIdentLevel();
        code += "return {{FIELD_NAME}}_list_t(gaia_id(), {{FIRST_OFFSET}}, {{NEXT_OFFSET}});";
        code.DecrementIdentLevel();
        code += "}";
    }

    // Stores field names to make it simpler to create a namespace after the class
    // to allow unqualified access to the expressions.
    vector<string> field_names;

    // Add EDC expressions
    code += "struct expr {";
    code.IncrementIdentLevel();

    code += "static inline gaia::direct_access::expression_t<{{TABLE_NAME}}_t, gaia::common::gaia_id_t> gaia_id{&{{TABLE_NAME}}_t::gaia_id};";

    for (const auto& f : field_records)
    {
        code.SetValue("TYPE", field_cpp_type_string(f));
        code.SetValue("FIELD_NAME", f.name());
        code += "static inline gaia::direct_access::expression_t<{{TABLE_NAME}}_t, {{TYPE}}> {{FIELD_NAME}}{&{{TABLE_NAME}}_t::{{FIELD_NAME}}};";
        field_names.push_back(code.GetValue("FIELD_NAME"));
    }

    for (auto& relationship : child_relationships)
    {
        code.SetValue("FIELD_NAME", relationship.field_name());
        code.SetValue("PARENT_TABLE", relationship.parent_table());

        code += "static inline gaia::direct_access::expression_t<{{TABLE_NAME}}_t, {{PARENT_TABLE}}_t> {{FIELD_NAME}}{&{{TABLE_NAME}}_t::{{FIELD_NAME}}};";
        field_names.push_back(code.GetValue("FIELD_NAME"));
    }

    for (auto& relationship : parent_relationships)
    {
        code.SetValue("FIELD_NAME", relationship.field_name());
        code.SetValue("FIRST_OFFSET", relationship.first_offset());
        code.SetValue("NEXT_OFFSET", relationship.next_offset());

        code += "static inline gaia::direct_access::expression_t<{{TABLE_NAME}}_t, {{FIELD_NAME}}_list_t> {{FIELD_NAME}}{&{{TABLE_NAME}}_t::{{FIELD_NAME}}};";
        field_names.push_back(code.GetValue("FIELD_NAME"));
    }

    code.DecrementIdentLevel();
    code += "};\n";

    // The private area.
    code.DecrementIdentLevel();
    code += "private:";
    code.IncrementIdentLevel();
    code += "friend struct edc_object_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t, internal::{{TABLE_NAME}}, "
            "internal::{{TABLE_NAME}}T>;";

    // The constructor.
    code += "explicit {{TABLE_NAME}}_t(gaia::common::gaia_id_t id) : edc_object_t(id, \"{{TABLE_NAME}}_t\") {}";

    // Finishing brace.
    code.DecrementIdentLevel();
    code += "};";
    code += "";

    code += "namespace {{TABLE_NAME}}_expr {";
    code.IncrementIdentLevel();
    for (const string& field_name : field_names)
    {
        code.SetValue("FIELD_NAME", field_name);
        code += "static auto& {{FIELD_NAME}} = {{TABLE_NAME}}_t::expr::{{FIELD_NAME}};";
    }
    code.DecrementIdentLevel();
    code += "};";
    code += "";

    string str = code.ToString();
    return str;
}

string gaia_generate(const string& dbname)
{
    gaia_id_t db_id = find_db_id(dbname);
    if (db_id == c_invalid_gaia_id)
    {
        throw db_not_exists(dbname);
    }

    string code_lines;
    begin_transaction();

    std::map<uint8_t, gaia_relationship_t> relationships;

    code_lines = generate_boilerplate_top(dbname);

    code_lines += generate_constant_list(db_id);

    code_lines += generate_declarations(db_id);

    // This is to workaround the issue of incomplete forward declaration of structs that refer to each other.
    // By collecting the IDs in the sorted set, the structs are generated in the ascending order of their IDs.
    set<gaia_id_t> table_ids;
    for (const auto& table : gaia_database_t::get(db_id).gaia_tables())
    {
        table_ids.insert(table.gaia_id());
    }

    for (auto table_id : table_ids)
    {
        field_vector_t field_records;
        auto table_record = gaia_table_t::get(table_id);
        for (auto field_id : list_fields(table_id))
        {
            field_records.push_back(gaia_field_t::get(field_id));
        }

        std::vector<parent_relationship> parent_relationships_names;
        std::vector<child_relationship> child_relationships_names;

        for (auto& relationship : list_parent_relationships(table_record))
        {
            parent_relationships_names.push_back(
                parent_relationship{relationship});
        }

        for (auto& relationship : list_child_relationships(table_record))
        {
            child_relationships_names.push_back(
                child_relationship{relationship});
        }

        code_lines += generate_edc_struct(
            table_id,
            table_record,
            field_records,
            parent_relationships_names,
            child_relationships_names);
    }
    commit_transaction();

    code_lines += generate_boilerplate_bottom(dbname);

    return code_lines;
}
} // namespace catalog
} // namespace gaia
