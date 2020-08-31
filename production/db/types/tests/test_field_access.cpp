/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <fstream>

#include "gtest/gtest.h"

#include "field_access.hpp"
#include "file.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db::types;

const uint64_t c_type_id = 88;

// The following values must match the values from test_record_data.json.
const char* c_first_name = "Takeshi";
const char* c_last_name = "Kovac";
const char* c_new_last_name = "Kovacs";
const uint8_t c_age = 242;
const uint8_t c_new_age = 246;
const int8_t c_has_children = 0;
const int8_t c_new_has_children = 1;
const int64_t c_identifier = 7364592217;
const int64_t c_new_identifier = 9287332599;
const size_t c_count_known_associates = 4;
const int64_t c_known_associates[] = { 8583390572, 8438230053, 2334850034, 5773382939 };
const size_t c_index_new_known_associate = 1;
const int64_t c_new_known_associate = 7234958243;
const size_t c_count_known_aliases = 4;
const char* c_known_aliases[] = { "Mamba Lev", "One Hand Rending", "The Ice", "Ken Kakura" };
const size_t c_index_new_known_alias = 2;
const char* c_new_known_alias = "The Icepick";
const double c_sleeve_cost = 769999.19;
const double c_new_sleeve_cost = 1299999.69;
const float c_monthly_sleeve_insurance = 149.29;
const float c_new_monthly_sleeve_insurance = 259.79;
const size_t c_count_credit_amounts = 3;
const double c_last_yearly_top_credit_amounts[] = { 190000000.39, 29900000.49, 0 };
const size_t c_index_new_credit_amount = 1;
const double c_new_credit_amount = 39900000.89;

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
    bool pass_schema = false,
    bool check_new_values = false)
{
    data_holder_t first_name = get_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::first_name);
    cout << "\tfirst_name = " << first_name.hold.string_value << endl;
    ASSERT_EQ(first_name.type, reflection::String);
    ASSERT_EQ(0, strcmp(first_name.hold.string_value, c_first_name));

    data_holder_t last_name = get_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::last_name);
    cout << "\tlast_name = " << last_name.hold.string_value << endl;
    ASSERT_EQ(last_name.type, reflection::String);
    ASSERT_EQ(0, strcmp(last_name.hold.string_value, check_new_values ? c_new_last_name : c_last_name));

    data_holder_t age = get_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::age);
    cout << "\tage = " << age.hold.integer_value << endl;
    ASSERT_EQ(age.type, reflection::UByte);
    ASSERT_EQ(check_new_values ? c_new_age : c_age, age.hold.integer_value);

    data_holder_t has_children = get_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::has_children);
    cout << "\thas_children = " << has_children.hold.integer_value << endl;
    ASSERT_EQ(has_children.type, reflection::Bool);
    ASSERT_EQ(check_new_values ? c_new_has_children : c_has_children, has_children.hold.integer_value);

    data_holder_t identifier = get_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::identifier);
    cout << "\tidentifier = " << identifier.hold.integer_value << endl;
    ASSERT_EQ(identifier.type, reflection::Long);
    ASSERT_EQ(check_new_values ? c_new_identifier : c_identifier, identifier.hold.integer_value);

    size_t count_known_associates = get_field_array_size(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::known_associates);
    cout << "\tcount_known_associates = " << count_known_associates << endl;
    ASSERT_EQ(c_count_known_associates, count_known_associates);

    for (size_t i = 0; i < count_known_associates; i++)
    {
        data_holder_t known_associate = get_field_array_element(
            c_type_id,
            data_loader.get_data(),
            pass_schema ? schema_loader.get_data() : nullptr,
            field::known_associates,
            i);
        cout << "\t\tknown_associate[" << i << "] = " << known_associate.hold.integer_value << endl;
        ASSERT_EQ(known_associate.type, reflection::Long);
        if (i == c_index_new_known_associate)
        {
            ASSERT_EQ(check_new_values ? c_new_known_associate : c_known_associates[i],
                known_associate.hold.integer_value);
        }
        else
        {
            ASSERT_EQ(c_known_associates[i], known_associate.hold.integer_value);
        }
    }

    size_t count_known_aliases = get_field_array_size(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::known_aliases);
    cout << "\tcount_known_aliases = " << count_known_aliases << endl;
    ASSERT_EQ(c_count_known_aliases, count_known_aliases);

    for (size_t i = 0; i < count_known_aliases; i++)
    {
        data_holder_t known_alias = get_field_array_element(
            c_type_id,
            data_loader.get_data(),
            pass_schema ? schema_loader.get_data() : nullptr,
            field::known_aliases,
            i);
        cout << "\t\tknown_alias[" << i << "] = " << known_alias.hold.string_value << endl;
        ASSERT_EQ(known_alias.type, reflection::String);
        if (i == c_index_new_known_alias)
        {
            ASSERT_EQ(0, strcmp(
                known_alias.hold.string_value,
                check_new_values ? c_new_known_alias : c_known_aliases[i]));
        }
        else
        {
            ASSERT_EQ(0, strcmp(known_alias.hold.string_value, c_known_aliases[i]));
        }
    }

    data_holder_t sleeve_cost = get_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::sleeve_cost);
    cout << "\tsleeve_cost = " << sleeve_cost.hold.float_value << endl;
    ASSERT_EQ(sleeve_cost.type, reflection::Double);
    ASSERT_TRUE(sleeve_cost.hold.float_value >= (check_new_values ? c_new_sleeve_cost : c_sleeve_cost));
    ASSERT_TRUE(sleeve_cost.hold.float_value <= (check_new_values ? c_new_sleeve_cost : c_sleeve_cost) + 1);

    data_holder_t monthly_sleeve_insurance = get_field_value(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::monthly_sleeve_insurance);
    cout << "\tmonthly_sleeve_insurance = " << monthly_sleeve_insurance.hold.float_value << endl;
    ASSERT_EQ(monthly_sleeve_insurance.type, reflection::Float);
    ASSERT_TRUE(monthly_sleeve_insurance.hold.float_value
        >= (check_new_values ? c_new_monthly_sleeve_insurance : c_monthly_sleeve_insurance));
    ASSERT_TRUE(monthly_sleeve_insurance.hold.float_value
        <= (check_new_values ? c_new_monthly_sleeve_insurance : c_monthly_sleeve_insurance) + 1);

    size_t count_credit_amounts = get_field_array_size(
        c_type_id,
        data_loader.get_data(),
        pass_schema ? schema_loader.get_data() : nullptr,
        field::last_yearly_top_credit_amounts);
    cout << "\tcount_credit_amounts = " << count_credit_amounts << endl;
    ASSERT_EQ(c_count_credit_amounts, count_credit_amounts);

    for (size_t i = 0; i < count_credit_amounts; i++)
    {
        data_holder_t credit_amount = get_field_array_element(
            c_type_id,
            data_loader.get_data(),
            pass_schema ? schema_loader.get_data() : nullptr,
            field::last_yearly_top_credit_amounts,
            i);
        cout << "\t\tcredit_amount[" << i << "] = " << credit_amount.hold.float_value << endl;
        ASSERT_EQ(credit_amount.type, reflection::Double);
        if (i == c_index_new_credit_amount)
        {
            ASSERT_TRUE(credit_amount.hold.float_value
                >= (check_new_values ? c_new_credit_amount : c_last_yearly_top_credit_amounts[i]));
            ASSERT_TRUE(credit_amount.hold.float_value
                <= (check_new_values ? c_new_credit_amount : c_last_yearly_top_credit_amounts[i]) + 1);
        }
        else
        {
            ASSERT_TRUE(credit_amount.hold.float_value >= c_last_yearly_top_credit_amounts[i]);
            ASSERT_TRUE(credit_amount.hold.float_value <= c_last_yearly_top_credit_amounts[i] + 1);
        }
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
        // Schema information is not passed to the get_field_value() calls.
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

        // Pass schema information to the get_field_value() calls,
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

void update_flatbuffers_data()
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

    // Update the serialized data.
    // We pass nullptr for schema because the cache is initialized already.
    cout << "\nUpdating fields..." << endl;

    cout << "\tupdating age to " << (int)c_new_age << "..." << endl;
    data_holder_t new_age;
    new_age.type = reflection::UByte;
    new_age.hold.integer_value = c_new_age;
    bool set_result = set_field_value(
        c_type_id,
        data_loader.get_data(),
        nullptr,
        field::age,
        new_age);
    ASSERT_EQ(true, set_result);

    cout << "\tupdating has_children to " << (int)c_new_has_children << "..." << endl;
    data_holder_t new_has_children;
    new_has_children.type = reflection::Bool;
    new_has_children.hold.integer_value = c_new_has_children;
    set_result = set_field_value(
        c_type_id,
        data_loader.get_data(),
        nullptr,
        field::has_children,
        new_has_children);
    ASSERT_EQ(true, set_result);

    cout << "\tupdating identifier to " << c_new_identifier << "..." << endl;
    data_holder_t new_identifier;
    new_identifier.type = reflection::Long;
    new_identifier.hold.integer_value = c_new_identifier;
    set_result = set_field_value(
        c_type_id,
        data_loader.get_data(),
        nullptr,
        field::identifier,
        new_identifier);
    ASSERT_EQ(true, set_result);

    cout << "\tupdating known_associate[" << c_index_new_known_associate
        << "] to " << c_new_known_associate << "..." << endl;
    data_holder_t new_known_associate;
    new_known_associate.type = reflection::Long;
    new_known_associate.hold.integer_value = c_new_known_associate;
    set_field_array_element(
        c_type_id,
        data_loader.get_data(),
        nullptr,
        field::known_associates,
        c_index_new_known_associate,
        new_known_associate);

    cout << "\tupdating sleeve_cost to " << c_new_sleeve_cost << "..." << endl;
    data_holder_t new_sleeve_cost;
    new_sleeve_cost.type = reflection::Double;
    new_sleeve_cost.hold.float_value = c_new_sleeve_cost;
    set_result = set_field_value(
        c_type_id,
        data_loader.get_data(),
        nullptr,
        field::sleeve_cost,
        new_sleeve_cost);
    ASSERT_EQ(true, set_result);

    cout << "\tupdating monthly_sleeve_insurance to " << c_new_monthly_sleeve_insurance << "..." << endl;
    data_holder_t new_monthly_sleeve_insurance;
    new_monthly_sleeve_insurance.type = reflection::Float;
    new_monthly_sleeve_insurance.hold.float_value = c_new_monthly_sleeve_insurance;
    set_result = set_field_value(
        c_type_id,
        data_loader.get_data(),
        nullptr,
        field::monthly_sleeve_insurance,
        new_monthly_sleeve_insurance);
    ASSERT_EQ(true, set_result);

    cout << "\tupdating credit_amount[" << c_index_new_credit_amount
        << "] to " << c_new_credit_amount << "..." << endl;
    data_holder_t new_credit_amount;
    new_credit_amount.type = reflection::Double;
    new_credit_amount.hold.float_value = c_new_credit_amount;
    set_field_array_element(
        c_type_id,
        data_loader.get_data(),
        nullptr,
        field::last_yearly_top_credit_amounts,
        c_index_new_credit_amount,
        new_credit_amount);

    vector<uint8_t> serialization;

    cout << "\tupdating last_name to " << c_new_last_name << "..." << endl;
    data_holder_t new_last_name;
    new_last_name.type = reflection::String;
    new_last_name.hold.string_value = c_new_last_name;
    serialization = set_field_value(
        c_type_id,
        data_loader.get_data(),
        data_loader.get_data_length(),
        schema_loader.get_data(),
        field::last_name,
        new_last_name);

    cout << "\tupdating known_alias[" << c_index_new_known_alias
        << "] to " << c_new_known_alias << "..." << endl;
    data_holder_t new_known_alias;
    new_known_alias.type = reflection::String;
    new_known_alias.hold.string_value = c_new_known_alias;
    serialization = set_field_array_element(
        c_type_id,
        serialization.data(),
        serialization.size(),
        schema_loader.get_data(),
        field::known_aliases,
        c_index_new_known_alias,
        new_known_alias);

    // Write out the final serialization.
    ofstream file;
    file.open("updated_test_record_data.bin", ios::binary | ios::out | ios::trunc);
    file.write(reinterpret_cast<char*>(serialization.data()), serialization.size());
    file.close();

    // Read back the modified data using cache information.
    // Schema information is not passed to the get_field_value() calls.
    cout << "\nReading back field values:" << endl;

    // Load flatbuffers serialization.
    data_loader.load_file_data("updated_test_record_data.bin");

    get_fields_data(
        data_loader,
        schema_loader,
        false,
        true);

    cout << endl;

    // Cache should continue to be populated.
    ASSERT_EQ(1, type_cache_t::get()->size());
}

TEST(types, field_update)
{
    update_flatbuffers_data();
}
