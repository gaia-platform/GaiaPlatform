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

// The following values must match the values from test_record_data.json.
const char* c_first_name = "Takeshi";
const char* c_last_name = "Kovacs";
const uint8_t c_age = 242;
const int8_t c_has_children = 0;
const int64_t c_identifier = 7364592217;
const size_t c_count_known_associates = 4;
const int64_t c_known_associates[] = { 8583390572, 8438230053, 2334850034, 5773382939 };
const size_t c_count_known_aliases = 4;
const char* c_known_aliases[] = { "Mamba Lev", "One Hand Rending", "The Icepick", "Ken Kakura" };
const double c_sleeve_cost = 769999.0;
const float c_monthly_sleeve_insurance = 149.0;
const size_t c_count_credit_amounts = 3;
const double c_last_yearly_top_credit_amounts[] = { 199000000, 99000000, 0 };

enum field
{
    first_name,
    last_name,
    age,
    has_children,
    identifier,
    known_associates,
    known_aliases,
    sleeve_cost,
    monthly_sleeve_insurance,
    last_yearly_top_credit_amounts,

    // Keep this entry last and add any new entry above.
    count_fields
};

void get_fields_data(
    file_loader_t& data_loader,
    file_loader_t& schema_loader,
    bool pass_schema = false)
{
    data_holder_t first_name = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::first_name);
    ASSERT_EQ(first_name.type, reflection::String);
    ASSERT_EQ(0, strcmp(first_name.hold.string_value, c_first_name));
    cout << "\tfirst_name = " << first_name.hold.string_value << endl;

    data_holder_t last_name = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::last_name);
    ASSERT_EQ(last_name.type, reflection::String);
    ASSERT_EQ(0, strcmp(last_name.hold.string_value, c_last_name));
    cout << "\tlast_name = " << last_name.hold.string_value << endl;

    data_holder_t age = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::age);
    ASSERT_EQ(age.type, reflection::UByte);
    ASSERT_EQ(c_age, age.hold.integer_value);
    cout << "\tage = " << age.hold.integer_value << endl;

    data_holder_t has_children = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::has_children);
    ASSERT_EQ(has_children.type, reflection::Bool);
    ASSERT_EQ(c_has_children, has_children.hold.integer_value);
    cout << "\thas_children = " << has_children.hold.integer_value << endl;

    data_holder_t identifier = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::identifier);
    ASSERT_EQ(identifier.type, reflection::Long);
    ASSERT_EQ(c_identifier, identifier.hold.integer_value);
    cout << "\testate_value = " << identifier.hold.integer_value << endl;

    size_t count_known_associates = get_table_field_array_size(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::known_associates);
    ASSERT_EQ(c_count_known_associates, count_known_associates);
    cout << "\tcount known_associates = " << count_known_associates << endl;

    for (size_t i = 0; i < count_known_associates; i++)
    {
        data_holder_t known_associate = get_table_field_array_element(
            c_type_id,
            data_loader.get_data(),
            pass_schema ? schema_loader.get_data() : nullptr,
            field::known_associates,
            i);
        ASSERT_EQ(known_associate.type, reflection::Long);
        ASSERT_EQ(c_known_associates[i], known_associate.hold.integer_value);
        cout << "\t\tknown_associate[" << i << "] = " << known_associate.hold.integer_value << endl;
    }

    size_t count_known_aliases = get_table_field_array_size(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::known_aliases);
    ASSERT_EQ(c_count_known_aliases, count_known_aliases);
    cout << "\tcount known_aliases = " << count_known_aliases << endl;

    for (size_t i = 0; i < count_known_aliases; i++)
    {
        data_holder_t known_alias = get_table_field_array_element(
            c_type_id,
            data_loader.get_data(),
            pass_schema ? schema_loader.get_data() : nullptr,
            field::known_aliases,
            i);
        ASSERT_EQ(known_alias.type, reflection::String);
        ASSERT_EQ(0, strcmp(known_alias.hold.string_value, c_known_aliases[i]));
        cout << "\t\tknown_alias[" << i << "] = " << known_alias.hold.string_value << endl;
    }

    data_holder_t sleeve_cost = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::sleeve_cost);
    ASSERT_EQ(sleeve_cost.type, reflection::Double);
    ASSERT_TRUE(sleeve_cost.hold.float_value > c_sleeve_cost);
    ASSERT_TRUE(sleeve_cost.hold.float_value <= c_sleeve_cost + 1);
    cout << "\tmonthly_mortgage = " << sleeve_cost.hold.float_value << endl;

    data_holder_t monthly_sleeve_insurance = get_table_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::monthly_sleeve_insurance);
    ASSERT_EQ(monthly_sleeve_insurance.type, reflection::Float);
    ASSERT_TRUE(monthly_sleeve_insurance.hold.float_value > c_monthly_sleeve_insurance);
    ASSERT_TRUE(monthly_sleeve_insurance.hold.float_value <= c_monthly_sleeve_insurance + 1);
    cout << "\tmonthly_cable_bill = " << monthly_sleeve_insurance.hold.float_value << endl;

    size_t count_credit_amounts = get_table_field_array_size(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::last_yearly_top_credit_amounts);
    ASSERT_EQ(c_count_credit_amounts, count_credit_amounts);
    cout << "\tcount credit amounts = " << count_credit_amounts << endl;

    for (size_t i = 0; i < count_credit_amounts; i++)
    {
        data_holder_t credit_amount = get_table_field_array_element(
            c_type_id,
            data_loader.get_data(),
            pass_schema ? schema_loader.get_data() : nullptr,
            field::last_yearly_top_credit_amounts,
            i);
        ASSERT_EQ(credit_amount.type, reflection::Double);
        ASSERT_EQ(c_last_yearly_top_credit_amounts[i], credit_amount.hold.float_value);
        cout << "\t\tlast_yearly_top_credit_amount[" << i << "] = " << credit_amount.hold.float_value << endl;
    }
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
    ASSERT_EQ(field::count_fields, field_cache->size());

    // Add field cache to type cache.
    type_cache_t::get()->set_field_cache(c_type_id, field_cache);
    ASSERT_EQ(1, type_cache_t::get()->size());

    if (access_fields)
    {
        cout << "\nFirst round of field access:" << endl;

        // Access fields using cache information.
        // Schema information is not passed to the get_table_field_value() calls.
        get_fields_data(
            data_loader,
            schema_loader);
    }

    // Remove field cache from type cache.
    type_cache_t::get()->remove_field_cache(c_type_id);
    ASSERT_EQ(0, type_cache_t::get()->size());

    if (access_fields)
    {
        cout << "\nSecond round of field access:" << endl;

        // Pass schema information to the get_table_field_value() calls,
        // because cache is empty.
        get_fields_data(
            data_loader,
            schema_loader,
            true);
    }

    cout << endl;

    // Cache should continue to be empty.
    ASSERT_EQ(0, type_cache_t::get()->size());
}

TEST(types, type_cache)
{
    process_flatbuffers_data();
}

TEST(types, field_access)
{
    process_flatbuffers_data(true);
}
