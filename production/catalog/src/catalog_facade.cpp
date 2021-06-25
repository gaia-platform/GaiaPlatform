/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/catalog/catalog_facade.hpp"

#include "gaia_internal/catalog/catalog.hpp"

namespace gaia
{
namespace catalog
{
namespace generate
{

//
// database_facade_t
//

database_facade_t::database_facade_t(gaia_database_t database)
    : m_database(std::move(database))
{
}

std::string database_facade_t::database_name() const
{
    return m_database.name();
}

std::vector<table_facade_t> database_facade_t::tables() const
{
    auto tables = std::vector<table_facade_t>();

    for (auto& table : m_database.gaia_tables())
    {
        tables.emplace_back(table);
    }

    return tables;
}

bool database_facade_t::is_default_database()
{
    return database_name().empty();
}

//
// table_facade_t
//

table_facade_t::table_facade_t(gaia_table_t table)
    : m_table(std::move(table))
{
}

std::string table_facade_t::table_name() const
{
    return m_table.name();
}

std::string table_facade_t::table_type() const
{
    return std::to_string(m_table.type());
}

std::string table_facade_t::class_name() const
{
    return std::string(m_table.name()) + "_t";
}

std::vector<field_facade_t> table_facade_t::fields() const
{
    auto fields = std::vector<field_facade_t>();

    // Direct access reference list API guarantees LIFO. As long as we only
    // allow appending new fields to table definitions, reversing the field list
    // order should result in fields being listed in the ascending order of
    // their positions. This is essential to correctly generate the insert()
    // method.
    for (auto& field : m_table.gaia_fields())
    {
        fields.insert(fields.begin(), field_facade_t{field});
    }

    return fields;
}

bool table_facade_t::has_string_or_vector() const
{
    for (auto& field : m_table.gaia_fields())
    {
        if (field.type() == static_cast<uint8_t>(data_type_t::e_string)
            || field.repeated_count() != 1)
        {
            return true;
        }
    }
    return false;
}

bool table_facade_t::needs_reference_class() const
{
    for (const link_facade_t& link : incoming_links())
    {
        if (link.is_one_to_one())
        {
            return true;
        }
    }
    return false;
}

std::vector<link_facade_t> table_facade_t::incoming_links() const
{
    auto links = std::vector<link_facade_t>();

    for (const gaia_relationship_t& relationship : m_table.incoming_relationships())
    {
        links.emplace_back(relationship, false);
    }

    return links;
}

std::vector<link_facade_t> table_facade_t::outgoing_links() const
{
    auto links = std::vector<link_facade_t>();

    for (const gaia_relationship_t& relationship : m_table.outgoing_relationships())
    {
        links.emplace_back(relationship, true);
    }

    return links;
}

//
// field_facade_t
//

field_facade_t::field_facade_t(gaia_field_t field)
    : m_field(std::move(field))
{
}

std::string field_facade_t::field_name() const
{
    return m_field.name();
}

std::string field_facade_t::field_type(bool is_function_parameter) const
{
    std::string type_str;

    switch (static_cast<data_type_t>(m_field.type()))
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

    if (m_field.repeated_count() == 0)
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
    else if (m_field.repeated_count() > 1)
    {
        // We should report the input error to the user at data definition time.
        // If we find a fixed size array definition here, it will be either data
        // corruption or bugs in catching user input errors.
        ASSERT_UNREACHABLE("Fixed size array is not supported");
    }
    return type_str;
}

bool field_facade_t::is_string() const
{
    return m_field.type() == static_cast<uint8_t>(data_type_t::e_string);
}

bool field_facade_t::is_vector() const
{
    return m_field.repeated_count() != 1;
}

std::pair<std::string, std::string> field_facade_t::generate_expr_variable() const
{
    return generate_expr_variable(table_name(), field_type(), field_name());
}

std::string field_facade_t::table_name() const
{
    return m_field.table().name();
}

std::pair<std::string, std::string> field_facade_t::generate_expr_variable(const std::string& table, const std::string& type, const std::string& field)
{
    std::string expr_decl;
    std::string expr_init;
    std::string type_decl;

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

//
// link_facade_t
//

link_facade_t::link_facade_t(gaia::catalog::gaia_relationship_t relationship, bool is_from_parent)
    : m_relationship(std::move(relationship)), m_is_from_parent(is_from_parent)
{
}

std::string link_facade_t::field_name() const
{
    if (m_is_from_parent)
    {
        return m_relationship.to_child_link_name();
    }
    return m_relationship.to_parent_link_name();
}

std::string link_facade_t::from_table() const
{
    if (m_is_from_parent)
    {
        return m_relationship.parent().name();
    }
    return m_relationship.child().name();
}

std::string link_facade_t::to_table() const
{
    if (m_is_from_parent)
    {
        return m_relationship.child().name();
    }
    return m_relationship.parent().name();
}

bool link_facade_t::is_single_cardinality() const
{
    return !m_is_from_parent || is_one_to_one();
}

bool link_facade_t::is_multiple_cardinality() const
{
    return m_is_from_parent && is_one_to_many();
}

bool link_facade_t::is_one_to_one() const
{
    return static_cast<relationship_cardinality_t>(m_relationship.cardinality())
        == relationship_cardinality_t::one;
}

bool link_facade_t::is_one_to_many() const
{
    return static_cast<relationship_cardinality_t>(m_relationship.cardinality())
        == relationship_cardinality_t::many;
}

std::string link_facade_t::target_type() const
{
    if (m_is_from_parent)
    {
        std::string type;
        if (is_multiple_cardinality())
        {
            type.append(from_table());
            type.append("_t::");
            type.append(field_name());
            type.append("_list_t");
        }
        else if (is_single_cardinality())
        {
            type.append(to_table());
            type.append("_ref_t");
        }
        else
        {
            ASSERT_UNREACHABLE("Unsupported relationship cardinality!");
        }

        return type;
    }
    else
    {
        std::string type;
        type.append(to_table());
        type.append("_t");
        return type;
    }
}

} // namespace generate
} // namespace catalog
} // namespace gaia
