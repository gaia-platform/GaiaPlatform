/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_catalog.h"
#include "catalog_manager.hpp"
#include "gaia_exception.hpp"
#include "fbs_generator.hpp"
#include "retail_assert.hpp"
#include "system_table_types.hpp"
#include "system_base.hpp"
#include <memory>

using namespace gaia::catalog::ddl;

/**
 * Catalog public APIs
 **/
namespace gaia {
namespace catalog {

static constexpr char c_empty_c_str[] = "";

gaia_id_t create_database(const string &name, bool throw_on_exists) {
    return catalog_manager_t::get().create_database(name, throw_on_exists);
}

gaia_id_t create_table(const string &name,
    const field_def_list_t &fields) {
    return catalog_manager_t::get().create_table(c_empty_c_str, name, fields);
}

gaia_id_t create_table(
    const string &dbname,
    const string &name,
    const field_def_list_t &fields,
    bool throw_on_exists) {
    return catalog_manager_t::get().create_table(dbname, name, fields, throw_on_exists);
}

void drop_table(const string &name) {
    return catalog_manager_t::get().drop_table(c_empty_c_str, name);
}

void drop_table(const string &dbname, const string &name) {
    return catalog_manager_t::get().drop_table(dbname, name);
}

vector<gaia_id_t> list_fields(gaia_id_t table_id) {
    return catalog_manager_t::get().list_fields(table_id);
}

vector<gaia_id_t> list_references(gaia_id_t table_id) {
    return catalog_manager_t::get().list_references(table_id);
}

gaia_id_t find_db_id(const string &dbname) {
    return catalog_manager_t::get().find_db_id(dbname);
}

/**
 * Class methods
 **/

catalog_manager_t::catalog_manager_t() {
    init();
}

catalog_manager_t &catalog_manager_t::get() {
    static catalog_manager_t s_instance;
    return s_instance;
}

void catalog_manager_t::bootstrap_catalog() {
    create_database("catalog", false);
    {
        // create table gaia_database (name string);
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        create_table_impl("catalog", "gaia_database", fields, true, false, static_cast<gaia_id_t>(catalog_table_type_t::gaia_database));
    }
    {
        // create table gaia_table (
        //     name string,
        //     is_system bool,
        //     max_rows uint64,
        //     max_size uint64,
        //     max_seconds uint64,
        //     binary_schema string,
        //     references gaia_database,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>("type", data_type_t::e_uint32, 1));
        fields.emplace_back(make_unique<field_definition_t>("is_system", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<field_definition_t>("trim_action", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<field_definition_t>("max_rows", data_type_t::e_uint64, 1));
        fields.emplace_back(make_unique<field_definition_t>("max_size", data_type_t::e_uint64, 1));
        fields.emplace_back(make_unique<field_definition_t>("max_seconds", data_type_t::e_uint64, 1));
        fields.emplace_back(make_unique<field_definition_t>("binary_schema", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>(c_empty_c_str, data_type_t::e_references, 1, "catalog.gaia_database"));
        create_table_impl("catalog", "gaia_table", fields, true, false, static_cast<gaia_id_t>(catalog_table_type_t::gaia_table));
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
        fields.emplace_back(make_unique<field_definition_t>(c_empty_c_str, data_type_t::e_references, 1, "catalog.gaia_table"));
        // The "ref" named reference to the gaia_table defines the referential relationship.
        fields.emplace_back(make_unique<field_definition_t>("ref", data_type_t::e_references, 1, "catalog.gaia_table"));
        create_table_impl("catalog", "gaia_field", fields, true, false, static_cast<gaia_id_t>(catalog_table_type_t::gaia_field));
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
        create_table_impl("catalog", "gaia_ruleset", fields, true, false, static_cast<gaia_id_t>(catalog_table_type_t::gaia_ruleset));
    }
    {
        // create table gaia_rule (
        //     name string,
        //     ruleset_id bool,
        //     references gaia_ruleset,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>(c_empty_c_str, data_type_t::e_references, 1, "catalog.gaia_ruleset"));
        create_table_impl("catalog", "gaia_rule", fields, true, false, static_cast<gaia_id_t>(catalog_table_type_t::gaia_rule));
    }
}

void catalog_manager_t::create_system_tables() {
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
        create_table_impl("event_log", "event_log", fields, true, false, static_cast<gaia_id_t>(system_table_type_t::event_log));
    }
}

void catalog_manager_t::init() {
    reload_cache();
    bootstrap_catalog();
    create_system_tables();
    // Create the special global database.
    // Tables created without specifying a database name will belong to the global database.
    m_global_db_id = create_database(c_global_db_name, false);
}

void catalog_manager_t::clear_cache() {
    m_table_names.clear();
}

void catalog_manager_t::reload_cache() {
    unique_lock lock(m_lock);

    clear_cache();

    gaia::db::begin_transaction();
    for (auto db : gaia_database_t::list()) {
        m_db_names[db.name()] = db.gaia_id();
    }

    for (auto table : gaia_table_t::list()) {
        string full_table_name = string(table.gaia_database().name()) + "." + string(table.name());
        m_table_names[full_table_name] = table.gaia_id();
    }
    gaia::db::commit_transaction();
}

gaia_id_t catalog_manager_t::create_database(
    const string &name,
    bool throw_on_exist) {
    unique_lock lock(m_lock);
    if (m_db_names.find(name) != m_db_names.end()) {
        if (throw_on_exist) {
            throw db_already_exists(name);
        } else {
            return m_db_names.at(name);
        }
    }
    gaia::db::begin_transaction();
    gaia_id_t id = gaia_database_t::insert_row(name.c_str());
    gaia::db::commit_transaction();
    m_db_names[name] = id;
    return id;
}

gaia_id_t catalog_manager_t::create_table(
    const string &dbname,
    const string &name,
    const field_def_list_t &fields,
    bool throw_on_exists) {
    return create_table_impl(dbname, name, fields, false, throw_on_exists);
}

void catalog_manager_t::drop_table(
    const string &dbname,
    const string &name) {

    unique_lock lock(m_lock);

    if (!dbname.empty() && m_db_names.find(dbname) == m_db_names.end()) {
        throw db_not_exists(dbname);
    }

    string full_table_name = (dbname.empty() ? c_empty_c_str : dbname + ".") + name;
    gaia_id_t db_id = find_db_id_no_lock(dbname);
    retail_assert(db_id != INVALID_GAIA_ID);

    if (m_table_names.find(full_table_name) == m_table_names.end()) {
        throw table_not_exists(name);
    }
    gaia_id_t table_id = m_table_names[full_table_name];

    // Remove all records belong to the table in the catalog tables.
    gaia::db::begin_transaction();
    for (gaia_id_t field_id : list_fields(table_id)) {
        auto field_record = gaia_field_t::get(field_id);
        field_record.delete_row();
    }
    for (gaia_id_t reference_id : list_references(table_id)) {
        auto reference_record = gaia_field_t::get(reference_id);
        reference_record.delete_row();
    }

    auto table_record = gaia_table_t::get(table_id);
    // Unlink the table from its database.
    gaia_database_t::get(db_id).gaia_table_list().erase(table_record);
    // Remove the table.
    table_record.delete_row();
    gaia::db::commit_transaction();

    // Invalidate catalog caches.
    m_table_names.erase(full_table_name);
}

static gaia_ptr insert_gaia_table_row(
    gaia_id_t table_id,
    const char *name,
    gaia_type_t type,
    bool is_system,
    uint8_t trim_action,
    uint64_t max_rows,
    uint64_t max_size,
    uint64_t max_seconds,
    const char *binary_schema) {

    // NOTE: The number of table references must be updated manually for bootstrapping,
    //       when the references of the gaia_table change. The constant from existing catalog
    //       extended data classes "gaia_catalog.h" may be incorrect when that happens.
    static constexpr size_t c_gaia_table_num_refs = c_num_gaia_table_ptrs;

    flatbuffers::FlatBufferBuilder fbb(c_flatbuffer_builder_size);
    fbb.Finish(Creategaia_tableDirect(fbb, name, table_id, is_system, trim_action, max_rows, max_size, max_seconds, binary_schema));

    return gaia_ptr::create(
        table_id,                       // id
        type,                           // type
        c_gaia_table_num_refs,          // num_refs
        fbb.GetSize(),                  // data_size
        fbb.GetBufferPointer()          // data
    );
}

gaia_id_t catalog_manager_t::create_table_impl(
    const string &dbname,
    const string &table_name,
    const field_def_list_t &fields,
    bool is_system,
    bool throw_on_exist,
    gaia_id_t fixed_id) {

    unique_lock lock(m_lock);

    if (!dbname.empty() && m_db_names.find(dbname) == m_db_names.end()) {
        throw db_not_exists(dbname);
    }

    string full_table_name = (dbname.empty() ? c_empty_c_str : dbname + ".") + table_name;
    gaia_id_t db_id = find_db_id_no_lock(dbname);
    retail_assert(db_id != INVALID_GAIA_ID);

    if (m_table_names.find(full_table_name) != m_table_names.end()) {
        if (throw_on_exist) {
            throw table_already_exists(full_table_name);
        } else {
            return m_table_names.at(full_table_name);
        }
    }

    // Check for any duplication in field names.
    // We do this before generating fbs because FlatBuffers schema
    // also does not allow duplicate field names and we may generate
    // invalid fbs without checking duplication first.
    set<string> field_names;
    for (auto &field : fields) {
        if (field_names.find(field->name) != field_names.end()) {
            throw duplicate_field(field->name);
        }
        field_names.insert(field->name);
    }

    string bfbs{generate_bfbs(generate_fbs(dbname, table_name, fields))};

    gaia::db::begin_transaction();
    gaia_id_t table_id;
    if (fixed_id == INVALID_GAIA_ID) {
        table_id = gaia_table_t::insert_row(
            table_name.c_str(),                               // name
            system_base_t::get().get_next_type(),             // table type
            is_system,                                        // is_system
            static_cast<uint8_t>(trim_action_type_t::e_none), // trim_action
            0,                                                // max_rows
            0,                                                // max_size
            0,                                                // max_seconds
            bfbs.c_str()                                      // bfbs
        );
    } else {
        // NOTE: table_id and table type should be independent
        table_id = fixed_id;
        insert_gaia_table_row(
            table_id,                                         // table id
            table_name.c_str(),                               // name
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_table), // table type
            is_system,                                        // is_system
            static_cast<uint8_t>(trim_action_type_t::e_none), // trim_action
            0,                                                // max_rows
            0,                                                // max_size
            0,                                                // max_seconds
            bfbs.c_str()                                      // bfbs
        );
    }

    // Connect the table to the database
    gaia_database_t::get(db_id).gaia_table_list().insert(table_id);

    uint16_t field_position = 0, reference_position = 0;
    for (auto &field : fields) {
        gaia_id_t field_type_id{0};
        uint16_t position;
        if (field->type == data_type_t::e_references) {
            if (field->table_type_name == full_table_name || field->table_type_name == table_name) {
                // We allow a table definition to reference itself (self-referencing).
                field_type_id = table_id;
            } else if (m_table_names.find(field->table_type_name) != m_table_names.end()) {
                // A table definition can reference any of existing tables.
                field_type_id = m_table_names[field->table_type_name];
            } else if (!dbname.empty() && m_table_names.count(dbname + "." + field->table_type_name)) {
                // A table definition can reference existing tables in its own database without specifying the database name.
                field_type_id = m_table_names[dbname + "." + field->table_type_name];
            } else {
                // We cannot find the referenced table.
                // Forward declaration is not supported right now.
                throw table_not_exists(field->table_type_name);
            }
            // The field ID/position values must be a contiguous range from 0 onward.
            position = reference_position++;
        } else {
            position = field_position++;
        }
        gaia_id_t field_id = gaia_field_t::insert_row(
            field->name.c_str(),               // name
            static_cast<uint8_t>(field->type), // type
            field->length,                     // repeated_count
            position,                          // position
            false,                             // deprecated
            field->active                      // active
        );
        // Connect the field to the table it belongs to.
        gaia_table_t::get(table_id).gaia_field_list().insert(field_id);

        if (field->type == data_type_t::e_references) {
            // Connect the referred table to the reference field.
            gaia_table_t::get(field_type_id).ref_gaia_field_list().insert(field_id);
        }
    }
    gaia::db::commit_transaction();

    m_table_names[full_table_name] = table_id;
    return table_id;
}

gaia_id_t catalog_manager_t::find_db_id(const string &dbname) const {
    shared_lock lock(m_lock);
    return find_db_id_no_lock(dbname);
}

inline gaia_id_t catalog_manager_t::find_db_id_no_lock(const string &dbname) const {
    if (dbname.empty()) {
        return m_global_db_id;
    } else if (m_db_names.count(dbname)) {
        return m_db_names.at(dbname);
    } else {
        return INVALID_GAIA_ID;
    }
}

vector<gaia_id_t> catalog_manager_t::list_fields(gaia_id_t table_id) const {
    vector<gaia_id_t> fields;
    // Direct access reference list API guarantees LIFO. As long as we only
    // allow appending new fields to table definitions, reversing the field list
    // order should result in fields being listed in the ascending order of
    // their positions.
    for (auto field : gaia_table_t::get(table_id).gaia_field_list()) {
        if (field.type() != static_cast<uint8_t>(data_type_t::e_references)) {
            fields.insert(fields.begin(), field.gaia_id());
        }
    }
    return fields;
}

vector<gaia_id_t> catalog_manager_t::list_references(gaia_id_t table_id) const {
    vector<gaia_id_t> references;
    // Direct access reference list API guarantees LIFO. As long as we only
    // allow appending new fields to table definitions, reversing the field list
    // order should result in fields being listed in the ascending order of
    // their positions.
    for (auto field : gaia_table_t::get(table_id).gaia_field_list()) {
        if (field.type() == static_cast<uint8_t>(data_type_t::e_references)) {
            references.insert(references.begin(), field.gaia_id());
        }
    }
    return references;
}

} // namespace catalog
} // namespace gaia
