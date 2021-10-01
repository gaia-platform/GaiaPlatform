/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

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

class test_amr_swarm : public db_catalog_test_base_t
{
public:
    test_amr_swarm()
        : db_catalog_test_base_t("amr_swarm.ddl"){};

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

TEST_F(test_amr_swarm, setup_complete_event)
{
    gaia::rules::subscribe_ruleset("amr_swarm_ruleset");

    gaia::db::begin_transaction();

    // Initial conditions require a Charging station, a Widget robot type, a Pallet robot type,
    // a pallet bot and two widget bots.
    station_type_t::insert_row((uint8_t)station_types::Charging, "sand01", 0, 8);
    station_t::insert_row(c_station_type_id, "sand01", (uint8_t)station_types::Charging);

    robot_type_t::insert_row((uint8_t)robot_types::Pallet, "sand01", c_pallet_capacity, 0);
    robot_t::insert_row(c_robot1_id, "robot01", c_max_charge, false, true, false, (uint8_t)robot_types::Pallet, c_station_type_id);

    robot_type_t::insert_row((uint8_t)robot_types::Widget, "sand01", 0, c_widget_capacity);
    robot_t::insert_row(c_robot2_id, "robot02", c_max_charge, false, true, false, (uint8_t)robot_types::Widget, c_station_type_id);
    robot_t::insert_row(c_robot3_id, "robot03", c_max_charge, false, true, false, (uint8_t)robot_types::Widget, c_station_type_id);

    // The setup_complete_event row causes the setup rule to fire.
    setup_complete_event_t::insert_row(0);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();
}
