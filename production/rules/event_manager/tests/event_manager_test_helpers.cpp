/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "event_manager.hpp"

// Test code can access these helpers by linking them
// into their test.
void gaia::rules::commit_trigger_test(
    uint64_t transaction_id, 
    const trigger_event_t* trigger_events, 
    size_t count_events, 
    bool immediate) 
{
    trigger_event_list_t trigger_event_list;
    for (size_t i = 0; i < count_events; i++)
    {
        trigger_event_list.push_back(trigger_events[i]);
    }

    event_manager_t::get().commit_trigger(transaction_id, trigger_event_list, immediate);
}
