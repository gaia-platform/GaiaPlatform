/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "test_serial.hpp"

#include <unistd.h>

#include <mutex>

#include <gtest/gtest.h>

#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/rules/rules_test_helpers.hpp"

#include "gaia_barn_storage.h"
#include "test_rulesets.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;

extern std::atomic<int32_t> g_update_value_called;
extern std::atomic<int32_t> g_update_timestamp_called;
extern std::atomic<int32_t> g_update_min_temp_called;
extern std::atomic<int32_t> g_update_max_temp_called;

std::atomic<int32_t> g_rule_called;

bool g_in_rule = false;
bool g_in_A = false;
bool g_in_B = false;

// See if any concurrent calls occurred.
int32_t g_total_concurrent_calls = 0;
// Verify that all rule invocations in serial group A were serialized.
int32_t g_A_concurrent_calls = 0;
// Verify that all rule invocations in serial group B were serialized.
int32_t g_B_concurrent_calls = 0;
// Use this value if you just want to assert that at least one concurrent call occurred.
int32_t c_g_at_least_one = -1;

std::mutex g_call_mutex;

const float c_g_incubator_min_temperature = 1;
const float c_g_incubator_max_temperature = 10;
const float c_g_sensor_value = 21;
const float c_g_sensor_timestamp = 111;

rule_monitor_t::rule_monitor_t(
    const char* rule_id,
    char serial_group,
    const char* ruleset_name,
    const char* rule_name,
    uint32_t event_type,
    gaia::common::gaia_type_t gaia_type)
{
    m_group = serial_group;
    enter_rule();
    // Hang out here so that if another rule is scheduled
    // and we are running concurrently then it will run.
    usleep(5000);
    gaia_log::app().warn("[{}] begin {} {} {} {}", rule_id, ruleset_name, rule_name, event_type, gaia_type);
    g_rule_called++;
}

rule_monitor_t::~rule_monitor_t()
{
    exit_rule();
}

void rule_monitor_t::enter_rule()
{
    std::lock_guard guard(g_call_mutex);

    if (g_in_rule)
    {
        g_total_concurrent_calls++;

        if (g_in_A && m_group == c_A)
        {
            g_A_concurrent_calls++;
        }

        if (g_in_B && m_group == c_B)
        {
            g_B_concurrent_calls++;
        }
    }

    set_group(true);
}

void rule_monitor_t::exit_rule()
{
    std::lock_guard guard(g_call_mutex);
    set_group(false);
}

void rule_monitor_t::set_group(bool value)
{
    g_in_rule = value;
    if (m_group == c_A)
    {
        g_in_A = value;
    }
    if (m_group == c_B)
    {
        g_in_B = value;
    }
}

class tools__gaia_translate__serial__test : public db_catalog_test_base_t
{
public:
    tools__gaia_translate__serial__test()
        : db_catalog_test_base_t("incubator.ddl", true, true, true)
    {
    }

    void init_storage()
    {
        gaia::db::begin_transaction();
        gaia::barn_storage::incubator_t::insert_row("TestIncubator", c_g_incubator_min_temperature, c_g_incubator_max_temperature);
        gaia::barn_storage::sensor_t::insert_row("TestSensor", c_g_sensor_timestamp, c_g_sensor_value);
        gaia::db::commit_transaction();
    }

    void verify_concurrency(
        int32_t expected_rule_called,
        int32_t expected_total_concurrent_calls = 0)
    {
        EXPECT_EQ(g_rule_called, expected_rule_called);
        if (expected_total_concurrent_calls == c_g_at_least_one)
        {
            EXPECT_GT(g_total_concurrent_calls, 0);
        }
        else
        {
            EXPECT_EQ(g_total_concurrent_calls, expected_total_concurrent_calls);
        }

        // Within a serial group, we should never get any concurrent calls
        EXPECT_EQ(g_A_concurrent_calls, 0);
        EXPECT_EQ(g_B_concurrent_calls, 0);
    }

    void verify_values(
        float expected_min_temp = c_g_incubator_min_temperature,
        float expected_max_temp = c_g_incubator_max_temperature,
        float expected_value = c_g_sensor_value,
        float expected_timestamp = c_g_sensor_timestamp,
        int32_t expected_update_min_temp_called = 0,
        int32_t expected_update_max_temp_called = 0,
        int32_t expected_update_value_called = 0,
        int32_t expected_update_timestamp_called = 0)
    {
        EXPECT_EQ(g_update_min_temp_called, expected_update_min_temp_called);
        EXPECT_EQ(g_update_max_temp_called, expected_update_max_temp_called);
        EXPECT_EQ(g_update_value_called, expected_update_value_called);
        EXPECT_EQ(g_update_timestamp_called, expected_update_timestamp_called);

        gaia::db::begin_transaction();

        for (const auto& i : gaia::barn_storage::incubator_t::list())
        {
            EXPECT_EQ(i.max_temp(), expected_max_temp);
            EXPECT_EQ(i.min_temp(), expected_min_temp);
        }

        for (const auto& s : gaia::barn_storage::sensor_t::list())
        {
            EXPECT_EQ(s.value(), expected_value);
            EXPECT_EQ(s.timestamp(), expected_timestamp);
        }

        gaia::db::commit_transaction();
    }

protected:
    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();
        gaia::rules::initialize_rules_engine();
        unsubscribe_rules();

        g_total_concurrent_calls = 0;
        g_A_concurrent_calls = 0;
        g_B_concurrent_calls = 0;
        g_in_rule = false;
        g_rule_called = 0;

        g_update_max_temp_called = 0;
        g_update_min_temp_called = 0;
        g_update_value_called = 0;
        g_update_timestamp_called = 0;
    }

    void TearDown() override
    {
        db_catalog_test_base_t::TearDown();
        gaia::rules::shutdown_rules_engine();
    }
};

TEST_F(tools__gaia_translate__serial__test, default_serial)
{
    subscribe_ruleset("test_default_serial");
    init_storage();
    gaia::rules::test::wait_for_rules_to_complete();

    verify_concurrency(2);
    verify_values(c_g_incubator_min_temperature * 4);
}

TEST_F(tools__gaia_translate__serial__test, default_parallel)
{
    subscribe_ruleset("test_default_parallel");
    init_storage();
    gaia::rules::test::wait_for_rules_to_complete();

    verify_concurrency(2, 1);
    verify_values(c_g_incubator_min_temperature * 2, c_g_incubator_max_temperature, c_g_sensor_value * 2);
}

TEST_F(tools__gaia_translate__serial__test, multiple_serial_same_group)
{
    subscribe_ruleset("test_serial_1A");
    subscribe_ruleset("test_serial_2A");
    init_storage();
    gaia::rules::test::wait_for_rules_to_complete();

    verify_concurrency(4);
    verify_values(c_g_incubator_min_temperature * 4, c_g_incubator_max_temperature * 4);
}

TEST_F(tools__gaia_translate__serial__test, multiple_serial_different_groups)
{
    subscribe_ruleset("test_serial_1A");
    subscribe_ruleset("test_serial_2A");
    subscribe_ruleset("test_serial_1B");
    subscribe_ruleset("test_serial_2B");

    init_storage();
    gaia::rules::test::wait_for_rules_to_complete();

    verify_concurrency(8, c_g_at_least_one);
    verify_values(c_g_incubator_min_temperature * 4, c_g_incubator_max_temperature * 4, c_g_sensor_value * 4, c_g_sensor_timestamp * 4);
}

TEST_F(tools__gaia_translate__serial__test, mixed_serial_parallel)
{
    subscribe_ruleset("test_serial_1A");
    subscribe_ruleset("test_serial_2A");
    subscribe_ruleset("test_serial_1B");
    subscribe_ruleset("test_serial_2B");
    subscribe_ruleset("test_parallel");

    init_storage();
    gaia::rules::test::wait_for_rules_to_complete();

    verify_concurrency(16, c_g_at_least_one);
    verify_values(c_g_incubator_min_temperature * 4, c_g_incubator_max_temperature * 4, c_g_sensor_value * 4, c_g_sensor_timestamp * 4, 2, 2, 2, 2);
}
