////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <cstdlib>

#include <thread>

#include <gtest/gtest.h>
#include <json.hpp>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"
#include "gaia_amr_swarm.h"
#include "gaia_db_extract.hpp"

using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::common;
using namespace gaia::addr_book;
using namespace gaia::amr_swarm;
using namespace gaia::tools::db_extract;
using json_t = nlohmann::json;

using std::exception;
using std::string;
using std::thread;

class tools__gaia_db_extract__row_field_values__test : public db_catalog_test_base_t
{
protected:
    tools__gaia_db_extract__row_field_values__test()
        : db_catalog_test_base_t(string("amr_swarm.ddl")){};

    template <typename T_floatsize>
    void check_within_tolerance(T_floatsize json_val, T_floatsize db_val, T_floatsize tolerance)
    {
        EXPECT_TRUE((json_val + tolerance) > db_val && (json_val - tolerance) < db_val);
    }
};

// Numerical analysis not necessary here. These values are acceptable.
constexpr float float_tolerance = 0.001;
constexpr double double_tolerance = 0.00001;

// Test JSON conversion to double, float, string, uint8_t, uint32_t, bool.
// Using amr_swarm DDL because of it's variety of data types.
TEST_F(tools__gaia_db_extract__row_field_values__test, verify_field_values)
{
    // Using configuration_t.
    {
        begin_transaction();

        // These values don't matter. Only the tests for equality.
        constexpr double c_double_1 = 3.5;
        constexpr double c_double_2 = 4.7;
        constexpr double c_double_3 = 2.43;
        constexpr double c_double_4 = 3.29;
        constexpr uint8_t c_uint8_1 = 5;
        constexpr uint8_t c_uint8_2 = 12;
        constexpr uint8_t c_uint8_3 = 255;
        constexpr uint8_t c_uint8_4 = 6;
        configuration_t::insert_row(c_double_1, c_double_2, c_uint8_1, c_uint8_2);
        configuration_t::insert_row(c_double_3, c_double_4, c_uint8_3, c_uint8_4);

        commit_transaction();

        // Produce the JSON string of this selection.
        auto extracted_rows = gaia_db_extract("amr_swarm", "configuration", c_start_at_first, c_row_limit_unlimited, "", c_start_at_first);
        EXPECT_NE(extracted_rows.compare(c_empty_object), 0);

        // Parse all of the rows.
        json_t json_object = json_t::parse(extracted_rows);

        // Verify the first row.
        json_t::iterator row_it = json_object["rows"].begin();
        auto json_row = *row_it;
        double widget_bot_recharge_threshold = json_row["widget_bot_recharge_threshold"].get<double>();
        double pallet_bot_recharge_threshold = json_row["pallet_bot_recharge_threshold"].get<double>();
        uint8_t number_of_pallet_bots = json_row["number_of_pallet_bots"].get<uint8_t>();
        uint8_t number_of_widget_bots = json_row["number_of_widget_bots"].get<uint8_t>();
        check_within_tolerance<double>(widget_bot_recharge_threshold, c_double_1, double_tolerance);
        check_within_tolerance<double>(pallet_bot_recharge_threshold, c_double_2, double_tolerance);
        EXPECT_EQ(number_of_pallet_bots, c_uint8_1);
        EXPECT_EQ(number_of_widget_bots, c_uint8_2);

        // Verify the second row.
        ++row_it;
        json_row = *row_it;
        widget_bot_recharge_threshold = json_row["widget_bot_recharge_threshold"].get<double>();
        pallet_bot_recharge_threshold = json_row["pallet_bot_recharge_threshold"].get<double>();
        number_of_pallet_bots = json_row["number_of_pallet_bots"].get<uint8_t>();
        number_of_widget_bots = json_row["number_of_widget_bots"].get<uint8_t>();

        check_within_tolerance<double>(widget_bot_recharge_threshold, c_double_3, double_tolerance);
        check_within_tolerance<double>(pallet_bot_recharge_threshold, c_double_4, double_tolerance);
        EXPECT_EQ(number_of_pallet_bots, c_uint8_3);
        EXPECT_EQ(number_of_widget_bots, c_uint8_4);
    }

    // Using robot_t.
    {
        begin_transaction();

        // These values don't matter. Only the tests for equality.
        constexpr uint16_t c_uint16_1 = 16000;
        constexpr uint16_t c_uint16_2 = 16001;
        constexpr char c_string_1[] = "my_sandbox_id";
        constexpr float c_float_1 = 6.123;
        constexpr float c_float_2 = 7.9876;
        constexpr uint8_t c_uint8_1 = 100;
        constexpr uint8_t c_uint8_2 = 101;
        constexpr uint16_t c_uint16_3 = 12000;
        constexpr uint16_t c_uint16_4 = 12001;
        robot_t::insert_row(c_uint16_1, c_string_1, c_float_1, false, true, true, c_uint8_1, c_uint16_3);
        robot_t::insert_row(c_uint16_2, c_string_1, c_float_2, true, false, true, c_uint8_2, c_uint16_4);

        commit_transaction();

        // Produce the JSON string of this selection.
        auto extracted_rows = gaia_db_extract("amr_swarm", "robot", c_start_at_first, c_row_limit_unlimited, "", c_start_at_first);
        EXPECT_NE(extracted_rows.compare(c_empty_object), 0);

        // Parse all of the rows.
        json_t json_object = json_t::parse(extracted_rows);

        // Verify the first row.
        json_t::iterator row_it = json_object["rows"].begin();
        auto json_row = *row_it;
        uint16_t id = json_row["id"].get<uint16_t>();
        string sandbox_id = json_row["sandbox_id"].get<string>();
        float charge = json_row["charge"].get<float>();
        bool is_stuck = json_row["is_stuck"].get<uint8_t>();
        bool is_idle = json_row["is_idle"].get<uint8_t>();
        bool is_loaded = json_row["is_loaded"].get<uint8_t>();
        uint8_t robot_type_id = json_row["robot_type_id"].get<uint8_t>();
        uint16_t station_id = json_row["station_id"].get<uint16_t>();

        EXPECT_EQ(id, c_uint16_1);
        EXPECT_STREQ(sandbox_id.c_str(), c_string_1);
        check_within_tolerance<float>(charge, c_float_1, float_tolerance);
        EXPECT_FALSE(is_stuck);
        EXPECT_TRUE(is_idle);
        EXPECT_TRUE(is_loaded);
        EXPECT_EQ(robot_type_id, c_uint8_1);
        EXPECT_EQ(station_id, c_uint16_3);

        // Verify the second row.
        ++row_it;
        json_row = *row_it;
        id = json_row["id"].get<uint16_t>();
        sandbox_id = json_row["sandbox_id"].get<string>();
        charge = json_row["charge"].get<float>();
        is_stuck = json_row["is_stuck"].get<uint8_t>();
        is_idle = json_row["is_idle"].get<uint8_t>();
        is_loaded = json_row["is_loaded"].get<uint8_t>();
        robot_type_id = json_row["robot_type_id"].get<uint8_t>();
        station_id = json_row["station_id"].get<uint16_t>();

        EXPECT_EQ(id, c_uint16_2);
        EXPECT_STREQ(sandbox_id.c_str(), c_string_1);
        check_within_tolerance<float>(charge, c_float_2, 0.005);
        EXPECT_TRUE(is_stuck);
        EXPECT_FALSE(is_idle);
        EXPECT_TRUE(is_loaded);
        EXPECT_EQ(robot_type_id, c_uint8_2);
        EXPECT_EQ(station_id, c_uint16_4);
    }
}
