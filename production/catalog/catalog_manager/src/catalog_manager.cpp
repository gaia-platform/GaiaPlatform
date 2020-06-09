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

void gaia::catalog::create_type(std::string name,
                                std::vector<ddl::field_definition_t *> *fields) {
    catalog_manager_t::get().create_type(name, fields);
}

void gaia::catalog::create_table_of(std::string tableName,
                                    std::string typeName) {
    catalog_manager_t::get().create_table_of(tableName, typeName);
}

void gaia::catalog::create_table(std::string name,
                                 std::vector<ddl::field_definition_t *> *fields) {
    catalog_manager_t::get().create_table(name, fields);
}

/**
 * catalog_manager class
 **/
void catalog_manager_t::init(bool is_engine) {
    m_is_initialized = true;
    if (is_engine) {
        gaia::db::begin_transaction();
        for (GaiaTypeType value : EnumValuesGaiaTypeType()) {
            if (value != GaiaTypeType_TABLE) {
                GaiaType::insert_row(EnumNameGaiaTypeType(value), value);
            }
        }
        for (unique_ptr<GaiaType> t{GaiaType::get_first()}; t;
             t.reset(t->get_next())) {
            m_type_cache[t->name()] = t->gaia_id();
        }
        gaia::db::commit_transaction();
    }
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

static GaiaTypeType to_gaia_type(ddl::data_type_t dt) {
    switch (dt) {
    case ddl::data_type_t::BOOLEAN:
        return GaiaTypeType_BOOLEAN;
    case ddl::data_type_t::BYTE:
        return GaiaTypeType_BYTE;
    case ddl::data_type_t::UBYTE:
        return GaiaTypeType_UBYTE;
    case ddl::data_type_t::SHORT:
        return GaiaTypeType_SHORT;
    case ddl::data_type_t::USHORT:
        return GaiaTypeType_USHORT;
    case ddl::data_type_t::INT:
        return GaiaTypeType_INT;
    case ddl::data_type_t::UINT:
        return GaiaTypeType_UINT;
    case ddl::data_type_t::LONG:
        return GaiaTypeType_LONG;
    case ddl::data_type_t::ULONG:
        return GaiaTypeType_ULONG;
    case ddl::data_type_t::FLOAT:
        return GaiaTypeType_FLOAT;
    case ddl::data_type_t::DOUBLE:
        return GaiaTypeType_DOUBLE;
    case ddl::data_type_t::STRING:
        return GaiaTypeType_STRING;
    default:
        throw gaia::common::gaia_exception("Unknown type");
    }
}

void catalog_manager_t::create_type(
    std::string name, std::vector<ddl::field_definition_t *> *fields) {
    if (m_type_cache.find(name) != m_type_cache.end()) {
        throw gaia::common::gaia_exception("The type " + name +
                                           " already exists.");
    }

    gaia::db::begin_transaction();
    gaia_id_t owner_id = GaiaType::insert_row(name.c_str(), GaiaTypeType_TABLE);
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
                m_type_cache[EnumNameGaiaTypeType(to_gaia_type(field->type))];
        }
        GaiaField::insert_row(field->name.c_str(), owner_id, type_id,
                              field->length, ++position, true, false);
    }
    gaia::db::commit_transaction();

    m_type_cache[name] = owner_id;
}

void catalog_manager_t::create_table_of(std::string tableName,
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
    gaia_id_t table_id =
        GaiaTable::insert_row(tableName.c_str(), m_type_cache[typeName], false,
                              false, GaiaTrimActionType_NONE, 0, 0, 0);
    gaia::db::commit_transaction();

    m_table_cache[tableName] = table_id;
}

void catalog_manager_t::create_table(
    std::string name, std::vector<ddl::field_definition_t *> *fields) {
    create_type(name, fields);
    create_table_of(name, name);
}
