/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

/*#include <chrono>
#include <cstdio>

#include "rules.hpp"
#include "barn_storage_gaia_generated.h"
#include <atomic>

using namespace gaia::rules;
using namespace BarnStorage;

extern atomic<int> TIMESTAMP;

void decrease_fans(Incubator& incubator, FILE *log);
void increase_fans(Incubator& incubator, FILE *log);

// ruleset
namespace incubator_ruleset
{

//
// rule-sensor_changed: [BarnStorage::Sensor](update)
//
void on_sensor_changed(const rule_context_t *context) {
    Sensor s = Sensor::get(context->record);
    Incubator i = Incubator::get(s.incubator_id());
    FILE *log = fopen("message.log", "a");
    fprintf(log, "%s fired for %s sensor of %s incubator\n", __func__,
            s.name(), i.name());

    double cur_temp = s.value();
    if (cur_temp < i.min_temp()) {
        decrease_fans(i, log);
    } else if (cur_temp > i.max_temp()) {
        increase_fans(i, log);
    }
    fclose(log);
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
}*/
