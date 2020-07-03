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

using table_names_t = unordered_map<string, gaia_id_t>;
using table_fields_t = unordered_map<gaia_id_t, vector<gaia_id_t>>;

class catalog_manager_t {
  public:
    /**
     * Catalog manager scaffolding to ensure we have one global static instance
     */
    catalog_manager_t(catalog_manager_t &) = delete;
    void operator=(catalog_manager_t const &) = delete;
    static catalog_manager_t &get();

    /*
    ** APIs for accessing catalog records
    */
    gaia_id_t create_table(const string &name, const vector<ddl::field_definition_t *> &fields);

    const set<gaia_id_t> &list_tables() const;
    const vector<gaia_id_t> &list_fields(gaia_id_t table_id) const;

  private:
    // Only internal static creation is allowed
    catalog_manager_t() {}
    ~catalog_manager_t() {}

    // Maintain some in-memory cache for fast lookup.
    // We can switch to use value index when the feature is ready.
    table_names_t m_table_names;
    table_fields_t m_table_fields;
    set<gaia_id_t> m_table_ids;
};
} // namespace catalog
} // namespace gaia
