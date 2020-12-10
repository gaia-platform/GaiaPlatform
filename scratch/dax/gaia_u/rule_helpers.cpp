#include "gaia_gaia_u.h"
#include "rule_helpers.hpp"

//void my_func(gaia::gaia_u::Buildings_t& building);
void event_planner::cancel_event(gaia::common::gaia_id_t event_id)
{
    printf("deleted event:  %lu\n", event_id);
}