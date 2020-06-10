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

gaia_id_t gaia::catalog::create_type(std::string name,
    std::vector<ddl::field_definition_t *> *fields) {
    return catalog_manager_t::get().create_type(name, fields);
}

gaia_id_t gaia::catalog::create_table_of(std::string tableName,
    std::string typeName) {
    return catalog_manager_t::get().create_table_of(tableName, typeName);
}

std::pair<gaia_id_t, gaia_id_t> gaia::catalog::create_table(std::string name,
    std::vector<ddl::field_definition_t *> *fields) {
    return catalog_manager_t::get().create_table(name, fields);
}

/**
 * catalog_manager class
 **/
void catalog_manager_t::init(bool is_engine) {
    if (is_engine) {
        gaia::db::begin_transaction();
        for (GaiaDataType value : EnumValuesGaiaDataType()) {
            if (value != GaiaDataType_TABLE) {
                GaiaType::insert_row(EnumNameGaiaDataType(value), value);
            }
        }
        for (unique_ptr<GaiaType> t{GaiaType::get_first()}; t; t.reset(t->get_next())) {
            m_type_cache[t->name()] = t->gaia_id();
        }
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

static GaiaDataType to_gaia_type(ddl::data_type_t dt) {
    switch (dt) {
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

gaia_id_t catalog_manager_t::create_type(std::string name,
    std::vector<ddl::field_definition_t *> *fields) {
    if (m_type_cache.find(name) != m_type_cache.end()) {
        throw gaia::common::gaia_exception("The type " + name +
                                           " already exists.");
    }

    gaia::db::begin_transaction();
    gaia_id_t owner_id = GaiaType::insert_row(name.c_str(), GaiaDataType_TABLE);
    uint16_t position = 0;
    for (ddl::field_definition_t *field : *fields) {
        gaia_id_t type_id;
        if (field->type == ddl::data_type_t::TABLE) {
            if (m_type_cache.find(field->table_type_name) == m_type_cache.end()) {
                throw gaia::common::gaia_exception(
                    "The type " + field->table_type_name + " does not exist.");
            } else {
                type_id = m_type_cache[field->table_type_name];
            }
        } else {
            type_id =
                m_type_cache[EnumNameGaiaDataType(to_gaia_type(field->type))];
        }
        GaiaField::insert_row(field->name.c_str(), owner_id, type_id,
            field->length, ++position, true, false);
    }
    gaia::db::commit_transaction();

    m_type_cache[name] = owner_id;
    return owner_id;
}

gaia_id_t catalog_manager_t::create_table_of(
    std::string tableName,
    std::string typeName) {
    if (m_type_cache.find(typeName) == m_type_cache.end()) {
        throw gaia::common::gaia_exception("The type " + typeName +
                                           " does not exist.");
    }
    if (m_table_cache.find(tableName) != m_table_cache.end()) {
        throw gaia::common::gaia_exception("The table " + tableName +
                                           " already exists.");
    }
    gaia::db::begin_transaction();
    gaia_id_t table_id = GaiaTable::insert_row(tableName.c_str(),
        m_type_cache[typeName], false, GaiaTrimActionType_NONE, 0, 0, 0);
    gaia::db::commit_transaction();

    m_table_cache[tableName] = table_id;
    return table_id;
}

std::pair<gaia_id_t, gaia_id_t> catalog_manager_t::create_table(
    std::string name,
    std::vector<ddl::field_definition_t *> *fields) {
    gaia_id_t type_id = create_type(name, fields);
    gaia_id_t table_id = create_table_of(name, name);
    return std::make_pair(table_id, type_id);
}
