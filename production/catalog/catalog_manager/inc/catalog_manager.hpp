/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "gaia_catalog.hpp"
#include <unordered_map>
#include <utility>

namespace gaia {
namespace catalog {
class catalog_manager_t {
  public:
    /**
     * Catalog manager scaffolding to ensure we have one global static instance.
     * Do not allow assignment or copying; this class is a singleton.
     */
    catalog_manager_t(catalog_manager_t &) = delete;
    void operator=(catalog_manager_t const &) = delete;
    static catalog_manager_t &get(bool is_initializing = false);

    void init(bool is_engine);

    /*
    ** DDL APIs
    */
    gaia_id_t create_type(std::string name,
        std::vector<ddl::field_definition_t *> *fields);

    gaia_id_t create_table_of(std::string tableName, std::string typeName);

    std::pair<gaia_id_t, gaia_id_t> create_table(std::string name,
        std::vector<ddl::field_definition_t *> *fields);

  private:
    // Only internal static creation is allowed
    catalog_manager_t();
    bool m_is_initialized = false;

    // Maintain some in-memory name caches for fast id lookup.
    // We can drop these when the storage engine begin to support index.
    unordered_map<std::string, gaia_id_t> m_type_cache;
    unordered_map<std::string, gaia_id_t> m_table_cache;
    unordered_map<std::string, gaia_id_t> m_field_cache;
};
} // namespace catalog
} // namespace gaia
