#include "gaia_school.h"
#include "gaia/rules/rules.hpp"
using namespace gaia::common;
using namespace gaia::db::triggers;
using namespace gaia::rules;
#include <chrono>
#include <thread>
#include <iostream>

namespace school_ruleset
{
  
// NOLINTNEXTLINE(readability-identifier-naming)
void school_ruleset_48wX9uHMQXV7ZY0_1(const rule_context_t* context)
{
auto building = gaia::school::building_t::get(context->record);

        if (!building.door_closed()) {
            std::chrono::seconds duration(10);
            std::this_thread::sleep_for(duration);
            [&]() mutable {auto w = building.writer(); w.door_closed= true; w.update_row();return w.door_closed;}();
        }
    }

    // Open the building door if there is a regonized face
    
// NOLINTNEXTLINE(readability-identifier-naming)
void school_ruleset_iWZBm_4ON596zRG_1(const rule_context_t* context)
{
auto face_scan = gaia::school::face_scan_t::get(context->record);
auto building = face_scan.location_building();

        if (context->last_operation(gaia::school::face_scan_t::s_gaia_type) == gaia::rules::last_operation_t::row_insert) {
            if (face_scan.recognized()) {
                [&]() mutable {auto w = building.writer(); w.door_closed= false; w.update_row();return w.door_closed;}();
            }
        }
    }
}// namespace school_ruleset
namespace school_ruleset{
void subscribe_ruleset_school_ruleset()
{
    // NOLINTNEXTLINE(readability-identifier-naming)
    rule_binding_t school_ruleset_48wX9uHMQXV7ZY0_1binding("school_ruleset","school_ruleset::1",school_ruleset::school_ruleset_48wX9uHMQXV7ZY0_1);
    // NOLINTNEXTLINE(readability-identifier-naming)
    field_position_list_t fields_school_ruleset_48wX9uHMQXV7ZY0_1;
    fields_school_ruleset_48wX9uHMQXV7ZY0_1.push_back(1);
    subscribe_rule(gaia::school::building_t::s_gaia_type, event_type_t::row_update, fields_school_ruleset_48wX9uHMQXV7ZY0_1,school_ruleset_48wX9uHMQXV7ZY0_1binding);

    // NOLINTNEXTLINE(readability-identifier-naming)
    rule_binding_t school_ruleset_iWZBm_4ON596zRG_1binding("school_ruleset","school_ruleset::2",school_ruleset::school_ruleset_iWZBm_4ON596zRG_1);
    subscribe_rule(gaia::school::face_scan_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,school_ruleset_iWZBm_4ON596zRG_1binding);
    subscribe_rule(gaia::school::face_scan_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,school_ruleset_iWZBm_4ON596zRG_1binding);

}
void unsubscribe_ruleset_school_ruleset()
{
    // NOLINTNEXTLINE(readability-identifier-naming)
    rule_binding_t school_ruleset_48wX9uHMQXV7ZY0_1binding("school_ruleset","school_ruleset::1",school_ruleset::school_ruleset_48wX9uHMQXV7ZY0_1);
    // NOLINTNEXTLINE(readability-identifier-naming)
    field_position_list_t fields_school_ruleset_48wX9uHMQXV7ZY0_1;
    fields_school_ruleset_48wX9uHMQXV7ZY0_1.push_back(1);
    unsubscribe_rule(gaia::school::building_t::s_gaia_type, event_type_t::row_update, fields_school_ruleset_48wX9uHMQXV7ZY0_1,school_ruleset_48wX9uHMQXV7ZY0_1binding);

    // NOLINTNEXTLINE(readability-identifier-naming)
    rule_binding_t school_ruleset_iWZBm_4ON596zRG_1binding("school_ruleset","school_ruleset::2",school_ruleset::school_ruleset_iWZBm_4ON596zRG_1);
    unsubscribe_rule(gaia::school::face_scan_t::s_gaia_type, event_type_t::row_update, gaia::rules::empty_fields,school_ruleset_iWZBm_4ON596zRG_1binding);
    unsubscribe_rule(gaia::school::face_scan_t::s_gaia_type, event_type_t::row_insert, gaia::rules::empty_fields,school_ruleset_iWZBm_4ON596zRG_1binding);

}
} // namespace school_ruleset
namespace gaia
{
namespace rules
{
extern "C" void subscribe_ruleset(const char* ruleset_name)
{
    if (strcmp(ruleset_name, "school_ruleset") == 0)
    {
        ::school_ruleset::subscribe_ruleset_school_ruleset();
        return;
    }
    throw ruleset_not_found(ruleset_name);
}
extern "C" void unsubscribe_ruleset(const char* ruleset_name)
{
    if (strcmp(ruleset_name, "school_ruleset") == 0)
    {
        ::school_ruleset::unsubscribe_ruleset_school_ruleset();
        return;
    }
    throw ruleset_not_found(ruleset_name);
}
extern "C" void initialize_rules()
{
    ::school_ruleset::subscribe_ruleset_school_ruleset();
}
} // namespace rules
} // namespace gaia
