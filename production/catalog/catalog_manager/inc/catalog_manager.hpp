#ifndef __CATALOG_H_
#define __CATALOG_H_
#include "gaia_catalog.hpp"
#include <unordered_map>

namespace gaia {
namespace catalog {
class catalog_manager_t {
  public:
    /**
     * Catalog manager scaffolding to ensure we have one global static instance.
     * Do not allow assignment or copying; this class is a singleton.
     * The implementation follows event_manager convention.
     */
    catalog_manager_t(catalog_manager_t &) = delete;
    void operator=(catalog_manager_t const &) = delete;
    static catalog_manager_t &get(bool is_initializing = false);

    void init(bool is_engine);

    /*
    ** DDL APIs
    */
    void create_type(std::string name,
                     std::vector<ddl::FieldDefinition *> *fields);

    void create_table_of(std::string tableName, std::string typeName);

    void create_table(std::string name,
                      std::vector<ddl::FieldDefinition *> *fields);

  private:
    // only internal static creation is allowed
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

#endif // __CATALOG_H_
