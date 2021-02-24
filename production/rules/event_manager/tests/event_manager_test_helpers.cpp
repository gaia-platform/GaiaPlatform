/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
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
    db::gaia_txn_id_t txn_id,
    const db::triggers::trigger_event_t* trigger_events,
    size_t count_events)
{
    db::triggers::trigger_event_list_t trigger_event_list;
    for (size_t i = 0; i < count_events; i++)
    {
        trigger_event_list.emplace_back(trigger_events[i]);
    }

    event_manager_t::get().commit_trigger(txn_id, trigger_event_list);

    // execute the rules synchronously now
    event_manager_t::get().m_invocations->execute_immediate();
}
