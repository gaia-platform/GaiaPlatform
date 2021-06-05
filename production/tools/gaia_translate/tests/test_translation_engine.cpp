/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <unistd.h>

#include "gtest/gtest.h"

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_barn_storage.h"
#include "test_rulesets.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace rule_test_helpers;

extern std::atomic<int32_t> g_rule_called;
extern std::atomic<int32_t> g_insert_called;
extern std::atomic<int32_t> g_update_sensor_value_called;
extern std::atomic<int32_t> g_update_min_temp_called;
extern std::atomic<int32_t> g_update_max_temp_called;
extern std::atomic<int32_t> g_actuator_rule_called;

const float c_g_incubator_min_temperature = 99.0;
const float c_g_incubator_max_temperature = 102.0;
const int c_g_expected_sensor_value = 6;
const int c_g_expected_actuator_value = 1000;

class translation_engine_test : public db_catalog_test_base_t
{
public:
    translation_engine_test()
        : db_catalog_test_base_t("incubator.ddl"){};

    gaia_id_t insert_incubator(const char* name, float min_temp, float max_temp)
    {
        gaia::barn_storage::incubator_writer w;
        w.name = name;
        w.min_temp = min_temp;
        w.max_temp = max_temp;
        return w.insert_row();
    }

    void init_storage()
    {
        gaia::db::begin_transaction();
        auto incubator = gaia::barn_storage::incubator_t::get(insert_incubator("TestIncubator", c_g_incubator_min_temperature, c_g_incubator_max_temperature));
        incubator.sensors().insert(gaia::barn_storage::sensor_t::insert_row("TestSensor1", 0, 0.0));
        incubator.actuators().insert(gaia::barn_storage::actuator_t::insert_row("TestActuator1", 0, 0.0));
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

TEST_F(translation_engine_test, subscribe_invalid_ruleset)
{
    EXPECT_THROW(subscribe_ruleset("bogus"), ruleset_not_found);
    EXPECT_THROW(unsubscribe_ruleset("bogus"), ruleset_not_found);
}

TEST_F(translation_engine_test, subscribe_valid_ruleset)
{
    init_storage();

    // Initializing storage inserts an incubator with a sensor and an actuator.  This calls:
    // rule-4: OnChange(sensor.value) [insert]
    // rule-2: OnUpdate(incubator.max_temp)
    // rule-2: OnUpdate(incubator.max_temp)
    const int32_t c_count_rules_called_1 = 3;
    EXPECT_TRUE(wait_for_rule(g_rule_called, c_count_rules_called_1));

    EXPECT_EQ(g_insert_called, 1);
    EXPECT_EQ(g_update_max_temp_called, 2);
    EXPECT_EQ(g_update_sensor_value_called, 0);

    gaia::db::begin_transaction();

    for (const auto& i : gaia::barn_storage::incubator_t::list())
    {
        EXPECT_EQ(i.max_temp(), 4);
    }

    for (const auto& a : gaia::barn_storage::actuator_t::list())
    {
        EXPECT_EQ(a.value(), 0);
    }

    for (const auto& s : gaia::barn_storage::sensor_t::list())
    {
        EXPECT_EQ(s.value(), 0);
    }
    gaia::db::commit_transaction();

    gaia::db::begin_transaction();

    for (auto s : gaia::barn_storage::sensor_t::list())
    {
        auto w = s.writer();
        w.value = c_g_expected_sensor_value;
        w.update_row();
    }
    gaia::db::commit_transaction();

    // Updating a sensor value calls:
    // rule-4: OnChange(sensor.value) [update]
    // rule-2: OnUpdate(incubator.max_temp)
    // rule-2: OnUpdate(incubator.max_temp)
    // rule-3: OnUpdate(actuator.value)
    const int32_t c_count_rules_called_2 = c_count_rules_called_1 + 4;
    EXPECT_TRUE(wait_for_rule(g_rule_called, c_count_rules_called_2));

    EXPECT_EQ(g_insert_called, 1);
    EXPECT_EQ(g_update_max_temp_called, 4);
    EXPECT_EQ(g_update_sensor_value_called, 1);
    EXPECT_EQ(g_actuator_rule_called, 1);

    gaia::db::begin_transaction();

    for (const auto& i : gaia::barn_storage::incubator_t::list())
    {
        EXPECT_EQ(i.max_temp(), 10);
    }

    for (const auto& s : gaia::barn_storage::sensor_t::list())
    {
        EXPECT_EQ(s.value(), c_g_expected_sensor_value);
    }

    for (const auto& a : gaia::barn_storage::actuator_t::list())
    {
        EXPECT_EQ(a.value(), c_g_expected_actuator_value);
    }
    gaia::db::commit_transaction();

    gaia::db::begin_transaction();

    auto s_id = gaia::barn_storage::sensor_t::insert_row("TestSensor2", 0, 0.0);
    auto incubator = *(gaia::barn_storage::incubator_t::list().begin());
    incubator.sensors().insert(s_id);
    gaia::db::commit_transaction();

    // Inserting a new sensor calls:
    // rule-4: OnChange(sensor.value) [insert]
    // rule-2: OnUpdate(incubator.max_temp)
    // rule-2: OnUpdate(incubator.max_temp)
    const int32_t c_count_rules_called_3 = c_count_rules_called_2 + 3;
    EXPECT_TRUE(wait_for_rule(g_rule_called, c_count_rules_called_3));

    gaia::db::begin_transaction();

    auto s = gaia::barn_storage::sensor_t::get(s_id);
    s.incubator().sensors().remove(s);
    s.delete_row();

    gaia::db::commit_transaction();
}
