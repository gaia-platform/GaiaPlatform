////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

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
