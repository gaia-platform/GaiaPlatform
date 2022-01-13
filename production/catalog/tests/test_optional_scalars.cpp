/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/db/db_test_base.hpp"

#include "catalog_tests_helper.hpp"

using namespace gaia::db;

class test_optional_scalars : public db_test_base_t
{
};

TEST_F(test_optional_scalars, suppini)
{
    table_builder_t::new_table("test_optional")
        .field("optional_uint8", data_type_t::e_uint8, true)
        .field("optional_uint16", data_type_t::e_uint16, true)
        .field("optional_uint32", data_type_t::e_uint32, true)
        .field("optional_uint64", data_type_t::e_uint64, true)
        .field("optional_int8", data_type_t::e_int8, true)
        .field("optional_int16", data_type_t::e_int16, true)
        .field("optional_int32", data_type_t::e_int32, true)
        .field("optional_int64", data_type_t::e_int64, true)
        .field("optional_float", data_type_t::e_float, true)
        .field("optional_double", data_type_t::e_double, true)
        .field("optional_bool", data_type_t::e_bool, true)
        .create();
}
