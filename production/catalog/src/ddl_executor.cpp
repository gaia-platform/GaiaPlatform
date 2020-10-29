/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "ddl_executor.hpp"

#include <memory>

#include "fbs_generator.hpp"
#include "gaia_catalog.h"
#include "gaia_exception.hpp"
#include "json_generator.hpp"
#include "retail_assert.hpp"
#include "system_table_types.hpp"

using namespace gaia::catalog::ddl;

namespace gaia
{
namespace catalog
{

ddl_executor_t::ddl_executor_t()
{
    init();
}

ddl_executor_t& ddl_executor_t::get()
{
    static ddl_executor_t s_instance;
    return s_instance;
}

void ddl_executor_t::bootstrap_catalog()
{
    constexpr char c_anonymous_reference_field_name[] = "";

    create_database("catalog", false);
    {
        // create table gaia_database (name string);
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        create_table_impl(
            "catalog", "gaia_database", fields, true, false,
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_database));
    }
    {
        // create table gaia_table (
        //     name string,
        //     type uint32,
        //     is_system bool,
        //     binary_schema string,
        //     serialization_template string,
        //     references gaia_database,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>("type", data_type_t::e_uint32, 1));
        fields.emplace_back(make_unique<field_definition_t>("is_system", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<field_definition_t>("binary_schema", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>("serialization_template", data_type_t::e_string, 1));
        fields.emplace_back(
            make_unique<field_definition_t>(
                c_anonymous_reference_field_name, data_type_t::e_references, 1, "catalog.gaia_database"));
        create_table_impl(
            "catalog", "gaia_table", fields, true, false,
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_table));
    }
    {
        // create table gaia_field (
        //     name string,
        //     type uint8,
        //     repeated_count uint16,
        //     position uint16,
        //     deprecated bool,
        //     active bool,
        //     references gaia_table,
        //     ref references gaia_table,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>("type", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<field_definition_t>("repeated_count", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<field_definition_t>("position", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<field_definition_t>("deprecated", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<field_definition_t>("active", data_type_t::e_bool, 1));
        // The anonymous reference to the gaia_table defines the ownership.
        fields.emplace_back(make_unique<field_definition_t>(
            c_anonymous_reference_field_name, data_type_t::e_references, 1, "catalog.gaia_table"));
        // The "ref" named reference to the gaia_table defines the referential relationship.
        fields.emplace_back(make_unique<field_definition_t>("ref", data_type_t::e_references, 1, "catalog.gaia_table"));
        create_table_impl(
            "catalog", "gaia_field", fields, true, false,
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_field));
    }
    {
        // create table gaia_ruleset (
        //     name string,
        //     active_on_startup bool,
        //     table_ids string,
        //     source_location string,
        //     serial_stream string,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>("active_on_startup", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<field_definition_t>("table_ids", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>("source_location", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>("serial_stream", data_type_t::e_string, 1));
        create_table_impl(
            "catalog", "gaia_ruleset", fields, true, false,
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_ruleset));
    }
    {
        // create table gaia_rule (
        //     name string,
        //     ruleset_id bool,
        //     references gaia_ruleset,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(
            make_unique<field_definition_t>(
                c_anonymous_reference_field_name, data_type_t::e_references, 1, "catalog.gaia_ruleset"));
        create_table_impl(
            "catalog", "gaia_rule", fields, true, false,
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_rule));
    }
}

void ddl_executor_t::create_system_tables()
{
    create_database("event_log", false);
    {
        // create table event_log (
        //     event_type: uint32,
        //     type_id: uint64,
        //     record_id: uint64,
        //     column_id: uint16,
        //     timestamp: uint64,
        //     rules_invoked: bool
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("event_type", data_type_t::e_uint32, 1));
        fields.emplace_back(make_unique<field_definition_t>("type_id", data_type_t::e_uint32, 1));
        fields.emplace_back(make_unique<field_definition_t>("record_id", data_type_t::e_uint64, 1));
        fields.emplace_back(make_unique<field_definition_t>("column_id", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<field_definition_t>("timestamp", data_type_t::e_uint64, 1));
        fields.emplace_back(make_unique<field_definition_t>("rules_invoked", data_type_t::e_bool, 1));
        create_table_impl(
            "event_log", "event_log", fields, true, false,
            static_cast<gaia_type_t>(system_table_type_t::event_log));
    }
}

void ddl_executor_t::init()
{
    reload_cache();
    bootstrap_catalog();
    create_system_tables();
    // Create the special global database.
    // Tables created without specifying a database name will belong to the global database.
    m_empty_db_id = create_database(c_empty_db_name, false);
}

void ddl_executor_t::clear_cache()
{
    m_table_names.clear();
}

void ddl_executor_t::reload_cache()
{
    unique_lock lock(m_lock);

    clear_cache();

    gaia::db::begin_transaction();
    for (const auto& db : gaia_database_t::list())
    {
        m_db_names[db.name()] = db.gaia_id();
    }

    for (auto& table : gaia_table_t::list())
    {
        m_table_names[get_full_table_name(table.gaia_database().name(), table.name())] = table.gaia_id();
    }
    gaia::db::commit_transaction();
}

gaia_id_t ddl_executor_t::create_database(const string& name, bool throw_on_exist)
{
    unique_lock lock(m_lock);
    if (m_db_names.find(name) != m_db_names.end())
    {
        if (throw_on_exist)
        {
            throw db_already_exists(name);
        }
        else
        {
            return m_db_names.at(name);
        }
    }
    gaia::db::begin_transaction();
    gaia_id_t id = gaia_database_t::insert_row(name.c_str());
    gaia::db::commit_transaction();
    m_db_names[name] = id;
    return id;
}

gaia_id_t ddl_executor_t::create_table(
    const string& db_name,
    const string& name,
    const field_def_list_t& fields,
    bool throw_on_exists)
{
    return create_table_impl(db_name, name, fields, false, throw_on_exists);
}

void ddl_executor_t::drop_table_no_txn(gaia_id_t table_id)
{
    auto table_record = gaia_table_t::get(table_id);

    for (gaia_id_t field_id : list_fields(table_id))
    {
        // Unlink the field and the table.
        table_record.gaia_field_list().erase(field_id);
        // Remove the field.
        gaia_field_t::get(field_id).delete_row();
    }

    for (gaia_id_t reference_id : list_references(table_id))
    {
        // Unlink the reference and the owner table.
        table_record.gaia_field_list().erase(reference_id);
        auto reference_record = gaia_field_t::get(reference_id);
        // Unlink the reference and the referred table.
        reference_record.ref_gaia_table().ref_gaia_field_list().erase(reference_id);
        // Remove the reference.
        reference_record.delete_row();
    }

    // Unlink the table from its database.
    table_record.gaia_database().gaia_table_list().erase(table_record);
    // Remove the table.
    table_record.delete_row();
}

void ddl_executor_t::drop_database(const string& name)
{
    unique_lock lock(m_lock);
    gaia_id_t db_id = find_db_id_no_lock(name);
    if (db_id == INVALID_GAIA_ID)
    {
        throw db_not_exists(name);
    }
    {
        auto_transaction_t txn;
        auto db_record = gaia_database_t::get(db_id);
        vector<gaia_id_t> table_ids;
        for (auto& table : db_record.gaia_table_list())
        {
            table_ids.push_back(table.gaia_id());
        }
        for (gaia_id_t table_id : table_ids)
        {
            drop_table_no_txn(table_id);
        }
        db_record.delete_row();
        txn.commit();
    }
    m_db_names.erase(name);
}

void ddl_executor_t::drop_table(const string& db_name, const string& name)
{

    unique_lock lock(m_lock);

    if (!db_name.empty() && m_db_names.find(db_name) == m_db_names.end())
    {
        throw db_not_exists(db_name);
    }

    string full_table_name = get_full_table_name(db_name, name);
    gaia_id_t db_id = find_db_id_no_lock(db_name);
    retail_assert(db_id != INVALID_GAIA_ID);

    if (m_table_names.find(full_table_name) == m_table_names.end())
    {
        throw table_not_exists(name);
    }
    gaia_id_t table_id = m_table_names[full_table_name];

    {
        auto_transaction_t txn;
        drop_table_no_txn(table_id);
        txn.commit();
    }

    // Invalidate catalog caches.
    m_table_names.erase(full_table_name);
}

gaia_id_t ddl_executor_t::create_table_impl(
    const string& dbname,
    const string& table_name,
    const field_def_list_t& fields,
    bool is_system,
    bool throw_on_exist,
    gaia_type_t fixed_type)
{
    unique_lock lock(m_lock);

    if (!dbname.empty() && m_db_names.find(dbname) == m_db_names.end())
    {
        throw db_not_exists(dbname);
    }

    string full_table_name = get_full_table_name(dbname, table_name);
    gaia_id_t db_id = find_db_id_no_lock(dbname);
    retail_assert(db_id != INVALID_GAIA_ID);

    if (m_table_names.find(full_table_name) != m_table_names.end())
    {
        if (throw_on_exist)
        {
            throw table_already_exists(full_table_name);
        }
        else
        {
            return m_table_names.at(full_table_name);
        }
    }

    // Check for any duplication in field names.
    // We do this before generating fbs because FlatBuffers schema
    // also does not allow duplicate field names and we may generate
    // invalid fbs without checking duplication first.
    set<string> field_names;
    for (const auto& field : fields)
    {
        if (field_names.find(field->name) != field_names.end())
        {
            throw duplicate_field(field->name);
        }
        field_names.insert(field->name);
    }

    string fbs{generate_fbs(dbname, table_name, fields)};
    string bfbs{generate_bfbs(fbs)};
    string bin{generate_bin(fbs, generate_json(fields))};

    gaia::db::begin_transaction();
    gaia_type_t table_type = fixed_type == INVALID_GAIA_TYPE ? gaia_boot_t::get().get_next_type() : fixed_type;
    gaia_id_t table_id = gaia_table_t::insert_row(
        table_name.c_str(),
        table_type,
        is_system,
        bfbs.c_str(),
        bin.c_str());

    // Connect the table to the database
    gaia_database_t::get(db_id).gaia_table_list().insert(table_id);

    uint16_t field_position = 0, reference_position = 0;
    for (const auto& field : fields)
    {
        gaia_id_t field_type_id{0};
        uint16_t position;
        if (field->type == data_type_t::e_references)
        {
            if (field->table_type_name == full_table_name || field->table_type_name == table_name)
            {
                // We allow a table definition to reference itself (self-referencing).
                field_type_id = table_id;
            }
            else if (m_table_names.find(field->table_type_name) != m_table_names.end())
            {
                // A table definition can reference any of existing tables.
                field_type_id = m_table_names[field->table_type_name];
            }
            else if (!dbname.empty() && m_table_names.count(dbname + c_db_table_name_connector + field->table_type_name))
            {
                // A table definition can reference existing tables in its own database
                // without specifying the database name.
                field_type_id = m_table_names[dbname + c_db_table_name_connector + field->table_type_name];
            }
            else
            {
                // We cannot find the referenced table.
                // Forward declaration is not supported right now.
                throw table_not_exists(field->table_type_name);
            }
            // The field ID/position values must be a contiguous range from 0 onward.
            position = reference_position++;
        }
        else
        {
            position = field_position++;
        }
        gaia_id_t field_id = gaia_field_t::insert_row(
            field->name.c_str(), static_cast<uint8_t>(field->type), field->length, position, false, field->active);
        // Connect the field to the table it belongs to.
        gaia_table_t::get(table_id).gaia_field_list().insert(field_id);

        if (field->type == data_type_t::e_references)
        {
            // Connect the referred table to the reference field.
            gaia_table_t::get(field_type_id).ref_gaia_field_list().insert(field_id);
        }
    }
    gaia::db::commit_transaction();

    m_table_names[full_table_name] = table_id;
    return table_id;
}

gaia_id_t ddl_executor_t::find_db_id(const string& dbname) const
{
    shared_lock lock(m_lock);
    return find_db_id_no_lock(dbname);
}

inline gaia_id_t ddl_executor_t::find_db_id_no_lock(const string& dbname) const
{
    if (dbname.empty())
    {
        return m_empty_db_id;
    }
    else if (m_db_names.count(dbname))
    {
        return m_db_names.at(dbname);
    }
    else
    {
        return INVALID_GAIA_ID;
    }
}

string ddl_executor_t::get_full_table_name(const string& db, const string& table)
{
    if (db.empty())
    {
        return table;
    }
    else
    {
        return db + c_db_table_name_connector + table;
    }
}

} // namespace catalog
} // namespace gaia
