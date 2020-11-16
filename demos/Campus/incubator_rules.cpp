#include "gaia_incubator.h"
#include "rules.hpp"
using namespace gaia::rules;
#include <atomic>
#include <algorithm>

using namespace std;

extern atomic<int> TIMESTAMP;
const double FAN_SPEED_LIMIT = 5000.0;
const double FAN_SPEED_INCREMENT = 500;

namespace incubator_ruleset
{
  
void incubator_ruleset_A3BqyrkBaT57xMe_2(const rule_context_t* context)
{
auto sensor = gaia::incubator::sensor_t::get(context->record);
auto incubator = sensor.incubator();
for (auto actuator : incubator.actuator_list())
{

        // Prefixing an active field that is being read with '@' signals to the Gaia Platform that this 
        // rule should not be called if the field changes. If the '@' were not present, then this rule 
        // would be if the incubator turned on or off.
        if (!incubator.is_on())
        {
            return;
        }

        if (sensor.value() >= incubator.max_temp())
        {
            [&]() mutable {auto w = actuator.writer(); w.value= min(FAN_SPEED_LIMIT, actuator.value() + FAN_SPEED_INCREMENT); w.update_row();return w.value;}();
            [&]() mutable {auto w = actuator.writer(); w.timestamp= TIMESTAMP; w.update_row();return w.timestamp;}();
        }
        else if (sensor.value() <= incubator.min_temp())
        {
            [&]() mutable {auto w = actuator.writer(); w.value= max(0.0, actuator.value() - (2*FAN_SPEED_INCREMENT)); w.update_row();return w.value;}();
            [&]() mutable {auto w = actuator.writer(); w.timestamp= TIMESTAMP; w.update_row();return w.timestamp;}();
        }
    }}

void incubator_ruleset_A3BqyrkBaT57xMe_1(const rule_context_t* context)
{
auto incubator = gaia::incubator::incubator_t::get(context->record);
for (auto actuator : incubator.actuator_list())
{
for (auto sensor : incubator.sensor_list())
{

        // Prefixing an active field that is being read with '@' signals to the Gaia Platform that this 
        // rule should not be called if the field changes. If the '@' were not present, then this rule 
        // would be if the incubator turned on or off.
        if (!incubator.is_on())
        {
            return;
        }

        if (sensor.value() >= incubator.max_temp())
        {
            [&]() mutable {auto w = actuator.writer(); w.value= min(FAN_SPEED_LIMIT, actuator.value() + FAN_SPEED_INCREMENT); w.update_row();return w.value;}();
            [&]() mutable {auto w = actuator.writer(); w.timestamp= TIMESTAMP; w.update_row();return w.timestamp;}();
        }
        else if (sensor.value() <= incubator.min_temp())
        {
            [&]() mutable {auto w = actuator.writer(); w.value= max(0.0, actuator.value() - (2*FAN_SPEED_INCREMENT)); w.update_row();return w.value;}();
            [&]() mutable {auto w = actuator.writer(); w.timestamp= TIMESTAMP; w.update_row();return w.timestamp;}();
        }
    }
}
}

    // Rule 2:  Turn off all the fans if the incubator is powered down.    
    
void incubator_ruleset_UNxiBWGsvLBxOaj_1(const rule_context_t* context)
{
auto incubator = gaia::incubator::incubator_t::get(context->record);
for (auto actuator : incubator.actuator_list())
{

        if (!incubator.is_on())
        {
            [&]() mutable {auto w = actuator.writer(); w.value= 0; w.update_row();return w.value;}();
            [&]() mutable {auto w = actuator.writer(); w.timestamp= TIMESTAMP; w.update_row();return w.timestamp;}();
        }
    }
}

    // Rule 3:  If the fan is at 70% of its limit and the temperature is still too hot then
    // set the fan to its maximum speed.
    
void incubator_ruleset_fphZ2BmRlBx498K_1(const rule_context_t* context)
{
auto actuator = gaia::incubator::actuator_t::get(context->record);
auto incubator = actuator.incubator();
for (auto sensor : incubator.sensor_list())
{

        if (actuator.value() == FAN_SPEED_LIMIT)
        {
            return;
        }

        if (actuator.value() > 0.7 * FAN_SPEED_LIMIT && sensor.value() > incubator.max_temp())
        {
            [&]() mutable {auto w = actuator.writer(); w.value= FAN_SPEED_LIMIT; w.update_row();return w.value;}();
        }
    }
}

/*
    // Rule 4 on power on, turn on each actuator to make sure it is working.
    {
        if (incubator.is_on) 
        {
            actuator.value = 1500;
        }
    }
*/

/*
    // Rule 4 improvement: only run the POST for the first power-on on the incubator.
    // Requires a schema change to add "post_complete" field to the incubator.  Intended
    // to demonstrate a possible developer workflow.
    {
        if (is_on && !@post_complete) 
        {
            actuator.value = 1500;
        }
        post_complete = true;
    }
*/
}
namespace incubator_ruleset{
void subscribeRuleset_incubator_ruleset()
{
rule_binding_t incubator_ruleset_A3BqyrkBaT57xMe_1binding("incubator_ruleset","incubator_ruleset::2_incubator",incubator_ruleset::incubator_ruleset_A3BqyrkBaT57xMe_1);
field_position_list_t fields_incubator_ruleset_A3BqyrkBaT57xMe_1;
fields_incubator_ruleset_A3BqyrkBaT57xMe_1.push_back(2);
fields_incubator_ruleset_A3BqyrkBaT57xMe_1.push_back(3);
subscribe_rule(gaia::incubator::incubator_t::s_gaia_type, event_type_t::row_update, fields_incubator_ruleset_A3BqyrkBaT57xMe_1,incubator_ruleset_A3BqyrkBaT57xMe_1binding);
rule_binding_t incubator_ruleset_A3BqyrkBaT57xMe_2binding("incubator_ruleset","incubator_ruleset::2_sensor",incubator_ruleset::incubator_ruleset_A3BqyrkBaT57xMe_2);
field_position_list_t fields_incubator_ruleset_A3BqyrkBaT57xMe_2;
fields_incubator_ruleset_A3BqyrkBaT57xMe_2.push_back(2);
subscribe_rule(gaia::incubator::sensor_t::s_gaia_type, event_type_t::row_update, fields_incubator_ruleset_A3BqyrkBaT57xMe_2,incubator_ruleset_A3BqyrkBaT57xMe_2binding);
rule_binding_t incubator_ruleset_UNxiBWGsvLBxOaj_1binding("incubator_ruleset","incubator_ruleset::3",incubator_ruleset::incubator_ruleset_UNxiBWGsvLBxOaj_1);
field_position_list_t fields_incubator_ruleset_UNxiBWGsvLBxOaj_1;
fields_incubator_ruleset_UNxiBWGsvLBxOaj_1.push_back(1);
subscribe_rule(gaia::incubator::incubator_t::s_gaia_type, event_type_t::row_update, fields_incubator_ruleset_UNxiBWGsvLBxOaj_1,incubator_ruleset_UNxiBWGsvLBxOaj_1binding);
rule_binding_t incubator_ruleset_fphZ2BmRlBx498K_1binding("incubator_ruleset","incubator_ruleset::4",incubator_ruleset::incubator_ruleset_fphZ2BmRlBx498K_1);
field_position_list_t fields_incubator_ruleset_fphZ2BmRlBx498K_1;
fields_incubator_ruleset_fphZ2BmRlBx498K_1.push_back(2);
subscribe_rule(gaia::incubator::actuator_t::s_gaia_type, event_type_t::row_update, fields_incubator_ruleset_fphZ2BmRlBx498K_1,incubator_ruleset_fphZ2BmRlBx498K_1binding);
}
void unsubscribeRuleset_incubator_ruleset()
{
rule_binding_t incubator_ruleset_A3BqyrkBaT57xMe_1binding("incubator_ruleset","incubator_ruleset::2_incubator",incubator_ruleset::incubator_ruleset_A3BqyrkBaT57xMe_1);
field_position_list_t fields_incubator_ruleset_A3BqyrkBaT57xMe_1;
fields_incubator_ruleset_A3BqyrkBaT57xMe_1.push_back(2);
fields_incubator_ruleset_A3BqyrkBaT57xMe_1.push_back(3);
unsubscribe_rule(gaia::incubator::incubator_t::s_gaia_type, event_type_t::row_update, fields_incubator_ruleset_A3BqyrkBaT57xMe_1,incubator_ruleset_A3BqyrkBaT57xMe_1binding);
rule_binding_t incubator_ruleset_A3BqyrkBaT57xMe_2binding("incubator_ruleset","incubator_ruleset::2_sensor",incubator_ruleset::incubator_ruleset_A3BqyrkBaT57xMe_2);
field_position_list_t fields_incubator_ruleset_A3BqyrkBaT57xMe_2;
fields_incubator_ruleset_A3BqyrkBaT57xMe_2.push_back(2);
unsubscribe_rule(gaia::incubator::sensor_t::s_gaia_type, event_type_t::row_update, fields_incubator_ruleset_A3BqyrkBaT57xMe_2,incubator_ruleset_A3BqyrkBaT57xMe_2binding);
rule_binding_t incubator_ruleset_UNxiBWGsvLBxOaj_1binding("incubator_ruleset","incubator_ruleset::3",incubator_ruleset::incubator_ruleset_UNxiBWGsvLBxOaj_1);
field_position_list_t fields_incubator_ruleset_UNxiBWGsvLBxOaj_1;
fields_incubator_ruleset_UNxiBWGsvLBxOaj_1.push_back(1);
unsubscribe_rule(gaia::incubator::incubator_t::s_gaia_type, event_type_t::row_update, fields_incubator_ruleset_UNxiBWGsvLBxOaj_1,incubator_ruleset_UNxiBWGsvLBxOaj_1binding);
rule_binding_t incubator_ruleset_fphZ2BmRlBx498K_1binding("incubator_ruleset","incubator_ruleset::4",incubator_ruleset::incubator_ruleset_fphZ2BmRlBx498K_1);
field_position_list_t fields_incubator_ruleset_fphZ2BmRlBx498K_1;
fields_incubator_ruleset_fphZ2BmRlBx498K_1.push_back(2);
unsubscribe_rule(gaia::incubator::actuator_t::s_gaia_type, event_type_t::row_update, fields_incubator_ruleset_fphZ2BmRlBx498K_1,incubator_ruleset_fphZ2BmRlBx498K_1binding);
}
}
namespace gaia {
 namespace rules{
extern "C" void subscribe_ruleset(const char* ruleset_name)
{
if (strcmp(ruleset_name, "incubator_ruleset") == 0)
{
::incubator_ruleset::subscribeRuleset_incubator_ruleset();
return;
}
throw ruleset_not_found(ruleset_name);
}
extern "C" void unsubscribe_ruleset(const char* ruleset_name)
{
if (strcmp(ruleset_name, "incubator_ruleset") == 0)
{
::incubator_ruleset::unsubscribeRuleset_incubator_ruleset();
return;
}
throw ruleset_not_found(ruleset_name);
}
extern "C" void initialize_rules()
{
::incubator_ruleset::subscribeRuleset_incubator_ruleset();
}
}
}