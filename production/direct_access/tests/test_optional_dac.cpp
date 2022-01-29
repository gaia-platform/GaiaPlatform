/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_optional_sandbox.h"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::optional_sandbox;

class optional_dac_test : public db_catalog_test_base_t
{
protected:
    optional_dac_test()
        : db_catalog_test_base_t(std::string("optional.ddl")){};
};

TEST_F(optional_dac_test, connect_with_id)
{
    auto_transaction_t txn;

    // Empty optional
    optional_values_writer values_w;
    gaia_id_t values_id = values_w.insert_row();

    optional_values_t values = optional_values_t::get(values_id);

    ASSERT_FALSE(values.optional_uint8().has_value());

    // Put a value in the optional
    values_w = values.writer();
    values_w.optional_uint8 = 8;
    values_w.update_row();

    ASSERT_TRUE(values.optional_uint8().has_value());
    ASSERT_EQ(values.optional_uint8().value(), 8);

    std::optional<uint8_t> opt_value = 10;
    values_w = values.writer();
    values_w.optional_uint8 = opt_value;
    values_w.update_row();

    ASSERT_TRUE(values.optional_uint8().has_value());
    ASSERT_EQ(values.optional_uint8().value(), 10);
}
