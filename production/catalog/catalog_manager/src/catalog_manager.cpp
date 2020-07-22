/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "catalog_manager.hpp"
#include "gaia_exception.hpp"
#include "fbs_generator.hpp"
#include "retail_assert.hpp"
#include <memory>

/**
 * Catalog public APIs
 **/
namespace gaia {
namespace catalog {

gaia_id_t create_table(const string &name,
    const ddl::field_def_list_t &fields) {
    return catalog_manager_t::get().create_table(name, fields);
}

const set<gaia_id_t> &list_tables() {
    return catalog_manager_t::get().list_tables();
}

const vector<gaia_id_t> &list_fields(gaia_id_t table_id) {
    return catalog_manager_t::get().list_fields(table_id);
}

const vector<gaia_id_t> &list_references(gaia_id_t table_id) {
    return catalog_manager_t::get().list_references(table_id);
}

/**
 * Class methods
 **/
catalog_manager_t &catalog_manager_t::get() {
    static catalog_manager_t s_instance;
    return s_instance;
}

gaia_id_t catalog_manager_t::create_table(const string &name,
    const ddl::field_def_list_t &fields) {
    if (m_table_names.find(name) != m_table_names.end()) {
        throw table_already_exists(name);
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

    string bfbs{generate_bfbs(generate_fbs(name, fields))};

    gaia::db::begin_transaction();
    gaia_id_t table_id = Gaia_table::insert_row(
        name.c_str(),                                     // name
        false,                                            // is_log
        static_cast<uint8_t>(trim_action_type_t::e_none), // trim_action
        0,                                                // max_rows
        0,                                                // max_size
        0,                                                // max_seconds
        bfbs.c_str()                                      // bfbs
    );

    uint16_t field_position = 0, reference_position = 0;
    vector<gaia_id_t> field_ids, reference_ids;

    for (auto &field : fields) {
        gaia_id_t field_type_id{0};
        uint16_t position;
        if (field->type == data_type_t::e_references) {
            if (field->table_type_name == name) {
                // We allow a table definition to reference itself (self-referencing).
                field_type_id = table_id;
            } else if (m_table_names.find(field->table_type_name) != m_table_names.end()) {
                // A table definition can refernece any of existing tables.
                field_type_id = m_table_names[field->table_type_name];
            } else {
                // Not self-referencing or reference to an existing table.
                throw table_not_exists(field->table_type_name);
            }
            position = ++reference_position;
        } else {
            position = ++field_position;
        }
        gaia_id_t field_id = Gaia_field::insert_row(
            field->name.c_str(),               // name
            table_id,                          // table_id
            static_cast<uint8_t>(field->type), // type
            field_type_id,                     // type_id
            field->length,                     // repeated_count
            position,                          // position
            true,                              // required
            false,                             // deprecated
            false,                             // active
            true,                              // nullable
            false,                             // has_default
            ""                                 // default value
        );

        if (field->type != data_type_t::e_references) {
            field_ids.push_back(field_id);
        } else {
            reference_ids.push_back(field_id);
        }
    }
    gaia::db::commit_transaction();

    m_table_names[name] = table_id;
    m_table_ids.insert(table_id);
    m_table_fields[table_id] = move(field_ids);
    m_table_references[table_id] = move(reference_ids);
    return table_id;
}

const set<gaia_id_t> &catalog_manager_t::list_tables() const {
    return m_table_ids;
}

const vector<gaia_id_t> &catalog_manager_t::list_fields(gaia_id_t table_id) const {
    return m_table_fields.at(table_id);
}

const vector<gaia_id_t> &catalog_manager_t::list_references(gaia_id_t table_id) const {
    return m_table_references.at(table_id);
}

} // namespace catalog
} // namespace gaia
