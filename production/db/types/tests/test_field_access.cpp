/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "field_access.hpp"
#include "file.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db::types;

const uint64_t c_type_id = 88;

enum field
{
    first_name,
    last_name,
    age,
    has_children,
    estate_value,
    monthly_mortgage,
    monthly_cable_bill,
};

void get_fields_data(
    file_loader_t& data_loader,
    file_loader_t& schema_loader,
    bool pass_schema = false)
{
    type_holder_t first_name = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::first_name);
    ASSERT_EQ(first_name.type, reflection::String);
    ASSERT_EQ(0, strcmp(first_name.string_value, "Takeshi"));
    cout << "\tfirst_name = " << first_name.string_value << endl;

    type_holder_t last_name = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::last_name);
    ASSERT_EQ(last_name.type, reflection::String);
    ASSERT_EQ(0, strcmp(last_name.string_value, "Kovacs"));
    cout << "\tlast_name = " << last_name.string_value << endl;

    type_holder_t age = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::age);
    ASSERT_EQ(age.type, reflection::UByte);
    ASSERT_EQ(42, age.integer_value);
    cout << "\tage = " << age.integer_value << endl;

    type_holder_t has_children = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::has_children);
    ASSERT_EQ(has_children.type, reflection::Bool);
    ASSERT_EQ(0, has_children.integer_value);
    cout << "\thas_children = " << has_children.integer_value << endl;

    type_holder_t estate_value = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::estate_value);
    ASSERT_EQ(estate_value.type, reflection::Long);
    ASSERT_EQ(736459, estate_value.integer_value);
    cout << "\testate_value = " << estate_value.integer_value << endl;

    type_holder_t monthly_mortgage = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::monthly_mortgage);
    ASSERT_EQ(monthly_mortgage.type, reflection::Double);
    ASSERT_TRUE(monthly_mortgage.float_value > 269999.0);
    ASSERT_TRUE(monthly_mortgage.float_value <= 270000.0);
    cout << "\tmonthly_mortgage = " << monthly_mortgage.float_value << endl;

    type_holder_t monthly_cable_bill = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::monthly_cable_bill);
    ASSERT_EQ(monthly_cable_bill.type, reflection::Float);
    ASSERT_TRUE(monthly_cable_bill.float_value > 49.0);
    ASSERT_TRUE(monthly_cable_bill.float_value <= 50.0);
    cout << "\tmonthly_cable_bill = " << monthly_cable_bill.float_value << endl;
}

void process_flatbuffers_data(bool access_fields = false)
{
    // Load binary flatbuffers schema.
    file_loader_t schema_loader;
    schema_loader.load_file_data("test_record.bfbs");

    // Load flatbuffers serialization.
    file_loader_t data_loader;
    data_loader.load_file_data("test_record_data.bin");

    // Create and initialize a field_cache.
    field_cache_t* field_cache = new field_cache_t();
    initialize_field_cache_from_binary_schema(field_cache, schema_loader.get_data());
    ASSERT_EQ(7, field_cache->size());

    // Add field cache to type cache.
    type_cache_t::get_type_cache()->set_field_cache(c_type_id, field_cache);
    ASSERT_EQ(1, type_cache_t::get_type_cache()->size());

    if (access_fields)
    {
        cout << "\nFirst round of field access:" << endl;
        // Access fields using cache information.
        // Schema information is not passed to the calls.
        get_fields_data(
            data_loader,
            schema_loader);
    }

    // Remove field cache from type cache.
    type_cache_t::get_type_cache()->remove_field_cache(c_type_id);
    ASSERT_EQ(0, type_cache_t::get_type_cache()->size());

    if (access_fields)
    {
        cout << "\nSecond round of field access:" << endl;
        // Pass schema information to the call,
        // because cache is empty.
        get_fields_data(
            data_loader,
            schema_loader,
            true);
    }

    cout << endl;

    // Cache should continue to be empty.
    ASSERT_EQ(0, type_cache_t::get_type_cache()->size());
}

TEST(types, type_cache)
{
    process_flatbuffers_data();
}

TEST(types, field_access)
{
    process_flatbuffers_data(true);
}
