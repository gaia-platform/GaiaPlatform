/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////

#include <filesystem>
#include <gaia_generate.hpp>
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
    explicit relationship_strings_t(gaia_relationship_t relationship)
        : m_relationship(std::move(relationship))
    {
    }

    std::string parent_table()
    {
        return m_relationship.parent().name();
    }

    std::string child_table()
    {
        return m_relationship.child().name();
    }

    bool is_one_to_many()
    {
        return static_cast<relationship_cardinality_t>(m_relationship.cardinality())
            == relationship_cardinality_t::many;
    }

    bool is_one_to_one()
    {
        return static_cast<relationship_cardinality_t>(m_relationship.cardinality())
            == relationship_cardinality_t::one;
    }

protected:
    gaia_relationship_t m_relationship;
};

class child_relationship : public relationship_strings_t
{
public:
    explicit child_relationship(const gaia_relationship_t& relationship)
        : relationship_strings_t(relationship)
    {
    }

    std::string field_name()
    {
        return m_relationship.to_parent_link_name();
    }

    std::string parent_offset()
    {
        return "c_" + child_table() + "_parent_" + field_name();
    }

    std::string parent_offset_value()
    {
        return to_string(m_relationship.parent_offset());
    }

    std::string next_offset()
    {

        return "c_" + child_table() + "_next_" + field_name();
    }

    std::string next_offset_value()
    {
        return to_string(m_relationship.next_child_offset());
    }
};

class parent_relationship : public relationship_strings_t
{
public:
    explicit parent_relationship(const gaia_relationship_t& relationship)
        : relationship_strings_t(relationship)
    {
    }

    std::string field_name()
    {
        return m_relationship.to_child_link_name();
    }

    std::string first_offset()
    {
        return "c_" + parent_table() + "_first_" + field_name();
    }

    std::string first_offset_value()
    {
        return to_string(m_relationship.first_child_offset());
    }

    std::string next_offset()
    {

        return "c_" + child_table() + "_next_" + m_relationship.to_parent_link_name();
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
        ASSERT_UNREACHABLE("Unknown type!");
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
        ASSERT_UNREACHABLE("Fixed size array is not supported");
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

    std::sort(relationships.begin(), relationships.end(), [](auto r1, auto r2) -> bool { return r1.first_child_offset() < r2.first_child_offset(); });

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

    std::sort(relationships.begin(), relationships.end(), [](auto r1, auto r2) -> bool { return r1.parent_offset() < r2.parent_offset(); });

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
            parent_relationships.emplace_back(relationship);
        }

        for (auto& relationship : list_child_relationships(table_record))
        {
            child_relationships.emplace_back(relationship);
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

// Generates code for static initialization of EDC expr variables for C++11 compliance.
// pair.first = declaration
// pair.second = initialization
static pair<string, string> generate_expr_variable(const string& table, const string& type, const string& field)
{
    string expr_decl;
    string expr_init;
    string type_decl;

    // Example:  gaia::direct_access::expression_t<employee_t, int64_t>
    type_decl.append("gaia::direct_access::expression_t<");
    type_decl.append(table);
    type_decl.append("_t, ");
    type_decl.append(type);
    type_decl.append(">");

    // Example:  static gaia::direct_access::expression_t<employee_t, int64_t> hire_date;
    expr_decl.append("static ");
    expr_decl.append(type_decl);
    expr_decl.append(" ");
    expr_decl.append(field);
    expr_decl.append(";");

    // Example:  template<class unused_t>
    // gaia::direct_access::expression_t<employee_t, int64_t> employee_t::expr_<unused_t>::hire_date{&employee_t::hire_date};
    expr_init.append("template<class unused_t> ");
    expr_init.append(type_decl);
    expr_init.append(" ");
    expr_init.append(table);
    expr_init.append("_t::expr_<unused_t>::");
    expr_init.append(field);
    expr_init.append("{&");
    expr_init.append(table);
    expr_init.append("_t::");
    expr_init.append(field);
    expr_init.append("};");

    return make_pair(expr_decl, expr_init);
}

static string generate_edc_struct(
    gaia_type_t table_type_id,
    gaia_table_t table_record,
    field_vector_t& field_records,
    std::vector<parent_relationship> parent_relationships,
    std::vector<child_relationship> child_relationships)
{
    flatbuffers::CodeWriter code(c_indent_string);
    vector<string> expr_init_list;

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

        //        if (relationship.is_one_to_many())
        {
            code += "typedef gaia::direct_access::reference_chain_container_t<{{CHILD_TABLE}}_t> "
                    "{{FIELD_NAME}}_list_t;";
        }
    }

    // Default public constructor.
    code += "{{TABLE_NAME}}_t() : edc_object_t(\"{{TABLE_NAME}}_t\") {}";

    // Below, a flatbuffer method is invoked as Create{{TABLE_NAME}}() or as
    // Create{{TABLE_NAME}}Direct. The choice is determined by whether any of
    // the fields are strings or vectors. If at least one is a string or a
    // vector, than the Direct variation is used.
    bool has_string_or_vector = false;
    // Accessors.
    for (const auto& f : field_records)
    {
        code.SetValue("TYPE", field_cpp_type_string(f));
        code.SetValue("FIELD_NAME", f.name());
        if (f.type() == static_cast<uint8_t>(data_type_t::e_string))
        {
            has_string_or_vector = true;
            code.SetValue("FCN_NAME", "GET_STR");
        }
        else if (f.repeated_count() != 1)
        {
            has_string_or_vector = true;
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
    code.SetValue("DIRECT", has_string_or_vector ? "Direct" : "");
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
        code.SetValue("CHILD_TABLE", relationship.child_table());
        code.SetValue("FIELD_NAME", relationship.field_name());
        code.SetValue("FIRST_OFFSET", relationship.first_offset());
        code.SetValue("NEXT_OFFSET", relationship.next_offset());

        if (relationship.is_one_to_many())
        {
            code += "{{FIELD_NAME}}_list_t {{FIELD_NAME}}() const {";
            code.IncrementIdentLevel();
            code += "return {{FIELD_NAME}}_list_t(gaia_id(), {{FIRST_OFFSET}}, {{NEXT_OFFSET}});";
            code.DecrementIdentLevel();
            code += "}";
        }
        else if (relationship.is_one_to_one())
        {
            code += "direct_access::edc_reference_t<{{CHILD_TABLE}}_t> {{FIELD_NAME}}() const {";
            code.IncrementIdentLevel();
            code += "return direct_access::edc_reference_t<{{CHILD_TABLE}}_t>{this->references()[{{FIRST_OFFSET}}]};";
            code.DecrementIdentLevel();
            code += "}";

            code += "void set_{{FIELD_NAME}}(common::gaia_id_t child_id) {";
            code.IncrementIdentLevel();
            code += "insert_child_reference(gaia_id(), child_id, {{FIRST_OFFSET}});";
            code.DecrementIdentLevel();
            code += "}";
        }
        else
        {
            ASSERT_UNREACHABLE("Unsupported relationship cardinality!");
        }
    }

    // Stores field names to make it simpler to create a namespace after the class
    // to allow unqualified access to the expressions.
    vector<string> field_names;

    // Add EDC expressions and use a dummy template to emulate C++17 inline variable
    // declarations with C++11 legal syntax.
    code += "template<class unused_t>";
    code += "struct expr_ {";
    code.IncrementIdentLevel();

    pair<string, string> expr_variable;

    expr_variable = generate_expr_variable(code.GetValue("TABLE_NAME"), "gaia::common::gaia_id_t", "gaia_id");
    code += expr_variable.first;
    expr_init_list.emplace_back(expr_variable.second);

    for (const auto& f : field_records)
    {
        code.SetValue("TYPE", field_cpp_type_string(f));
        code.SetValue("FIELD_NAME", f.name());
        expr_variable = generate_expr_variable(code.GetValue("TABLE_NAME"), code.GetValue("TYPE"), code.GetValue("FIELD_NAME"));
        code += expr_variable.first;
        expr_init_list.emplace_back(expr_variable.second);
        field_names.push_back(code.GetValue("FIELD_NAME"));
    }

    for (auto& relationship : child_relationships)
    {
        string type;
        type.append(relationship.parent_table());
        type.append("_t");

        expr_variable = generate_expr_variable(table_record.name(), type, relationship.field_name());
        code += expr_variable.first;
        expr_init_list.emplace_back(expr_variable.second);

        field_names.push_back(relationship.field_name());
    }

    for (auto& relationship : parent_relationships)
    {
        string type;

        if (relationship.is_one_to_many())
        {
            type.append(code.GetValue("TABLE_NAME"));
            type.append("_t::");
            type.append(relationship.field_name());
            type.append("_list_t");
        }
        else
        {
            code.SetValue("CHILD_TABLE", relationship.child_table());
            type.append(code.GetValue("CHILD_TABLE"));
            type.append("_t");
        }

        if (!relationship.is_one_to_one())
        {
            expr_variable = generate_expr_variable(code.GetValue("TABLE_NAME"), type, relationship.field_name());
            code += expr_variable.first;
            expr_init_list.emplace_back(expr_variable.second);
            field_names.push_back(relationship.field_name());
        }
    }

    code.DecrementIdentLevel();
    code += "};";
    code += "using expr = expr_<void>;\n";

    // The private area.
    code.DecrementIdentLevel();

    // The constructor.
    code += "explicit {{TABLE_NAME}}_t(gaia::common::gaia_id_t id) : edc_object_t(id, \"{{TABLE_NAME}}_t\") {}";

    code += "private:";
    code.IncrementIdentLevel();
    code += "friend class edc_object_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t, internal::{{TABLE_NAME}}, "
            "internal::{{TABLE_NAME}}T>;";

    // Finishing brace.
    code.DecrementIdentLevel();
    code += "};";
    code += "";

    // Initialization of static EDC expressions. For C++11 compliance we are not using
    // inline variables which are available in C++17.
    for (const string& expr_init : expr_init_list)
    {
        code += expr_init;
    }
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
            parent_relationships_names.emplace_back(relationship);
        }

        for (auto& relationship : list_child_relationships(table_record))
        {
            child_relationships_names.emplace_back(relationship);
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
