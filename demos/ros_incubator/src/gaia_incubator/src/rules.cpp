/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

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
 rule-sensor_inserted: [BarnStorage::Sensor](insert)
*/
void on_sensor_inserted(const rule_context_t *context)
{
    FILE* log = fopen("message.log", "a");
    fprintf(log, "beans");
    fclose(log);

    Sensor* s = Sensor::get_row_by_id(context->record);
    // TODO: try writing to a file
    // TODO: implement initialize_rules()
    printf("%s\n", s->name());
}

}
