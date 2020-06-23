/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "catalog_manager.hpp"
#include "catalog_gaia_generated.h"
#include "gaia_exception.hpp"

using namespace gaia::catalog;

/**
 * Catalog public APIs
 **/
gaia_id_t gaia::catalog::create_table(std::string name,
    const std::vector<ddl::field_definition_t *> &fields) {
    return catalog_manager_t::get().create_table(name, fields);
}

static gaia_data_type to_gaia_data_type(ddl::data_type_t data_type) {
    switch (data_type) {
    case ddl::data_type_t::BOOL:
        return gaia_data_type_BOOL;
    case ddl::data_type_t::INT8:
        return gaia_data_type_INT8;
    case ddl::data_type_t::UINT8:
        return gaia_data_type_UINT8;
    case ddl::data_type_t::INT16:
        return gaia_data_type_INT16;
    case ddl::data_type_t::UINT16:
        return gaia_data_type_UINT16;
    case ddl::data_type_t::INT32:
        return gaia_data_type_INT32;
    case ddl::data_type_t::UINT32:
        return gaia_data_type_UINT32;
    case ddl::data_type_t::INT64:
        return gaia_data_type_INT64;
    case ddl::data_type_t::UINT64:
        return gaia_data_type_UINT64;
    case ddl::data_type_t::FLOAT32:
        return gaia_data_type_FLOAT32;
    case ddl::data_type_t::FLOAT64:
        return gaia_data_type_FLOAT64;
    case ddl::data_type_t::STRING:
        return gaia_data_type_STRING;
    default:
        throw gaia::common::gaia_exception("Unknown type");
    }
}

/**
 * Class methods
 **/
catalog_manager_t &catalog_manager_t::get() {
    static catalog_manager_t s_instance;
    return s_instance;
}

gaia_id_t catalog_manager_t::create_table(std::string name,
    const std::vector<ddl::field_definition_t *> &fields) {
    if (m_table_cache.find(name) != m_table_cache.end()) {
        throw gaia::common::gaia_exception("The table " + name +
                                           " already exists.");
    }

    gaia::db::begin_transaction();
    gaia_id_t table_id = Gaia_table::insert_row(
        name.c_str(),               // name
        false,                      // is_log
        gaia_trim_action_type_NONE, // trim_action
        0,                          // max_rows
        0,                          // max_size
        0                           // max_seconds
    );
    uint16_t position = 0;
    for (ddl::field_definition_t *field : fields) {
        Gaia_field::insert_row(
            field->name.c_str(),            // name
            table_id,                       // table_id
            to_gaia_data_type(field->type), // type
            field->length,                  // repeated_count
            ++position,                     // position
            true,                           // required
            false,                          // deprecated
            false,                          // active
            true,                           // nullable
            false,                          // has_default
            ""                              // default value
        );
    }
    gaia::db::commit_transaction();

    m_table_cache[name] = table_id;
    return table_id;
}
