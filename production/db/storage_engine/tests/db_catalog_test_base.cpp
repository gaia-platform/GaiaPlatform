/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_catalog_test_base.hpp"

#include "ddl_executor.hpp"
#include "schema_loader.hpp"
#include "type_id_mapping.hpp"

namespace gaia
{
namespace db
{

void db_catalog_test_base_t::reset_database_status()
{
    type_id_mapping_t::instance().clear();
    gaia::catalog::ddl_executor_t::get().reset();
}

void db_catalog_test_base_t::load_schema(std::string ddl_file_name)
{
    schema_loader_t::instance().load_schema(ddl_file_name);
}

} // namespace db
} // namespace gaia
