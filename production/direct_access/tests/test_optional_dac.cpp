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

    optional_values_waynetype values = optional_values_waynetype::get(values_id);

    ASSERT_FALSE((values.*accessor)().has_value());
    // Since this test is run with C++17, gaia::common::optional_t maps to std::optional
    // hence the std::bad_optional_access exception.
    ASSERT_THROW((values.*accessor)().value(), std::bad_optional_access);

    // Put a raw value in the optional.
    values_w = values.writer();
    (values_w.*writer) = val;
    values_w.update_row();

    ASSERT_TRUE((values.*accessor)().has_value());
    ASSERT_EQ((values.*accessor)().value(), val);

    // Replace the optional value with another optional.
    optional_t<T_value> opt_value = other_val;
    values_w = values.writer();
    (values_w.*writer) = opt_value;
    values_w.update_row();

    ASSERT_TRUE((values.*accessor)().has_value());
    ASSERT_EQ((values.*accessor)().value(), other_val);

    // Replace the optional value with nullopt.
    values_w = values.writer();
    (values_w.*writer) = nullopt;
    values_w.update_row();

    ASSERT_FALSE((values.*accessor)().has_value());
    ASSERT_THROW((values.*accessor)().value(), std::bad_optional_access);
}

TEST_F(optional_dac_test, test_optionals_values)
{
    test_optional_value<uint8_t>(8, 10, &optional_values_waynetype::optional_uint8, &optional_values_writer::optional_uint8);
    test_optional_value<uint16_t>(8, 10, &optional_values_waynetype::optional_uint16, &optional_values_writer::optional_uint16);
    test_optional_value<uint32_t>(8, 10, &optional_values_waynetype::optional_uint32, &optional_values_writer::optional_uint32);
    test_optional_value<uint64_t>(8, 10, &optional_values_waynetype::optional_uint64, &optional_values_writer::optional_uint64);
    test_optional_value<int8_t>(8, 10, &optional_values_waynetype::optional_int8, &optional_values_writer::optional_int8);
    test_optional_value<int16_t>(8, 10, &optional_values_waynetype::optional_int16, &optional_values_writer::optional_int16);
    test_optional_value<int32_t>(8, 10, &optional_values_waynetype::optional_int32, &optional_values_writer::optional_int32);
    test_optional_value<int64_t>(8, 10, &optional_values_waynetype::optional_int64, &optional_values_writer::optional_int64);
    test_optional_value<float>(8.1f, 10.1f, &optional_values_waynetype::optional_float, &optional_values_writer::optional_float);
    test_optional_value<double>(9.2, 11.2, &optional_values_waynetype::optional_double, &optional_values_writer::optional_double);
    test_optional_value<bool>(true, false, &optional_values_waynetype::optional_bool, &optional_values_writer::optional_bool);
}

TEST_F(optional_dac_test, test_non_optionals_values)
{
    auto_transaction_t txn;

    optional_values_writer values_w;
    gaia_id_t values_id = values_w.insert_row();
    optional_values_waynetype values = optional_values_waynetype::get(values_id);

    // Default values for non-optional fields.
    ASSERT_EQ(values.non_optional_int(), 0);
    ASSERT_EQ(values.non_optional_fp(), 0.0);
    ASSERT_EQ(values.non_optional_int(), false);

    // Assign value to non-optional.
    values_w = values.writer();
    values_w.non_optional_int = 10;
    values_w.non_optional_fp = 10.2;
    values_w.non_optional_bool = true;
    values_w.update_row();

    ASSERT_EQ(values.non_optional_int(), 10);
    ASSERT_EQ(values.non_optional_fp(), 10.2);
    ASSERT_EQ(values.non_optional_bool(), true);
}

TEST_F(optional_dac_test, test_insert)
{
    auto_transaction_t txn;

    // The values are automatically "boxed into an optional".
    gaia_id_t values_id_1 = optional_insert_override_waynetype::insert_row(23, 2.3, true);
    optional_insert_override_waynetype values_1 = optional_insert_override_waynetype::get(values_id_1);
    ASSERT_EQ(values_1.optional_uint8(), 23);
    ASSERT_EQ(values_1.optional_double(), 2.3);
    ASSERT_EQ(values_1.non_optional_bool(), true);

    // The values are passed as optional.
    gaia_id_t values_id_2 = optional_insert_override_waynetype::insert_row(optional_t<uint8_t>(23), optional_t<double>(2.3), true);
    optional_insert_override_waynetype values_2 = optional_insert_override_waynetype::get(values_id_2);
    ASSERT_EQ(values_2.optional_uint8(), 23);
    ASSERT_EQ(values_2.optional_double(), 2.3);
    ASSERT_EQ(values_2.non_optional_bool(), true);

    // Pass nullopt to leave values uninitialized.
    gaia_id_t values_id_3 = optional_insert_override_waynetype::insert_row(nullopt, nullopt, true);
    optional_insert_override_waynetype values_3 = optional_insert_override_waynetype::get(values_id_3);
    ASSERT_FALSE(values_3.optional_uint8().has_value());
    ASSERT_FALSE(values_3.optional_double().has_value());
    ASSERT_EQ(values_3.non_optional_bool(), true);
}

TEST_F(optional_dac_test, test_vlr)
{
    auto_transaction_t txn;

    // Empty values do not create connection.
    optional_vlr_parent_waynetype parent = optional_vlr_parent_waynetype::get(optional_vlr_parent_writer().insert_row());
    optional_vlr_child_waynetype child = optional_vlr_child_waynetype::get(optional_vlr_child_writer().insert_row());

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
    child_w.parent_id = nullopt;
    child_w.update_row();

    ASSERT_EQ(parent.child().size(), 0);

    // Reconnect and parent implicit disconnect.
    child_w = child.writer();
    child_w.parent_id = c_parent_id;
    child_w.update_row();

    ASSERT_EQ(parent.child().size(), 1);

    parent_w = parent.writer();
    parent_w.id = nullopt;
    parent_w.update_row();

    ASSERT_EQ(parent.child().size(), 0);
}
