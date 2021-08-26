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

using namespace std;
using namespace gaia::barn_storage;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;

int g_test_mixed_value;

const float c_g_incubator_min_temperature = 99.0;
const float c_g_incubator_max_temperature = 102.0;

/**
 * Ensure that is possible to intermix cpp code with declarative code.
 */
class test_mixed_code : public db_catalog_test_base_t
{
public:
    test_mixed_code()
        : db_catalog_test_base_t("incubator.ddl"){};

protected:
    void SetUp() override
    {
        g_test_mixed_value = 0;
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

TEST_F(test_mixed_code, subscribe_valid_ruleset)
{
    gaia::db::begin_transaction();

    auto incubator = incubator_t::get(
        incubator_t::insert_row(
            "TestIncubator",
            c_g_incubator_min_temperature,
            c_g_incubator_max_temperature));

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    gaia::db::begin_transaction();

    const auto sensors = sensor_t::list().where([](const sensor_t& s) {
        return strcmp(s.name(), "TestSensor1") == 0;
    });

    ASSERT_EQ(2, std::distance(sensors.begin(), sensors.end()));

    const auto actuators = actuator_t::list().where([](const actuator_t& s) {
        return strcmp(s.name(), "TestActuator1") == 0;
    });

    ASSERT_EQ(2, std::distance(sensors.begin(), sensors.end()));

    gaia::db::commit_transaction();
}

// TESTCASE: create then delete a row so it doesn't exist as anchor in rule
TEST_F(test_mixed_code, insert_delete_row)
{
    gaia::db::begin_transaction();

    auto sensor = sensor_t::get(sensor_t::insert_row("TestSensor", 20210708, 98.6));
    sensor.delete_row();

    g_test_mixed_value = 0;
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    ASSERT_EQ(g_test_mixed_value, 0);
}
