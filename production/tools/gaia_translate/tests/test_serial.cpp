/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <unistd.h>

#include "gtest/gtest.h"

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/rules/rules_test_helpers.hpp"

#include "gaia_barn_storage.h"
#include "test_rulesets.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;

extern std::atomic<int32_t> g_rule_called;
extern std::atomic<int32_t> g_insert_called;
extern std::atomic<int32_t> g_update_value_called;
extern std::atomic<int32_t> g_update_timestamp_called;
extern std::atomic<int32_t> g_update_min_temp_called;
extern std::atomic<int32_t> g_update_max_temp_called;

const float c_g_incubator_min_temperature = 1;
const float c_g_incubator_max_temperature = 10;
const float c_g_sensor_value = 21;
const float c_g_sensor_timestamp = 111;

class serial_rules_test : public db_catalog_test_base_t
{
public:
    serial_rules_test()
        : db_catalog_test_base_t("incubator.ddl"){};

    void init_storage()
    {
        gaia::db::begin_transaction();
        gaia::barn_storage::incubator_t::insert_row("TestIncubator", c_g_incubator_min_temperature, c_g_incubator_max_temperature);
        gaia::barn_storage::sensor_t::insert_row("TestSensor", c_g_sensor_timestamp, c_g_sensor_value);
        gaia::db::commit_transaction();
    }

protected:
    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();
        gaia::rules::initialize_rules_engine();
    }

    void TearDown() override
    {
        db_catalog_test_base_t::TearDown();
        unsubscribe_rules();
        gaia::rules::shutdown_rules_engine();
    }
};

TEST_F(serial_rules_test, subscribe_serial_ruleset)
{
    // Initializing storage inserts an incubator and a sensor. This calls:
    // rule-1: OnInsert(incubator)
    // rule-2: OnInsert(incubator)
    // rule-3: OnInsert(sensor)
    // rule-4: OnInsert(sensor)
    // rule-5: OnUpdate(incubator.max_temp)
    // rule-6: OnUpdate(incubator.min_temp)
    // rule-7: OnUpdate(sensor.value)
    // rule-8: OnUpdate(sensor.timestamp)
    init_storage();
    gaia::rules::test::wait_for_rules_to_complete();

    EXPECT_EQ(g_update_max_temp_called, 1);
    EXPECT_EQ(g_update_min_temp_called, 1);
    EXPECT_EQ(g_update_max_temp_called, 1);
    EXPECT_EQ(g_update_min_temp_called, 1);

    gaia::db::begin_transaction();

    for (const auto& i : gaia::barn_storage::incubator_t::list())
    {
        EXPECT_EQ(i.max_temp(), c_g_incubator_max_temperature * 2);
        EXPECT_EQ(i.min_temp(), c_g_incubator_min_temperature * 2);
    }

    for (const auto& s : gaia::barn_storage::sensor_t::list())
    {
        EXPECT_EQ(s.value(), c_g_sensor_value * 2);
        EXPECT_EQ(s.timestamp(), c_g_sensor_timestamp * 2);
    }
    gaia::db::commit_transaction();
}
