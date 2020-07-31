/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "triggers.hpp"
#include "event_manager_settings.hpp"

// Provide helpers that must be linked into the test by including this file
namespace gaia
{
namespace rules
{
namespace test
{
    // Allows specifying the number of background threads that the rule scheduler uses as well
    // as whether to validate rule subscriptions with the catalog.  Unit tests 
    // will typically provide 0 for number of threads to run synchronously as well
    // as false for catalog verification.  Integration tests should not use
    // this test only intialize_rules_engine.
    void initialize_rules_engine(event_manager_settings_t& settings);

    void commit_trigger(
        uint64_t transaction_id,
        const db::triggers::trigger_event_t* triggger_events,
        size_t count_events);

} // test
} // rules
} // gaia
