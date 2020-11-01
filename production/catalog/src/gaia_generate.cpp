/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////

#include <memory>
#include <set>
#include <vector>

#include "flatbuffers/code_generators.h"

#include "catalog.hpp"
#include "gaia_catalog.h"
#include "logger.hpp"

using namespace std;

namespace gaia
{
namespace catalog
{

const string c_indent_string("    ");

struct field_strings_t
{
    string name;
    data_type_t type;
};

typedef vector<field_strings_t> field_vec;

struct table_references_t
{
    string name;
    string ref_name;
};

typedef vector<table_references_t> references_vec;
typedef map<gaia_id_t, references_vec> references_map;

// Build the two reference maps, one for the 1: side of the relationship, another for the :N side.
static void build_references_maps(gaia_id_t db_id, references_map& references_1, references_map& references_n)
{
    for (gaia_table_t& table : gaia_database_t::get(db_id).gaia_table_list())
    {
        vector<gaia_relationship_t> relationships{};

        // Direct access reference list API guarantees LIFO. As long as we only
        // allow appending new references to table definitions, reversing the
        // reference field list order should result in references being listed in
        // the ascending order of their positions.
        for (const gaia_relationship_t& relationship : table.child_gaia_relationship_list())
        {
            relationships.insert(relationships.begin(), relationship);
        }

        for (gaia_relationship_t relationship : relationships)
        {
            auto parent_table = relationship.parent_gaia_table();
            references_1[parent_table.gaia_id()].push_back({table.name(), relationship.name()});
            references_n[table.gaia_id()].push_back({parent_table.name(), relationship.name()});
        }
    }
}

static string field_cpp_type_string(data_type_t data_type)
{
    switch (data_type)
    {
    case data_type_t::e_bool:
        return "bool";
    case data_type_t::e_int8:
        return "int8_t";
    case data_type_t::e_uint8:
        return "uint8_t";
    case data_type_t::e_int16:
        return "int16_t";
    case data_type_t::e_uint16:
        return "uint16_t";
    case data_type_t::e_int32:
        return "int32_t";
    case data_type_t::e_uint32:
        return "uint32_t";
    case data_type_t::e_int64:
        return "int64_t";
    case data_type_t::e_uint64:
        return "uint64_t";
    case data_type_t::e_float:
        return "float";
    case data_type_t::e_double:
        return "double";
    case data_type_t::e_string:
        return "const char*";
    default:
        throw gaia::common::gaia_exception("Unknown type");
    }
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
    code += "#include \"gaia_object.hpp\"";
    code += "#include \"{{DBNAME}}_generated.h\"";
    code += "#include \"gaia_iterators.hpp\"";
    code += "";
    code += "using namespace std;";
    code += "using namespace gaia::direct_access;";
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
static string generate_constant_list(const gaia_id_t db_id, references_map& references_1, references_map& references_n)
{
    flatbuffers::CodeWriter code(c_indent_string);
    // A fixed constant is used for the flatbuffer builder constructor.
    code += "";
    code += "// The initial size of the flatbuffer builder buffer.";
    code += "constexpr int c_flatbuffer_builder_size = 128;";
    code += "";
    for (auto& table_record : gaia_database_t::get(db_id).gaia_table_list())
    {
        vector<gaia_relationship_t> relationships{};

        // Direct access reference list API guarantees LIFO. As long as we only
        // allow appending new references to table definitions, reversing the
        // reference field list order should result in references being listed in
        // the ascending order of their positions.
        for (const gaia_relationship_t& relationship : table_record.parent_gaia_relationship_list())
        {
            relationships.insert(relationships.begin(), relationship);
        }

        auto const_count = 0;
        code.SetValue("TABLE_NAME", table_record.name());
        code.SetValue("TABLE_TYPE", to_string(table_record.type()));
        code += "// Constants contained in the {{TABLE_NAME}} object.";
        code += "constexpr uint32_t c_gaia_type_{{TABLE_NAME}} = {{TABLE_TYPE}}u;";
        //        for (auto const& ref : references_1[table_record.gaia_id()])
        //        {
        //            code.SetValue("REF_TABLE", ref.name);
        //
        //            if (ref.ref_name.length())
        //            {
        //                code.SetValue("REF_NAME", ref.ref_name);
        //            }
        //            else
        //            {
        //                // This relationship is anonymous.
        //                code.SetValue("REF_NAME", ref.name);
        //            }
        //
        //            code.SetValue("CONST_VALUE", to_string(const_count++));
        //            code += "constexpr int c_first_{{REF_NAME}}_{{REF_TABLE}} = {{CONST_VALUE}};";
        //        }
        for (auto& relationship : relationships)
        {
            code.SetValue("REF_TABLE", relationship.parent_gaia_table().name());

            if (strlen(relationship.name()))
            {
                code.SetValue("REF_NAME", relationship.name());
            }
            else
            {
                // This relationship is anonymous.
                code.SetValue("REF_NAME", table_record.name());
            }

            code.SetValue("CONST_VALUE", to_string(const_count++));
            code += "constexpr int c_first_{{REF_NAME}}_{{REF_TABLE}} = {{CONST_VALUE}};";
        }

        for (auto const& ref : references_n[table_record.gaia_id()])
        {
            code.SetValue("REF_TABLE", ref.name);

            if (ref.ref_name.length())
            {
                code.SetValue("REF_NAME", ref.ref_name);
                code.SetValue("CONST_VALUE", to_string(const_count++));
                code += "constexpr int c_parent_{{REF_NAME}}_{{REF_TABLE}} = {{CONST_VALUE}};";
                code.SetValue("CONST_VALUE", to_string(const_count++));
                code += "constexpr int c_next_{{REF_NAME}}_{{TABLE_NAME}} = {{CONST_VALUE}};";
            }
            else
            {
                // This relationship is anonymous.
                code.SetValue("CONST_VALUE", to_string(const_count++));
                code += "constexpr int c_parent_{{TABLE_NAME}}_{{REF_TABLE}} = {{CONST_VALUE}};";
                code.SetValue("CONST_VALUE", to_string(const_count++));
                code += "constexpr int c_next_{{TABLE_NAME}}_{{TABLE_NAME}} = {{CONST_VALUE}};";
            }
        }
        code.SetValue("CONST_VALUE", to_string(const_count++));
        code += "constexpr int c_num_{{TABLE_NAME}}_ptrs = {{CONST_VALUE}};";
        code += "";
    }
    string str = code.ToString();
    return str;
}

static string generate_declarations(const gaia_id_t db_id)
{
    flatbuffers::CodeWriter code(c_indent_string);

    for (auto const& table : gaia_database_t::get(db_id).gaia_table_list())
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
    string table_name,
    field_vec& field_strings,
    references_vec& references_1,
    references_vec& references_n)
{
    flatbuffers::CodeWriter code(c_indent_string);

    // Struct statement.
    code.SetValue("TABLE_NAME", table_name);
    code.SetValue("POSITION", to_string(table_type_id));
    code += "typedef gaia_writer_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t, {{TABLE_NAME}}, {{TABLE_NAME}}T, "
            "c_num_{{TABLE_NAME}}_ptrs> {{TABLE_NAME}}_writer;";
    code += "struct {{TABLE_NAME}}_t : public gaia_object_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t, "
            "{{TABLE_NAME}}, {{TABLE_NAME}}T, c_num_{{TABLE_NAME}}_ptrs> {";

    code.IncrementIdentLevel();

    // Default public constructor.
    code += "{{TABLE_NAME}}_t() : gaia_object_t(\"{{TABLE_NAME}}_t\") {}";

    // Below, a flatbuffer method is invoked as Create{{TABLE_NAME}}() or
    // as Create{{TABLE_NAME}}Direct. The choice is determined by whether any of the
    // fields are strings. If at least one is a string, than the Direct variation
    // is used.
    // NOTE: There may be a third variation of this if any of the fields are vectors
    // or possibly arrays.
    bool has_string = false;
    // Accessors.
    for (auto const& f : field_strings)
    {
        code.SetValue("TYPE", field_cpp_type_string(f.type));
        code.SetValue("FIELD_NAME", f.name);
        if (f.type == data_type_t::e_string)
        {
            has_string = true;
            code.SetValue("FCN_NAME", "GET_STR");
        }
        else
        {
            code.SetValue("FCN_NAME", "GET");
        }
        code += "{{TYPE}} {{FIELD_NAME}}() const {return {{FCN_NAME}}({{FIELD_NAME}});}";
    }

    code += "using gaia_object_t::insert_row;";

    // The typed insert_row().
    string param_list("static gaia_id_t insert_row(");
    bool first = true;
    for (auto const& f : field_strings)
    {
        if (!first)
        {
            param_list += ", ";
        }
        else
        {
            first = false;
        }
        param_list += field_cpp_type_string(f.type) + " ";
        param_list += f.name;
    }
    code += param_list + ") {";
    code.IncrementIdentLevel();
    code += "flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);";
    code.SetValue("DIRECT", has_string ? "Direct" : "");
    param_list = "b.Finish(Create{{TABLE_NAME}}{{DIRECT}}(b";
    for (auto const& f : field_strings)
    {
        param_list += ", ";
        param_list += f.name;
    }
    param_list += "));";
    code += param_list;
    code += "return gaia_object_t::insert_row(b);";
    code.DecrementIdentLevel();
    code += "}";

    // The reference to the parent records.
    for (auto const& ref : references_n)
    {
        if (ref.ref_name.length())
        {
            code.SetValue("REF_NAME", ref.ref_name);
            code.SetValue("REF_TABLE", ref.name);
            code += "{{REF_TABLE}}_t {{REF_NAME}}_{{REF_TABLE}}() {";
            code.IncrementIdentLevel();
            code += "return {{REF_TABLE}}_t::get(this->references()[c_parent_{{REF_NAME}}_{{REF_TABLE}}]);";
        }
        else
        {
            // This relationship is anonymous.
            code.SetValue("REF_NAME", ref.name);
            code.SetValue("REF_TABLE", ref.name);
            code += "{{REF_TABLE}}_t {{REF_NAME}}() {";
            code.IncrementIdentLevel();
            code += "return {{REF_TABLE}}_t::get(this->references()[c_parent_{{TABLE_NAME}}_{{REF_TABLE}}]);";
        }
        code.DecrementIdentLevel();
        code += "}";
    }

    // The table range.
    code += "static gaia_container_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t>& list() {";
    code.IncrementIdentLevel();
    code += "static gaia_container_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t> list;";
    code += "return list;";
    code.DecrementIdentLevel();
    code += "}";

    // Iterator objects to scan rows pointed to by this one.
    for (auto const& ref : references_1)
    {
        code.SetValue("REF_TABLE", ref.name);
        if (ref.ref_name.length())
        {
            code.SetValue("REF_NAME", ref.ref_name);

            code += "reference_chain_container_t<{{TABLE_NAME}}_t, {{REF_TABLE}}_t, "
                    "c_parent_{{REF_NAME}}_{{TABLE_NAME}}, "
                    "c_first_{{REF_NAME}}_{{REF_TABLE}}, c_next_{{REF_NAME}}_{{REF_TABLE}}> "
                    "m_{{REF_NAME}}_{{REF_TABLE}}_list;";
            code += "reference_chain_container_t<{{TABLE_NAME}}_t, {{REF_TABLE}}_t, "
                    "c_parent_{{REF_NAME}}_{{TABLE_NAME}}, "
                    "c_first_{{REF_NAME}}_{{REF_TABLE}}, c_next_{{REF_NAME}}_{{REF_TABLE}}>& "
                    "{{REF_NAME}}_{{REF_TABLE}}_list() {";

            code.IncrementIdentLevel();
            code += "return m_{{REF_NAME}}_{{REF_TABLE}}_list;";
        }
        else
        {
            // This relationship is anonymous.
            code.SetValue("REF_NAME", ref.name);

            code += "reference_chain_container_t<{{TABLE_NAME}}_t, {{REF_TABLE}}_t, "
                    "c_parent_{{REF_TABLE}}_{{TABLE_NAME}}, "
                    "c_first_{{REF_NAME}}_{{REF_TABLE}}, c_next_{{REF_NAME}}_{{REF_TABLE}}> m_{{REF_NAME}}_list;";
            code += "reference_chain_container_t<{{TABLE_NAME}}_t, {{REF_TABLE}}_t, "
                    "c_parent_{{REF_TABLE}}_{{TABLE_NAME}}, "
                    "c_first_{{REF_NAME}}_{{REF_TABLE}}, c_next_{{REF_NAME}}_{{REF_TABLE}}>& {{REF_NAME}}_list() {";

            code.IncrementIdentLevel();
            code += "return m_{{REF_NAME}}_list;";
        }
        code.DecrementIdentLevel();
        code += "}";
    }

    // The private area.
    code.DecrementIdentLevel();
    code += "private:";
    code.IncrementIdentLevel();
    code += "friend struct gaia_object_t<c_gaia_type_{{TABLE_NAME}}, {{TABLE_NAME}}_t, {{TABLE_NAME}}, "
            "{{TABLE_NAME}}T, c_num_{{TABLE_NAME}}_ptrs>;";

    // The constructor.
    code += "{{TABLE_NAME}}_t(gaia_id_t id) : gaia_object_t(id, \"{{TABLE_NAME}}_t\") {";
    code.IncrementIdentLevel();
    for (auto const& ref : references_1)
    {
        if (ref.ref_name.length())
        {
            code.SetValue("REF_NAME", ref.ref_name);
            code.SetValue("REF_TABLE", ref.name);
            code += "m_{{REF_NAME}}_{{REF_TABLE}}_list.set_outer(gaia_id());";
        }
        else
        {
            // This relationship is anonymous.
            code.SetValue("REF_NAME", ref.name);
            code += "m_{{REF_NAME}}_list.set_outer(gaia_id());";
        }
    }
    code.DecrementIdentLevel();
    code += "}";

    // Finishing brace.
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

    references_map references_1;
    references_map references_n;
    string code_lines;
    begin_transaction();

    build_references_maps(db_id, references_1, references_n);

    code_lines = generate_boilerplate_top(dbname);

    code_lines += generate_constant_list(db_id, references_1, references_n);

    code_lines += generate_declarations(db_id);

    // This is to workaround the issue of incomplete forward declaration of structs that refer to each other.
    // By collecting the IDs in the sorted set, the structs are generated in the ascending order of their IDs.
    set<gaia_id_t> table_ids;
    for (auto const& table : gaia_database_t::get(db_id).gaia_table_list())
    {
        table_ids.insert(table.gaia_id());
    }
    for (auto table_id : table_ids)
    {
        field_vec field_strings;
        auto table_record = gaia_table_t::get(table_id);
        for (auto field_id : list_fields(table_id))
        {
            gaia_field_t field_record(gaia_field_t::get(field_id));
            field_strings.push_back(
                field_strings_t{field_record.name(), static_cast<data_type_t>(field_record.type())});
        }
        for (auto ref_id : list_references(table_id))
        {
            gaia_field_t ref_record = gaia_field_t::get(ref_id);
        }
        code_lines += generate_edc_struct(
            table_id,
            table_record.name(),
            field_strings,
            references_1[table_id],
            references_n[table_id]);
    }
    commit_transaction();

    code_lines += generate_boilerplate_bottom(dbname);

    return code_lines;
}
} // namespace catalog
} // namespace gaia
