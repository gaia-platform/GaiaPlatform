/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <fstream>
#include <iostream>
#include <limits>

#include <gtest/gtest.h>

#include "gaia_internal/common/file.hpp"

#include "field_access.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db::payload_types;

constexpr char c_updated_bin_filename[] = "updated_test_record_data.bin";
constexpr char c_bin_filename[] = "test_record_data.bin";
constexpr char c_bfbs_filename[] = "test_record.bfbs";

// The following values must match the values from test_record_data.json.
constexpr char c_first_name[] = "Takeshi";

constexpr char c_last_name[] = "Kovac";
constexpr char c_new_last_name[] = "Kovacs";

constexpr uint8_t c_age = 242;
constexpr uint8_t c_new_age = 246;

constexpr int8_t c_has_children = 0;
constexpr int8_t c_new_has_children = 1;

constexpr int64_t c_identifier = 7364592217;
constexpr int64_t c_new_identifier = 9287332599;

constexpr size_t c_count_known_associates = 4;
constexpr int64_t c_known_associates[] = {8583390572, 8438230053, 2334850034, 5773382939};
constexpr size_t c_index_new_known_associate = 1;
constexpr int64_t c_new_known_associate = 7234958243;
constexpr size_t c_new_count_known_associates = 6;

constexpr size_t c_count_known_aliases = 4;
constexpr const char* c_known_aliases[] = {"Mamba Lev", "One Hand Rending", "The Ice", "Ken Kakura"};
constexpr size_t c_index_new_known_alias = 2;
constexpr char c_new_known_alias[] = "The Icepick";
constexpr size_t c_new_count_known_aliases = 7;

constexpr double c_sleeve_cost = 769999.19;
constexpr double c_new_sleeve_cost = 1299999.69;

constexpr float c_monthly_sleeve_insurance = 149.29;
constexpr float c_new_monthly_sleeve_insurance = 259.79;

constexpr size_t c_count_credit_amounts = 3;
constexpr double c_last_yearly_top_credit_amounts[] = {190000000.39, 29900000.49, 0};
constexpr size_t c_index_new_credit_amount = 1;
constexpr double c_new_credit_amount = 39900000.89;
constexpr size_t c_new_count_credit_amounts = 2;

constexpr float c_default_value_for_float_field = 7.7;

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
    float_field_with_default_value,
    missing_int_field,
    missing_string_field,
    missing_int_array_field,

    // Keep this entry last and add any new entry above.
    count_fields
};

void get_fields_data(
    file_loader_t& data_loader,
    file_loader_t& schema_loader,
    bool check_new_values)
{
    data_holder_t first_name = get_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::first_name);
    cout << "\tfirst_name = " << first_name.hold.string_value << endl;
    ASSERT_EQ(first_name.type, reflection::String);
    ASSERT_EQ(0, strcmp(first_name.hold.string_value, c_first_name));

    data_holder_t last_name = get_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::last_name);
    cout << "\tlast_name = " << last_name.hold.string_value << endl;
    ASSERT_EQ(last_name.type, reflection::String);
    ASSERT_EQ(0, strcmp(last_name.hold.string_value, check_new_values ? c_new_last_name : c_last_name));

    data_holder_t age = get_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::age);
    cout << "\tage = " << age.hold.integer_value << endl;
    ASSERT_EQ(age.type, reflection::UByte);
    ASSERT_EQ(check_new_values ? c_new_age : c_age, age.hold.integer_value);

    data_holder_t has_children = get_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::has_children);
    cout << "\thas_children = " << has_children.hold.integer_value << endl;
    ASSERT_EQ(has_children.type, reflection::Bool);
    ASSERT_EQ(check_new_values ? c_new_has_children : c_has_children, has_children.hold.integer_value);

    data_holder_t identifier = get_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::identifier);
    cout << "\tidentifier = " << identifier.hold.integer_value << endl;
    ASSERT_EQ(identifier.type, reflection::Long);
    ASSERT_EQ(check_new_values ? c_new_identifier : c_identifier, identifier.hold.integer_value);

    size_t count_known_associates = get_field_array_size(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::known_associates);
    cout << "\tcount_known_associates = " << count_known_associates << endl;
    ASSERT_EQ(
        check_new_values ? c_new_count_known_associates : c_count_known_associates,
        count_known_associates);

    for (size_t i = 0; i < count_known_associates; i++)
    {
        data_holder_t known_associate = get_field_array_element(
            data_loader.get_data(),
            schema_loader.get_data(),
            field::known_associates,
            i);
        cout << "\t\tknown_associate[" << i << "] = " << known_associate.hold.integer_value << endl;
        ASSERT_EQ(known_associate.type, reflection::Long);

        // Skip new entries for which we do not have comparison values.
        if (i >= c_count_known_associates)
        {
            continue;
        }

        if (i == c_index_new_known_associate)
        {
            ASSERT_EQ(
                check_new_values ? c_new_known_associate : c_known_associates[i],
                known_associate.hold.integer_value);
        }
        else
        {
            ASSERT_EQ(c_known_associates[i], known_associate.hold.integer_value);
        }
    }

    size_t count_known_aliases = get_field_array_size(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::known_aliases);
    cout << "\tcount_known_aliases = " << count_known_aliases << endl;
    ASSERT_EQ(
        check_new_values ? c_new_count_known_aliases : c_count_known_aliases,
        count_known_aliases);

    for (size_t i = 0; i < count_known_aliases; i++)
    {
        data_holder_t known_alias = get_field_array_element(
            data_loader.get_data(),
            schema_loader.get_data(),
            field::known_aliases,
            i);
        cout << "\t\tknown_alias[" << i << "] = " << known_alias.hold.string_value << endl;
        ASSERT_EQ(known_alias.type, reflection::String);

        // Skip new entries for which we do not have comparison values.
        if (i >= c_count_known_aliases)
        {
            continue;
        }

        if (i == c_index_new_known_alias)
        {
            ASSERT_EQ(0, strcmp(known_alias.hold.string_value, check_new_values ? c_new_known_alias : c_known_aliases[i]));
        }
        else
        {
            ASSERT_EQ(0, strcmp(known_alias.hold.string_value, c_known_aliases[i]));
        }
    }

    data_holder_t sleeve_cost = get_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::sleeve_cost);
    cout << "\tsleeve_cost = " << sleeve_cost.hold.float_value << endl;
    ASSERT_EQ(sleeve_cost.type, reflection::Double);
    ASSERT_TRUE(sleeve_cost.hold.float_value >= (check_new_values ? c_new_sleeve_cost : c_sleeve_cost));
    ASSERT_TRUE(sleeve_cost.hold.float_value <= (check_new_values ? c_new_sleeve_cost : c_sleeve_cost) + 1);

    data_holder_t monthly_sleeve_insurance = get_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::monthly_sleeve_insurance);
    cout << "\tmonthly_sleeve_insurance = " << monthly_sleeve_insurance.hold.float_value << endl;
    ASSERT_EQ(monthly_sleeve_insurance.type, reflection::Float);
    ASSERT_TRUE(
        monthly_sleeve_insurance.hold.float_value
        >= (check_new_values ? c_new_monthly_sleeve_insurance : c_monthly_sleeve_insurance));
    ASSERT_TRUE(
        monthly_sleeve_insurance.hold.float_value
        <= (check_new_values ? c_new_monthly_sleeve_insurance : c_monthly_sleeve_insurance) + 1);

    size_t count_credit_amounts = get_field_array_size(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::last_yearly_top_credit_amounts);
    cout << "\tcount_credit_amounts = " << count_credit_amounts << endl;
    ASSERT_EQ(
        check_new_values ? c_new_count_credit_amounts : c_count_credit_amounts,
        count_credit_amounts);

    for (size_t i = 0; i < count_credit_amounts; i++)
    {
        data_holder_t credit_amount = get_field_array_element(
            data_loader.get_data(),
            schema_loader.get_data(),
            field::last_yearly_top_credit_amounts,
            i);
        cout << "\t\tcredit_amount[" << i << "] = " << credit_amount.hold.float_value << endl;
        ASSERT_EQ(credit_amount.type, reflection::Double);

        // Skip new entries for which we do not have comparison values.
        if (i >= c_count_credit_amounts)
        {
            continue;
        }

        if (i == c_index_new_credit_amount)
        {
            ASSERT_TRUE(
                credit_amount.hold.float_value
                >= (check_new_values ? c_new_credit_amount : c_last_yearly_top_credit_amounts[i]));
            ASSERT_TRUE(
                credit_amount.hold.float_value
                <= (check_new_values ? c_new_credit_amount : c_last_yearly_top_credit_amounts[i]) + 1);
        }
        else
        {
            ASSERT_TRUE(credit_amount.hold.float_value >= c_last_yearly_top_credit_amounts[i]);
            ASSERT_TRUE(credit_amount.hold.float_value <= c_last_yearly_top_credit_amounts[i] + 1);
        }
    }

    data_holder_t float_field_with_default_value = get_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::float_field_with_default_value);
    ASSERT_EQ(float_field_with_default_value.type, reflection::Float);
    ASSERT_FALSE(float_field_with_default_value.is_null);
    ASSERT_EQ(c_default_value_for_float_field, float_field_with_default_value.hold.float_value);
    cout << "\tfloat_field_with_default_value = " << float_field_with_default_value.hold.float_value << endl;

    data_holder_t missing_int_field = get_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::missing_int_field);
    ASSERT_EQ(missing_int_field.type, reflection::Int);
    ASSERT_TRUE(missing_int_field.is_null);
    cout << "\tmissing_int_field = null" << endl;

    data_holder_t missing_string_field = get_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::missing_string_field);
    ASSERT_EQ(missing_string_field.type, reflection::String);
    ASSERT_TRUE(missing_string_field.is_null);
    cout << "\tmissing_string_field = null" << endl;

    size_t count_missing_int_array_field = get_field_array_size(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::missing_int_array_field);
    ASSERT_EQ(std::numeric_limits<size_t>::max(), count_missing_int_array_field);
    cout << "\tcount_missing_int_array_field = " << count_missing_int_array_field << endl;
    cout << "\tmissing_int_array_field = null" << endl;

    // A few quick equality checks.
    ASSERT_TRUE(are_field_values_equal(
        data_loader.get_data(),
        data_loader.get_data(),
        schema_loader.get_data(),
        field::last_name));

    ASSERT_TRUE(are_field_values_equal(
        data_loader.get_data(),
        data_loader.get_data(),
        schema_loader.get_data(),
        field::age));

    ASSERT_TRUE(are_field_values_equal(
        data_loader.get_data(),
        data_loader.get_data(),
        schema_loader.get_data(),
        field::known_aliases));
}

void read_flatbuffers_data()
{
    // Load binary flatbuffers schema.
    file_loader_t schema_loader;
    schema_loader.load_file_data(c_bfbs_filename);

    // Load flatbuffers serialization.
    file_loader_t data_loader;
    data_loader.load_file_data(c_bin_filename);

    // Validate data.
    ASSERT_EQ(true, verify_data_schema(data_loader.get_data(), data_loader.get_data_length(), schema_loader.get_data()));

    // Read data.
    bool check_new_values = false;
    get_fields_data(
        data_loader,
        schema_loader,
        check_new_values);

    cout << endl;
}

TEST(db__payload_types__field_access__test, field_read)
{
    read_flatbuffers_data();
}

void update_flatbuffers_data()
{
    // Load binary flatbuffers schema.
    file_loader_t schema_loader;
    schema_loader.load_file_data(c_bfbs_filename);

    // Load flatbuffers serialization.
    file_loader_t data_loader;
    data_loader.load_file_data(c_bin_filename);

    // Validate data.
    ASSERT_EQ(true, verify_data_schema(data_loader.get_data(), data_loader.get_data_length(), schema_loader.get_data()));

    // Update the serialized data.
    // We pass nullptr for schema because the cache is initialized already.
    cout << "\nUpdating fields..." << endl;

    cout << "\tupdating age to " << static_cast<int>(c_new_age) << "..." << endl;
    data_holder_t new_age;
    new_age.type = reflection::UByte;
    new_age.hold.integer_value = c_new_age;
    bool set_result = set_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::age,
        new_age);
    ASSERT_EQ(true, set_result);

    cout << "\tupdating has_children to " << static_cast<int>(c_new_has_children) << "..." << endl;
    data_holder_t new_has_children;
    new_has_children.type = reflection::Bool;
    new_has_children.hold.integer_value = c_new_has_children;
    set_result = set_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::has_children,
        new_has_children);
    ASSERT_EQ(true, set_result);

    cout << "\tupdating identifier to " << c_new_identifier << "..." << endl;
    data_holder_t new_identifier;
    new_identifier.type = reflection::Long;
    new_identifier.hold.integer_value = c_new_identifier;
    set_result = set_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::identifier,
        new_identifier);
    ASSERT_EQ(true, set_result);

    cout << "\tupdating known_associate[" << c_index_new_known_associate
         << "] to " << c_new_known_associate << "..." << endl;
    data_holder_t new_known_associate;
    new_known_associate.type = reflection::Long;
    new_known_associate.hold.integer_value = c_new_known_associate;
    set_field_array_element(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::known_associates,
        c_index_new_known_associate,
        new_known_associate);

    cout << "\tupdating sleeve_cost to " << c_new_sleeve_cost << "..." << endl;
    data_holder_t new_sleeve_cost;
    new_sleeve_cost.type = reflection::Double;
    new_sleeve_cost.hold.float_value = c_new_sleeve_cost;
    set_result = set_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::sleeve_cost,
        new_sleeve_cost);
    ASSERT_EQ(true, set_result);

    cout << "\tupdating monthly_sleeve_insurance to " << c_new_monthly_sleeve_insurance << "..." << endl;
    data_holder_t new_monthly_sleeve_insurance;
    new_monthly_sleeve_insurance.type = reflection::Float;
    new_monthly_sleeve_insurance.hold.float_value = c_new_monthly_sleeve_insurance;
    set_result = set_field_value(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::monthly_sleeve_insurance,
        new_monthly_sleeve_insurance);
    ASSERT_EQ(true, set_result);

    cout << "\tupdating credit_amount[" << c_index_new_credit_amount
         << "] to " << c_new_credit_amount << "..." << endl;
    data_holder_t new_credit_amount;
    new_credit_amount.type = reflection::Double;
    new_credit_amount.hold.float_value = c_new_credit_amount;
    set_field_array_element(
        data_loader.get_data(),
        schema_loader.get_data(),
        field::last_yearly_top_credit_amounts,
        c_index_new_credit_amount,
        new_credit_amount);

    vector<uint8_t> serialization;

    cout << "\tupdating last_name to " << c_new_last_name << "..." << endl;
    data_holder_t new_last_name;
    new_last_name.type = reflection::String;
    new_last_name.hold.string_value = c_new_last_name;
    serialization = set_field_value(
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
        serialization.data(),
        serialization.size(),
        schema_loader.get_data(),
        field::known_aliases,
        c_index_new_known_alias,
        new_known_alias);

    cout << "\tresizing known_associates to " << c_new_count_known_associates << "..." << endl;
    serialization = set_field_array_size(
        serialization.data(),
        serialization.size(),
        schema_loader.get_data(),
        field::known_associates,
        c_new_count_known_associates);

    cout << "\tresizing known_aliases to " << c_new_count_known_aliases << "..." << endl;
    serialization = set_field_array_size(
        serialization.data(),
        serialization.size(),
        schema_loader.get_data(),
        field::known_aliases,
        c_new_count_known_aliases);

    cout << "\tresizing last_yearly_top_credit_amounts to " << c_new_count_credit_amounts << "..." << endl;
    serialization = set_field_array_size(
        serialization.data(),
        serialization.size(),
        schema_loader.get_data(),
        field::last_yearly_top_credit_amounts,
        c_new_count_credit_amounts);

    // A couple of quick (in)equality checks.
    ASSERT_FALSE(are_field_values_equal(
        data_loader.get_data(),
        serialization.data(),
        schema_loader.get_data(),
        field::last_name));

    ASSERT_FALSE(are_field_values_equal(
        data_loader.get_data(),
        serialization.data(),
        schema_loader.get_data(),
        field::known_aliases));

    // Write out the final serialization.
    ofstream file;
    file.open(c_updated_bin_filename, ios::binary | ios::out | ios::trunc);
    file.write(reinterpret_cast<char*>(serialization.data()), serialization.size());
    file.close();

    // Read back the modified data using cache information.
    // Schema information is not passed to the get_field_value() calls.
    cout << "\nReading back field values:" << endl;

    // Load flatbuffers serialization.
    data_loader.load_file_data(c_updated_bin_filename);

    // Validate data.
    ASSERT_EQ(true, verify_data_schema(data_loader.get_data(), data_loader.get_data_length(), schema_loader.get_data()));

    bool check_new_values = true;
    get_fields_data(
        data_loader,
        schema_loader,
        check_new_values);

    cout << endl;
}

TEST(db__payload_types__field_access__test, field_update)
{
    update_flatbuffers_data();
}
