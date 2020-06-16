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
void gaia::catalog::initialize_catalog(bool is_engine) {
    catalog_manager_t::get(true).init(is_engine);
}

gaia_id_t gaia::catalog::create_table(std::string name,
    const std::vector<ddl::field_definition_t *> &fields) {
    return catalog_manager_t::get().create_table(name, fields);
}

/**
 * catalog_manager class
 **/
void catalog_manager_t::init(bool is_engine) {
    if (is_engine) {
        gaia::db::begin_transaction();
        gaia::db::commit_transaction();
    }
    m_is_initialized = true;
}

catalog_manager_t &catalog_manager_t::get(bool is_initializing) {
    static catalog_manager_t s_instance;

    // Initialize errors can happen for two reasons:
    //
    // If we are currently trying to initialize then is_initializing
    // will be true. At this point, we don't expect the instance to be
    // initialized yet.
    //
    // If we are not intializing then we expect the instance to already be
    // initialized
    if (is_initializing == s_instance.m_is_initialized) {
        if (is_initializing) {
            throw gaia::common::gaia_exception(
                "The catalog manager has already been initialized.");
        } else {
            throw gaia::common::gaia_exception(
                "The catalog manager has not been initialized yet.");
        }
    }
    return s_instance;
}

catalog_manager_t::catalog_manager_t() {}

static GaiaDataType to_gaia_data_type(ddl::data_type_t data_type) {
    switch (data_type) {
    case ddl::data_type_t::BOOL:
        return GaiaDataType_BOOL;
    case ddl::data_type_t::INT8:
        return GaiaDataType_INT8;
    case ddl::data_type_t::UINT8:
        return GaiaDataType_UINT8;
    case ddl::data_type_t::INT16:
        return GaiaDataType_INT16;
    case ddl::data_type_t::UINT16:
        return GaiaDataType_UINT16;
    case ddl::data_type_t::INT32:
        return GaiaDataType_INT32;
    case ddl::data_type_t::UINT32:
        return GaiaDataType_UINT32;
    case ddl::data_type_t::INT64:
        return GaiaDataType_INT64;
    case ddl::data_type_t::UINT64:
        return GaiaDataType_UINT64;
    case ddl::data_type_t::FLOAT32:
        return GaiaDataType_FLOAT32;
    case ddl::data_type_t::FLOAT64:
        return GaiaDataType_FLOAT64;
    case ddl::data_type_t::STRING:
        return GaiaDataType_STRING;
    default:
        throw gaia::common::gaia_exception("Unknown type");
    }
}

gaia_id_t catalog_manager_t::create_table(std::string name,
    const std::vector<ddl::field_definition_t *> &fields) {
    if (m_table_cache.find(name) != m_table_cache.end()) {
        throw gaia::common::gaia_exception("The table " + name +
                                           " already exists.");
    }

    gaia::db::begin_transaction();
    gaia_id_t table_id = GaiaTable::insert_row(name.c_str(),
        false, GaiaTrimActionType_NONE, 0, 0, 0);
    uint16_t position = 0;
    for (ddl::field_definition_t *field : fields) {
        gaia_id_t type_id = 0;
        if (field->type == ddl::data_type_t::TABLE) {
            if (m_table_cache.find(field->table_type_name) == m_table_cache.end()) {
                throw gaia::common::gaia_exception(
                    "The field table type " + field->table_type_name + " does not exist.");
            } else if (type_id == table_id) {
                throw gaia::common::gaia_exception(
                    "The field table type " + field->table_type_name + " cannot be itself.");
            } else {
                type_id = m_table_cache[field->table_type_name];
            }
        }
        GaiaField::insert_row(
            field->name.c_str(),
            table_id,
            to_gaia_data_type(field->type),
            type_id,
            field->length,
            ++position,
            true,  // required
            false, // deprecated
            false, // active
            true,  // nullable
            false, // has_default
            "",    // default value
            false, // is_foreign_key
            0      // references
        );
    }
    gaia::db::commit_transaction();

    m_table_cache[name] = table_id;
    return table_id;
}
