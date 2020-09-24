/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_db_internal.hpp"
#include "triggers.hpp"
#include "event_manager_settings.hpp"

// Provide helpers that must be linked into the test by including this file.
namespace gaia
{
namespace rules
{
namespace test
{
    void initialize_rules_engine(event_manager_settings_t& settings);

    void commit_trigger(
        gaia_xid_t transaction_id,
        const db::triggers::trigger_event_t* triggger_events,
        size_t count_events);

} // test
} // rules
} // gaia
