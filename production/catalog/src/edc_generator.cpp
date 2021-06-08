/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "edc_generator.hpp"

#include <flatbuffers/code_generators.h>

#include <gaia_internal/catalog/catalog.hpp>

namespace gaia
{
namespace catalog
{
namespace generate
{

const std::string c_indentation_string("    ");

std::string edc_compilation_unit_writer_t::write_header()
{
    flatbuffers::CodeWriter code(c_indentation_string);
    code += generate_copyright();
    code += generate_open_header_guard();
    code += generate_includes();
    code += generate_open_namespace();
    code += generate_constants();
    code += generate_forward_declarations();
    code += generate_ref_forward_declarations();

    for (const table_facade_t& table : m_database.tables())
    {
        class_writer_t class_writer{table};
        code += class_writer.write_header();
    }

    code += generate_close_namespace();
    code += generate_close_header_guard();
    return code.ToString();
}

std::string edc_compilation_unit_writer_t::write_cpp()
{
    flatbuffers::CodeWriter code(c_indentation_string);
    code += generate_copyright();
    code += generate_includes_cpp();
    code += generate_open_namespace();

    for (const table_facade_t& table : m_database.tables())
    {
        class_writer_t class_writer{table};
        code += class_writer.write_cpp();
    }

    code += generate_close_namespace();
    return code.ToString();
}
flatbuffers::CodeWriter edc_compilation_unit_writer_t::create_code_writer()
{
    flatbuffers::CodeWriter code(c_indentation_string);
    code.SetValue("DBNAME", m_database.database_name());
    return code;
}

std::string edc_compilation_unit_writer_t::generate_copyright()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "/////////////////////////////////////////////";
    code += "// Copyright (c) Gaia Platform LLC";
    code += "// All rights reserved.";
    code += "/////////////////////////////////////////////";
    code += "";
    code += "// Automatically generated by the Gaia Extended Data Classes code generator.";
    code += "// Do not modify.";

    return code.ToString();
}

std::string edc_compilation_unit_writer_t::generate_open_header_guard()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "#ifndef GAIA_GENERATED_{{DBNAME}}_H_";
    code += "#define GAIA_GENERATED_{{DBNAME}}_H_";

    return code.ToString();
}

std::string edc_compilation_unit_writer_t::generate_close_header_guard()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "#endif  // GAIA_GENERATED_{{DBNAME}}_H_";
    return code.ToString();
}

std::string edc_compilation_unit_writer_t::generate_includes()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "#include <gaia/direct_access/edc_object.hpp>";
    code += "#include <gaia/direct_access/edc_iterators.hpp>";
    code += "#include \"{{DBNAME}}_generated.h\"";

    return code.ToString();
}

std::string edc_compilation_unit_writer_t::generate_includes_cpp()
{
    return "#include \"{GENERATED_EDC_HEADER}\"\n";
}

std::string edc_compilation_unit_writer_t::generate_open_namespace()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "namespace " + c_gaia_namespace + " {";
    if (!m_database.is_default_database())
    {
        code += "namespace {{DBNAME}} {";
    }

    return code.ToString();
}

std::string edc_compilation_unit_writer_t::generate_close_namespace()
{
    flatbuffers::CodeWriter code = create_code_writer();
    if (!m_database.is_default_database())
    {
        code += "}  // namespace {{DBNAME}}";
    }
    code += "}  // namespace " + c_gaia_namespace;

    return code.ToString();
}

std::string edc_compilation_unit_writer_t::generate_constants()
{
    flatbuffers::CodeWriter code = create_code_writer();
    // A fixed constant is used for the flatbuffer builder constructor.
    code += "// The initial size of the flatbuffer builder buffer.";
    code += "constexpr int c_flatbuffer_builder_size = 128;";
    code += "";

    for (const table_facade_t& table : m_database.tables())
    {
        code.SetValue("TABLE_NAME", table.table_name());
        code.SetValue("TABLE_TYPE", table.table_type());
        code += "// Constants contained in the {{TABLE_NAME}} object.";
        code += "constexpr uint32_t c_gaia_type_{{TABLE_NAME}} = {{TABLE_TYPE}}u;";

        // This map is used to place the constants ordered by offset.
        // There is no practical reason besides making the code easier to read.
        // The '//' in the end of each line prevents the new line to be created.
        std::map<int, std::string> table_constants;

        for (const incoming_relationship_facade_t& relationship : table.incoming_relationships())
        {
            flatbuffers::CodeWriter const_code = create_code_writer();
            const_code.SetValue("PARENT_OFFSET", relationship.parent_offset());
            const_code.SetValue("PARENT_OFFSET_VALUE", relationship.parent_offset_value());
            const_code += "constexpr int {{PARENT_OFFSET}} = {{PARENT_OFFSET_VALUE}};\\";
            table_constants.insert({std::stoi(relationship.parent_offset_value()), const_code.ToString()});

            const_code.Clear();
            const_code.SetValue("NEXT_OFFSET", relationship.next_offset());
            const_code.SetValue("NEXT_OFFSET_VALUE", relationship.next_offset_value());
            const_code += "constexpr int {{NEXT_OFFSET}} = {{NEXT_OFFSET_VALUE}};\\";
            table_constants.insert({std::stoi(relationship.next_offset_value()), const_code.ToString()});
        }

        for (const outgoing_relationship_facade_t& relationship : table.outgoing_relationships())
        {
            flatbuffers::CodeWriter const_code = create_code_writer();
            const_code.SetValue("FIRST_OFFSET", relationship.first_offset());
            const_code.SetValue("FIRST_OFFSET_VALUE", relationship.first_offset_value());
            const_code += "constexpr int {{FIRST_OFFSET}} = {{FIRST_OFFSET_VALUE}};\\";
            table_constants.insert({std::stoi(relationship.first_offset_value()), const_code.ToString()});
        }

        for (auto& constant_pair : table_constants)
        {
            code += constant_pair.second;
        }
        code += "";
    }

    return code.ToString();
}

std::string edc_compilation_unit_writer_t::generate_forward_declarations()
{
    flatbuffers::CodeWriter code = create_code_writer();

    for (const auto& table : m_database.tables())
    {
        code.SetValue("TABLE_NAME", table.table_name());
        code += "class {{TABLE_NAME}}_t;";
    }
    std::string str = code.ToString();
    return str;
}

std::string edc_compilation_unit_writer_t::generate_ref_forward_declarations()
{
    flatbuffers::CodeWriter code = create_code_writer();

    for (const auto& table : m_database.tables())
    {
        for (const auto& relationship : table.outgoing_relationships())
        {
            if (relationship.is_one_to_one())
            {
                code.SetValue("TABLE_NAME", relationship.child_table());
                code += "class {{TABLE_NAME}}_ref_t;";
            }
        }
    }

    std::string str = code.ToString();
    return str;
}

std::string class_writer_t::write_header()
{
    flatbuffers::CodeWriter code(c_indentation_string);
    code += generate_writer() + "\\";
    code += generate_class_definition() + "\\";
    code += "public:";
    increment_indent();
    code += generate_list_types() + "\\";
    code += generate_public_constructor() + "\\";
    code += generate_insert() + "\\";
    code += generate_list_accessor() + "\\";
    code += generate_fields_accessors() + "\\";
    code += generate_incoming_relationships_accessors() + "\\";
    code += generate_outgoing_relationships_accessors();
    code += generate_expressions() + "\\";
    decrement_indent();
    code += "private:";
    increment_indent();
    code += generate_private_constructor() + "\\";
    code += generate_friend_declarations() + "\\";
    decrement_indent();
    code += generate_close_class_definition();
    code += generate_ref_class();
    code += generate_expr_namespace();
    code += generate_expr_instantiation_cpp();
    return code.ToString();
}

std::string class_writer_t::write_cpp()
{
    flatbuffers::CodeWriter code(c_indentation_string);
    code += generate_class_section_comment_cpp();
    code += generate_insert_cpp() + "\\";
    code += generate_list_accessor_cpp() + "\\";
    code += generate_fields_accessors_cpp() + "\\";
    code += generate_incoming_relationships_accessors_cpp() + "\\";
    code += generate_outgoing_relationships_accessors_cpp() + "\\";
    code += generate_ref_class_cpp() + "\\";
    return code.ToString();
}

flatbuffers::CodeWriter class_writer_t::create_code_writer()
{
    flatbuffers::CodeWriter code(c_indentation_string);
    code.SetValue("TABLE_NAME", m_table.table_name());

    for (int i = 0; i < m_indent_level; i++)
    {
        code.IncrementIdentLevel();
    }

    return code;
}

std::string class_writer_t::generate_class_section_comment_cpp()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "//";
    code += "// Implementation of class {{TABLE_NAME}}_t.";
    code += "//";
    return code.ToString();
}

std::string class_writer_t::generate_writer()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "typedef gaia::direct_access::edc_writer_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t, internal::{{TABLE_NAME}}, internal::{{TABLE_NAME}}T> "
            "{{TABLE_NAME}}_writer;";
    return code.ToString();
}

std::string class_writer_t::generate_class_definition()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "class {{TABLE_NAME}}_t : public gaia::direct_access::edc_object_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t, "
            "internal::{{TABLE_NAME}}, internal::{{TABLE_NAME}}T> {";
    return code.ToString();
}

std::string class_writer_t::generate_list_types()
{
    flatbuffers::CodeWriter code = create_code_writer();
    for (auto& relationship : m_table.outgoing_relationships())
    {
        code.SetValue("FIELD_NAME", relationship.field_name());
        code.SetValue("CHILD_TABLE", relationship.child_table());

        if (relationship.is_one_to_many())
        {
            code += "typedef gaia::direct_access::reference_chain_container_t<{{CHILD_TABLE}}_t> "
                    "{{FIELD_NAME}}_list_t;";
        }
    }
    return code.ToString();
}

std::string class_writer_t::generate_public_constructor()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "{{TABLE_NAME}}_t() : edc_object_t(\"{{TABLE_NAME}}_t\") {}";
    return code.ToString();
}

std::string class_writer_t::generate_insert()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "static gaia::common::gaia_id_t insert_row(\\";
    code += generate_method_params(m_table.fields()) + "\\";
    code += ");";
    return code.ToString();
}

std::string class_writer_t::generate_insert_cpp()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "gaia::common::gaia_id_t {{TABLE_NAME}}_t::insert_row(\\";
    code += generate_method_params(m_table.fields()) + "\\";
    code += ") {";
    code.IncrementIdentLevel();

    code += "flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);";
    code.SetValue("DIRECT", m_table.has_string_or_vector() ? "Direct" : "");
    code += "b.Finish(internal::Create{{TABLE_NAME}}{{DIRECT}}(b\\";

    for (const auto& field : m_table.fields())
    {
        code += ", \\";
        if (field.is_vector())
        {
            code += "&\\";
        }
        code += field.field_name() + "\\";
    }
    code += "));";
    code += "return edc_object_t::insert_row(b);";
    code.DecrementIdentLevel();
    code += "}";
    return code.ToString();
}

std::string class_writer_t::generate_list_accessor()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "static gaia::direct_access::edc_container_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t> list();";
    return code.ToString();
}

std::string class_writer_t::generate_list_accessor_cpp()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "gaia::direct_access::edc_container_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t> {{TABLE_NAME}}_t::list() {";
    code.IncrementIdentLevel();
    code += "return gaia::direct_access::edc_container_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t>();";
    code.DecrementIdentLevel();
    code += "}";
    return code.ToString();
}

std::string class_writer_t::generate_fields_accessors()
{
    flatbuffers::CodeWriter code = create_code_writer();

    for (const auto& field : m_table.fields())
    {
        code.SetValue("TYPE", field.field_type());
        code.SetValue("FIELD_NAME", field.field_name());
        code += "{{TYPE}} {{FIELD_NAME}}() const;";
    }

    return code.ToString();
}

std::string class_writer_t::generate_fields_accessors_cpp()
{
    flatbuffers::CodeWriter code = create_code_writer();

    // Below, a flatbuffer method is invoked as Create{{TABLE_NAME}}() or as
    // Create{{TABLE_NAME}}Direct. The choice is determined by whether any of
    // the fields are strings or vectors. If at least one is a string or a
    // vector, than the Direct variation is used.
    for (const auto& field : m_table.fields())
    {
        code.SetValue("TYPE", field.field_type());
        code.SetValue("FIELD_NAME", field.field_name());

        if (field.is_string())
        {
            code.SetValue("FUNCTION_NAME", "GET_STR");
        }
        else if (field.is_vector())
        {
            code.SetValue("FUNCTION_NAME", "GET_ARRAY");
        }
        else
        {
            code.SetValue("FUNCTION_NAME", "GET");
        }
        code += "{{TYPE}} {{TABLE_NAME}}_t::{{FIELD_NAME}}() const {";
        code.IncrementIdentLevel();
        code += "return {{FUNCTION_NAME}}({{FIELD_NAME}});";
        code.DecrementIdentLevel();
        code += "}";
    }

    return code.ToString();
}

std::string class_writer_t::generate_incoming_relationships_accessors()
{
    flatbuffers::CodeWriter code = create_code_writer();

    // Iterate over the relationships where the current table is the child
    for (auto& relationship : m_table.incoming_relationships())
    {
        code.SetValue("FIELD_NAME", relationship.field_name());
        code.SetValue("PARENT_TABLE", relationship.parent_table());

        code += "{{PARENT_TABLE}}_t {{FIELD_NAME}}() const;";
    }

    return code.ToString();
}

std::string class_writer_t::generate_incoming_relationships_accessors_cpp()
{
    flatbuffers::CodeWriter code = create_code_writer();

    // Iterate over the relationships where the current table is the child
    for (auto& relationship : m_table.incoming_relationships())
    {
        code.SetValue("FIELD_NAME", relationship.field_name());
        code.SetValue("PARENT_TABLE", relationship.parent_table());
        code.SetValue("PARENT_OFFSET", relationship.parent_offset());

        code += "{{PARENT_TABLE}}_t {{TABLE_NAME}}_t::{{FIELD_NAME}}() const {";
        code.IncrementIdentLevel();
        code += "return {{PARENT_TABLE}}_t::get(this->references()[{{PARENT_OFFSET}}]);";
        code.DecrementIdentLevel();
        code += "}";
    }

    return code.ToString();
}

std::string class_writer_t::generate_outgoing_relationships_accessors()
{
    flatbuffers::CodeWriter code = create_code_writer();

    // Iterate over the relationships where the current table appear as parent
    for (auto& relationship : m_table.outgoing_relationships())
    {
        if (relationship.is_one_to_many())
        {
            code.SetValue("FIELD_NAME", relationship.field_name());
            code += "{{FIELD_NAME}}_list_t {{FIELD_NAME}}() const;";
        }
        else if (relationship.is_one_to_one())
        {
            code.SetValue("FIELD_NAME", relationship.field_name());
            code.SetValue("CHILD_TABLE", relationship.child_table());
            code += "{{CHILD_TABLE}}_ref_t {{FIELD_NAME}}() const; ";
        }
        else
        {
            ASSERT_UNREACHABLE("Unsupported relationship cardinality!");
        }
    }

    return code.ToString();
}

std::string class_writer_t::generate_outgoing_relationships_accessors_cpp()
{
    flatbuffers::CodeWriter code = create_code_writer();

    // Iterate over the relationships where the current table appear as parent
    for (auto& relationship : m_table.outgoing_relationships())
    {
        code.SetValue("CHILD_TABLE", relationship.child_table());
        code.SetValue("FIELD_NAME", relationship.field_name());
        code.SetValue("FIRST_OFFSET", relationship.first_offset());
        code.SetValue("NEXT_OFFSET", relationship.next_offset());

        if (relationship.is_one_to_many())
        {
            code += "{{TABLE_NAME}}_t::{{FIELD_NAME}}_list_t {{TABLE_NAME}}_t::{{FIELD_NAME}}() const {";
            code.IncrementIdentLevel();
            code += "return {{TABLE_NAME}}_t::{{FIELD_NAME}}_list_t(gaia_id(), {{FIRST_OFFSET}}, {{NEXT_OFFSET}});";
            code.DecrementIdentLevel();
            code += "}";
        }
        else if (relationship.is_one_to_one())
        {
            code += "{{CHILD_TABLE}}_ref_t {{TABLE_NAME}}_t::{{FIELD_NAME}}() const {";
            code.IncrementIdentLevel();
            code += "return {{CHILD_TABLE}}_ref_t(gaia_id(), this->references()[{{FIRST_OFFSET}}], {{FIRST_OFFSET}});";
            code.DecrementIdentLevel();
            code += "}";
        }
        else
        {
            ASSERT_UNREACHABLE("Unsupported relationship cardinality!");
        }
    }

    return code.ToString();
}

std::string class_writer_t::generate_expressions()
{
    flatbuffers::CodeWriter code = create_code_writer();

    // Add EDC expressions and use a dummy template to emulate C++17 inline variable
    // declarations with C++11 legal syntax.
    code += "template<class unused_t>";
    code += "struct expr_ {";
    code.IncrementIdentLevel();

    std::pair<std::string, std::string> expr_variable;

    expr_variable = field_facade_t::generate_expr_variable(code.GetValue("TABLE_NAME"), "gaia::common::gaia_id_t", "gaia_id");
    code += expr_variable.first;

    for (const auto& field : m_table.fields())
    {
        expr_variable = field.generate_expr_variable();
        code += expr_variable.first;
    }

    for (auto& relationship : m_table.incoming_relationships())
    {
        expr_variable = field_facade_t::generate_expr_variable(m_table.table_name(), relationship.target_type(), relationship.field_name());
        code += expr_variable.first;
    }

    for (auto& relationship : m_table.outgoing_relationships())
    {
        expr_variable = field_facade_t::generate_expr_variable(m_table.table_name(), relationship.target_type(), relationship.field_name());
        code += expr_variable.first;
    }

    code.DecrementIdentLevel();
    code += "};";
    code += "using expr = expr_<void>;";
    return code.ToString();
}

std::string class_writer_t::generate_private_constructor()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "explicit {{TABLE_NAME}}_t(gaia::common::gaia_id_t id) : edc_object_t(id, \"{{TABLE_NAME}}_t\") {}";
    return code.ToString();
}

std::string class_writer_t::generate_friend_declarations()
{
    flatbuffers::CodeWriter code = create_code_writer();
    code += "friend class edc_object_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t, internal::{{TABLE_NAME}}, "
            "internal::{{TABLE_NAME}}T>;";
    code += "friend class {{TABLE_NAME}}_ref_t;";
    return code.ToString();
}

std::string class_writer_t::generate_close_class_definition()
{
    return "};\n";
}

void class_writer_t::increment_indent()
{
    m_indent_level++;
}

void class_writer_t::decrement_indent()
{
    ASSERT_PRECONDITION(m_indent_level > 0, "Indent level cannot be go negative.");
    m_indent_level--;
}

std::string class_writer_t::generate_method_params(std::vector<field_facade_t> fields)
{
    std::string param_list;

    bool first = true;
    for (const auto& field : fields)
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
        param_list += field.field_type(is_function_parameter) + " ";
        param_list += field.field_name();
    }
    return param_list;
}

std::string class_writer_t::generate_expr_namespace()
{
    flatbuffers::CodeWriter code = create_code_writer();

    code += "namespace {{TABLE_NAME}}_expr {";
    code.IncrementIdentLevel();
    code += "static auto& gaia_id = {{TABLE_NAME}}_t::expr::gaia_id;";

    for (const field_facade_t& field : m_table.fields())
    {
        code.SetValue("FIELD_NAME", field.field_name());
        code += "static auto& {{FIELD_NAME}} = {{TABLE_NAME}}_t::expr::{{FIELD_NAME}};";
    }

    for (auto& relationship : m_table.incoming_relationships())
    {
        code.SetValue("FIELD_NAME", relationship.field_name());
        code += "static auto& {{FIELD_NAME}} = {{TABLE_NAME}}_t::expr::{{FIELD_NAME}};";
    }

    for (auto& relationship : m_table.outgoing_relationships())
    {
        code.SetValue("FIELD_NAME", relationship.field_name());
        code += "static auto& {{FIELD_NAME}} = {{TABLE_NAME}}_t::expr::{{FIELD_NAME}};";
    }
    code.DecrementIdentLevel();
    code += "};";

    return code.ToString();
}

std::string class_writer_t::generate_expr_instantiation_cpp()
{
    flatbuffers::CodeWriter code = create_code_writer();

    // Initialization of static EDC expressions. For C++11 compliance we are not using
    // inline variables which are available in C++17.
    std::pair<std::string, std::string> expr_variable;

    expr_variable = field_facade_t::generate_expr_variable(m_table.table_name(), "gaia::common::gaia_id_t", "gaia_id");
    code += expr_variable.second;

    for (const auto& field : m_table.fields())
    {
        expr_variable = field.generate_expr_variable();
        code += expr_variable.second;
    }

    for (auto& relationship : m_table.incoming_relationships())
    {
        expr_variable = field_facade_t::generate_expr_variable(m_table.table_name(), relationship.target_type(), relationship.field_name());
        code += expr_variable.second;
    }

    for (auto& relationship : m_table.outgoing_relationships())
    {
        expr_variable = field_facade_t::generate_expr_variable(m_table.table_name(), relationship.target_type(), relationship.field_name());
        code += expr_variable.second;
    }

    return code.ToString();
}

std::string class_writer_t::generate_ref_class()
{
    flatbuffers::CodeWriter code = create_code_writer();

    code += "class {{TABLE_NAME}}_ref_t : public {{TABLE_NAME}}_t, direct_access::edc_ref_t {";
    code += "public:";
    code.IncrementIdentLevel();
    code += "{{TABLE_NAME}}_ref_t() = delete;";
    code += "{{TABLE_NAME}}_ref_t(gaia::common::gaia_id_t parent, gaia::common::gaia_id_t child, "
            "gaia::common::reference_offset_t child_offset);";
    code += "void disconnect();";
    code += "void connect(gaia::common::gaia_id_t id);";
    code += "void connect(const {{TABLE_NAME}}_t& object);";
    code.DecrementIdentLevel();
    code += "};";
    return code.ToString();
}

std::string class_writer_t::generate_ref_class_cpp()
{
    flatbuffers::CodeWriter code = create_code_writer();

    // Constructor.
    code += "{{TABLE_NAME}}_ref_t::{{TABLE_NAME}}_ref_t(gaia::common::gaia_id_t parent, "
            "gaia::common::gaia_id_t child, gaia::common::reference_offset_t child_offset)";
    code.IncrementIdentLevel();
    code += ": {{TABLE_NAME}}_t(child), direct_access::edc_ref_t(parent, child_offset) {};";
    code.DecrementIdentLevel();

    // disconnect()
    code += "void {{TABLE_NAME}}_ref_t::disconnect() {";
    code.IncrementIdentLevel();
    code += "edc_ref_t::disconnect(this->gaia_id());";
    code.DecrementIdentLevel();
    code += "}";

    // connect(gaia_id_t)
    code += "void {{TABLE_NAME}}_ref_t::connect(gaia::common::gaia_id_t id) {";
    code.IncrementIdentLevel();
    code += "edc_ref_t::connect(this->gaia_id(), id);";
    code.DecrementIdentLevel();
    code += "}";

    // connect(edc_class_t)
    code += "void {{TABLE_NAME}}_ref_t::connect(const {{TABLE_NAME}}_t& object) {";
    code.IncrementIdentLevel();
    code += "connect(object.gaia_id());";
    code.DecrementIdentLevel();
    code += "}";

    return code.ToString();
}

} // namespace generate
} // namespace catalog
} // namespace gaia
