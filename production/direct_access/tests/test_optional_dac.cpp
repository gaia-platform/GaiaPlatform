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

// T_accessor and T_writer are the accessor for the getter method and the writer field.
// Given a DAC object doing '(dac_object.*accessor)()' is the same as 'dac_object.field()'
// Given a DAC writer object doing '(dac_w.*writer)' is the same as 'dac_w.field'.
template <typename T_value, typename T_accessor, typename T_writer>
void test_optional_value(T_value val, T_value other_val, T_accessor accessor, T_writer writer)
{
    auto_transaction_t txn;

    // Empty optional.
    optional_values_writer values_w;
    gaia_id_t values_id = values_w.insert_row();

    optional_values_t values = optional_values_t::get(values_id);

    ASSERT_FALSE((values.*accessor)().has_value());

    // Put a raw value in the optional.
    values_w = values.writer();
    (values_w.*writer) = val;
    values_w.update_row();

    ASSERT_TRUE((values.*accessor)().has_value());
    ASSERT_EQ((values.*accessor)().value(), val);

    // Replace the optional value with another optional.
    std::optional<T_value> opt_value = other_val;
    values_w = values.writer();
    (values_w.*writer) = opt_value;
    values_w.update_row();

    ASSERT_TRUE((values.*accessor)().has_value());
    ASSERT_EQ((values.*accessor)().value(), other_val);
}

TEST_F(optional_dac_test, test_optionals_values)
{
    test_optional_value<uint8_t>(8, 10, &optional_values_t::optional_uint8, &optional_values_writer::optional_uint8);
    test_optional_value<uint16_t>(8, 10, &optional_values_t::optional_uint16, &optional_values_writer::optional_uint16);
    test_optional_value<uint32_t>(8, 10, &optional_values_t::optional_uint32, &optional_values_writer::optional_uint32);
    test_optional_value<uint64_t>(8, 10, &optional_values_t::optional_uint64, &optional_values_writer::optional_uint64);
    test_optional_value<int8_t>(8, 10, &optional_values_t::optional_int8, &optional_values_writer::optional_int8);
    test_optional_value<int16_t>(8, 10, &optional_values_t::optional_int16, &optional_values_writer::optional_int16);
    test_optional_value<int32_t>(8, 10, &optional_values_t::optional_int32, &optional_values_writer::optional_int32);
    test_optional_value<int64_t>(8, 10, &optional_values_t::optional_int64, &optional_values_writer::optional_int64);
    test_optional_value<float>(8.1f, 10.1f, &optional_values_t::optional_float, &optional_values_writer::optional_float);
    test_optional_value<double>(9.2, 11.2, &optional_values_t::optional_double, &optional_values_writer::optional_double);
    test_optional_value<bool>(true, false, &optional_values_t::optional_bool, &optional_values_writer::optional_bool);
}

TEST_F(optional_dac_test, test_vlr)
{
    auto_transaction_t txn;

    // Empty values do not create connection.
    optional_vlr_parent_t parent = optional_vlr_parent_t::get(optional_vlr_parent_writer().insert_row());
    optional_vlr_child_t child = optional_vlr_child_t::get(optional_vlr_child_writer().insert_row());

    ASSERT_EQ(parent.child().size(), 0);

    // Implicit connect.
    const gaia_id_t c_parent_id = 1;
    optional_vlr_parent_writer parent_w = parent.writer();
    parent_w.id = c_parent_id;
    parent_w.update_row();

    optional_vlr_child_writer child_w = child.writer();
    child_w.parent_id = c_parent_id;
    child_w.update_row();

    ASSERT_EQ(parent.child().size(), 1);

    // Child implicit disconnect.
    child_w = child.writer();
    child_w.parent_id = std::nullopt;
    child_w.update_row();

    ASSERT_EQ(parent.child().size(), 0);

    // Reconnect and parent implicit disconnect.
    child_w = child.writer();
    child_w.parent_id = c_parent_id;
    child_w.update_row();

    ASSERT_EQ(parent.child().size(), 1);

    parent_w = parent.writer();
    parent_w.id = std::nullopt;
    parent_w.update_row();

    ASSERT_EQ(parent.child().size(), 0);
}
