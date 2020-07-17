/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <chrono>
#include <cstdio>

#include "rules.hpp"
#include "barn_storage_gaia_generated.h"

using namespace gaia::rules;
using namespace BarnStorage;

// This code only exists to verify that the gaia_incubator package is
// linked to the Gaia production libraries. In the future, this
// implementation will contain new rules for the ROS demo.

/** ruleset*/
namespace incubator_ruleset
{

/**
 rule-sensor_changed: [BarnStorage::Sensor](update)
*/
void on_sensor_changed(const rule_context_t *context) {
    printf("on_sensor_changed!\n");
}
}

void add_fan_control_rule() {
    try {
        rule_binding_t fan_control("incubator_ruleset", "rule-sensor_changed",
            incubator_ruleset::on_sensor_changed);
        subscribe_rule(Sensor::s_gaia_type, event_type_t::row_update,
            empty_fields, fan_control);
    } catch (duplicate_rule) {
        printf("The rule has already been added.\n");
    }
}
