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
    gaia_id_t create_table(std::string name, const std::vector<ddl::field_definition_t *> &fields);

  private:
    // Only internal static creation is allowed
    catalog_manager_t();
    bool m_is_initialized = false;

    // Maintain an in-memory table name cache for fast id lookup.
    unordered_map<std::string, gaia_id_t> m_table_cache;
};
} // namespace catalog
} // namespace gaia
