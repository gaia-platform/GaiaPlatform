/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_catalog_test_base.hpp"

#include "ddl_executor.hpp"
#include "schema_loader.hpp"
#include "type_id_record_id_cache.hpp"

namespace gaia
{
namespace db
{

void db_catalog_test_base_t::reset_database_status()
{
    type_id_record_id_cache_t::instance().clear();
    gaia_boot_t::get().reset_gaia_boot();
    gaia::catalog::ddl_executor_t::get().init();
}

void db_catalog_test_base_t::load_dll(std::string ddl_file_name)
{
    schema_loader_t::instance().load_schema(ddl_file_name);
}

} // namespace db
} // namespace gaia
