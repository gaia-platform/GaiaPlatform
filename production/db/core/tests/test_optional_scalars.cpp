/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog_tests_helper.hpp"
#include "gaia_internal/db/db_test_base.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"
#include "gaia_internal/db/gaia_ptr_api.hpp"

#include "field_access.hpp"

using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::catalog;
using namespace gaia::direct_access;
using namespace gaia::db::payload_types;

class test_optional_scalars : public db_test_base_t
{
};

TEST_F(test_optional_scalars, suppini)
{
    gaia_id_t internal_type_id
        = table_builder_t::new_table("test_optional")
              .field("non_optional_uint8", data_type_t::e_uint8, table_builder_t::c_non_optional)
              .field("optional_uint8", data_type_t::e_uint8, table_builder_t::c_optional)
              //              .field("optional_uint8", data_type_t::e_uint8, table_builder_t::c_optional)
              //              .field("optional_uint16", data_type_t::e_uint16, table_builder_t::c_optional)
              //              .field("optional_uint32", data_type_t::e_uint32, table_builder_t::c_optional)
              //              .field("optional_uint64", data_type_t::e_uint64, table_builder_t::c_optional)
              //              .field("optional_int8", data_type_t::e_int8, table_builder_t::c_optional)
              //              .field("optional_int16", data_type_t::e_int16, table_builder_t::c_optional)
              //              .field("optional_int32", data_type_t::e_int32, table_builder_t::c_optional)
              //              .field("optional_int64", data_type_t::e_int64, table_builder_t::c_optional)
              //              .field("optional_float", data_type_t::e_float, table_builder_t::c_optional)
              //              .field("optional_double", data_type_t::e_double, table_builder_t::c_optional)
              //              .field("optional_bool", data_type_t::e_bool, table_builder_t::c_optional)
              .create();

    auto_transaction_t txn;

    gaia_table_t table = gaia_table_t::get(internal_type_id);
    gaia_type_t gaia_type = table.type();
    dac_vector_t<uint8_t> serialization_template = table.serialization_template();
    gaia_ptr_t object = gaia_ptr::create(gaia_type, serialization_template.size(), serialization_template.data());

    for (auto f : table.gaia_fields())
    {
        gaia_log::app().info("{} -> {}", f.name(), f.position());
    }

    txn.commit();

    data_holder_t before_value = get_field_value(gaia_type, reinterpret_cast<const uint8_t*>(object.data()), table.binary_schema().data(), table.binary_schema().size(), 1);
    //        ASSERT_TRUE(before_value.is_null);
    gaia_log::app().info("Val {}", before_value.hold.integer_value);

    data_holder_t new_val;
    new_val.type = reflection::UByte;
    new_val.hold.integer_value = uint8_t{3};

    auto* data = new uint8_t[object.data_size()];
    memcpy(data, object.data(), object.data_size());

    set_field_value(gaia_type, data, table.binary_schema().data(), table.binary_schema().size(), 1, new_val);
    object.update_payload(object.data_size(), data);
    data_holder_t after_value = get_field_value(gaia_type, reinterpret_cast<const uint8_t*>(object.data()), table.binary_schema().data(), table.binary_schema().size(), 1);
    ASSERT_EQ(after_value.hold.integer_value, 3);
}
