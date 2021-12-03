/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "schema_loader.hpp"
#include "type_id_mapping.hpp"

namespace gaia
{
namespace db
{

void db_catalog_test_base_t::reset_database_status()
{
    type_id_mapping_t::instance().clear();
}

void db_catalog_test_base_t::load_schema(std::string ddl_file_name)
{
    schema_loader_t::instance().load_schema(ddl_file_name);
}

} // namespace db
} // namespace gaia
