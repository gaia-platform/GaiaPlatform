/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog_tests_helper.hpp"
#include "gaia_internal/db/db_ddl_test_base.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"
#include "gaia_internal/db/gaia_ptr_api.hpp"

#include "field_access.hpp"

using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::catalog;
using namespace gaia::direct_access;
using namespace gaia::db::payload_types;

class test_optional_scalars : public db_ddl_test_base_t
{
};

TEST_F(test_optional_scalars, binary_schema_has_optional_values)
{
    gaia_id_t internal_type_id
        = table_builder_t::new_table("test_optional")
              .field("optional_uint8", data_type_t::e_uint8, table_builder_t::c_optional)
              .field("optional_uint16", data_type_t::e_uint16, table_builder_t::c_optional)
              .field("optional_uint32", data_type_t::e_uint32, table_builder_t::c_optional)
              .field("optional_uint64", data_type_t::e_uint64, table_builder_t::c_optional)
              .field("optional_int8", data_type_t::e_int8, table_builder_t::c_optional)
              .field("optional_int16", data_type_t::e_int16, table_builder_t::c_optional)
              .field("optional_int32", data_type_t::e_int32, table_builder_t::c_optional)
              .field("optional_int64", data_type_t::e_int64, table_builder_t::c_optional)
              .field("optional_float", data_type_t::e_float, table_builder_t::c_optional)
              .field("optional_double", data_type_t::e_double, table_builder_t::c_optional)
              .field("optional_bool", data_type_t::e_bool, table_builder_t::c_optional)
              .create();

    auto_transaction_t txn;

    gaia_table_t table = gaia_table_t::get(internal_type_id);

    const reflection::Schema& schema = *reflection::GetSchema(table.binary_schema().data());
    const reflection::Object* object = schema.root_table();
    auto fields = object->fields();

    ASSERT_TRUE(fields->LookupByKey("optional_uint8")->optional());
    ASSERT_TRUE(fields->LookupByKey("optional_uint16")->optional());
    ASSERT_TRUE(fields->LookupByKey("optional_uint32")->optional());
    ASSERT_TRUE(fields->LookupByKey("optional_uint64")->optional());
    ASSERT_TRUE(fields->LookupByKey("optional_int8")->optional());
    ASSERT_TRUE(fields->LookupByKey("optional_int16")->optional());
    ASSERT_TRUE(fields->LookupByKey("optional_int32")->optional());
    ASSERT_TRUE(fields->LookupByKey("optional_int64")->optional());
    ASSERT_TRUE(fields->LookupByKey("optional_float")->optional());
    ASSERT_TRUE(fields->LookupByKey("optional_double")->optional());
    ASSERT_TRUE(fields->LookupByKey("optional_bool")->optional());
}
