////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "event_manager.hpp"

// Test code can access these helpers by linking them
// into their test.

void gaia::rules::test::initialize_rules_engine(const event_manager_settings_t& settings)
{
    bool require_initialized = false;
    event_manager_t::get(require_initialized).init(settings);
    initialize_rules();
}

void gaia::rules::test::commit_trigger(
    const db::triggers::trigger_event_t* trigger_events,
    size_t count_events)
{
    db::triggers::trigger_event_list_t trigger_event_list;
    for (size_t i = 0; i < count_events; i++)
    {
        trigger_event_list.emplace_back(trigger_events[i]);
    }

    event_manager_t::get().commit_trigger(trigger_event_list);

    // execute the rules synchronously now
    event_manager_t::get().m_invocations->execute_immediate();
}

void gaia::rules::test::wait_for_rules_to_complete()
{
    event_manager_t& event_manager = event_manager_t::get();
    if (event_manager.m_invocations)
    {
        event_manager.m_invocations->wait_for_rules_to_complete();
    }
}
