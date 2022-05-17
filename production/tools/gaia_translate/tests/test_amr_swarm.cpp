/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unordered_set>

#include "gtest/gtest.h"

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/rules/rules_test_helpers.hpp"

#include "constants.hpp"
#include "gaia_amr_swarm.h"
#include "test_rulesets.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace gaia::amr_swarm;

class tools__gaia_translate__amr_swarm__test : public db_catalog_test_base_t
{
public:
    tools__gaia_translate__amr_swarm__test()
        : db_catalog_test_base_t("amr_swarm.ddl", true, true, true)
    {
    }

protected:
    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();
        gaia::rules::initialize_rules_engine();
        gaia::rules::unsubscribe_rules();
    }

    void TearDown() override
    {
        db_catalog_test_base_t::TearDown();
        unsubscribe_rules();
        gaia::rules::shutdown_rules_engine();
    }
};

constexpr uint16_t c_station_type_id = 201;
constexpr uint16_t c_robot1_id = 1;
constexpr uint16_t c_robot2_id = 2;
constexpr uint16_t c_robot3_id = 3;
constexpr uint32_t c_pallet_capacity = 10;
constexpr uint32_t c_widget_capacity = 10;
constexpr float c_max_charge = 1.0;
constexpr char c_sandbox[] = "sand01";

TEST_F(tools__gaia_translate__amr_swarm__test, setup_complete_event)
{
    subscribe_ruleset("amr_swarm_ruleset");

    begin_transaction();

    // Initial conditions require a Charging station, a Widget robot type, a Pallet robot type,
    // a pallet bot and two widget bots.
    station_type_t::insert_row((uint8_t)station_types::Charging, c_sandbox, 0, 8);
    station_t::insert_row(c_station_type_id, c_sandbox, (uint8_t)station_types::Charging);

    robot_type_t::insert_row((uint8_t)robot_types::Pallet, c_sandbox, c_pallet_capacity, 0);
    robot_t::insert_row(c_robot1_id, c_sandbox, c_max_charge, false, true, false, (uint8_t)robot_types::Pallet, c_station_type_id);

    robot_type_t::insert_row((uint8_t)robot_types::Widget, c_sandbox, 0, c_widget_capacity);
    robot_t::insert_row(c_robot2_id, c_sandbox, c_max_charge, false, true, false, (uint8_t)robot_types::Widget, c_station_type_id);
    robot_t::insert_row(c_robot3_id, c_sandbox, c_max_charge, false, true, false, (uint8_t)robot_types::Widget, c_station_type_id);

    // The setup_complete_event row causes the setup rule to fire.
    setup_complete_event_t::insert_row(0);

    commit_transaction();

    test::wait_for_rules_to_complete();

    // Look for expected results of the rule.
    begin_transaction();

    // Obtain the configuration row.
    configuration_t configuration;
    int counter = 0;

    for (const configuration_t& c : configuration_t::list())
    {
        ++counter;
        configuration = c;
    }
    EXPECT_EQ(counter, 1) << "Wrong number of configuration rows.";

    // Expect 3 robots.
    counter = 0;
    for (const auto& robot : configuration.robots())
    {
        EXPECT_EQ(robot.station_id(), c_station_type_id);
        ++counter;
    }
    EXPECT_EQ(counter, 3) << "Wrong number of robots";

    // Expect one robot connected to each of the next 3 relationships. (The
    // assignment of robots to relationships is non-deterministic because it
    // depends on iteration order.)
    std::unordered_set<uint16_t> robot_ids({c_robot1_id, c_robot2_id, c_robot3_id});

    auto robot = configuration.main_pallet_bot();
    EXPECT_EQ(robot_ids.count(robot.id()), 1);
    robot_ids.extract(robot.id());
    robot = configuration.left_widget_bot();
    EXPECT_EQ(robot_ids.count(robot.id()), 1);
    robot_ids.extract(robot.id());
    robot = configuration.right_widget_bot();
    EXPECT_EQ(robot_ids.count(robot.id()), 1);
    robot_ids.extract(robot.id());
    EXPECT_EQ(robot_ids.size(), 0);

    commit_transaction();
}
