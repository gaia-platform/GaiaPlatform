/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include <unistd.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gtest/gtest.h"

#include "gaia/rules/rules.hpp"
#include "gaia_barn_storage.h"

using namespace std;
using namespace gaia::rules;
using namespace gaia::common;
using namespace gaia::barn_storage;

extern int g_text_mixed_called;

const int c_g_rule_execution_delay = 50000;
const float c_g_incubator_min_temperature = 99.0;
const float c_g_incubator_max_temperature = 102.0;

/**
 * Ensure that is possible to intermix cpp code with declarative code.
 */
class test_mixed_code : public db_catalog_test_base_t
{
public:
    test_mixed_code()
        : db_catalog_test_base_t("barn_storage.ddl"){};

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

TEST_F(test_mixed_code, subscribe_valid_ruleset)
{
    gaia::db::begin_transaction();

    auto incubator = incubator_t::get(
        incubator_t::insert_row(
            "TestIncubator",
            c_g_incubator_min_temperature,
            c_g_incubator_max_temperature));

    gaia::db::commit_transaction();

    while (g_text_mixed_called == 0)
    {
        usleep(c_g_rule_execution_delay);
    }

    gaia::db::begin_transaction();

    const auto sensors = sensor_t::list().where([](const sensor_t& s) {
        return strcmp(s.name(), "TestSensor1") == 0;
    });

    ASSERT_EQ(1, std::distance(sensors.begin(), sensors.end()));

    const auto actuators = actuator_t::list().where([](const actuator_t& s) {
        return strcmp(s.name(), "TestActuator1") == 0;
    });

    ASSERT_EQ(1, std::distance(sensors.begin(), sensors.end()));

    gaia::db::commit_transaction();
}
