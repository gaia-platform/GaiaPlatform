/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "db_types.hpp"
#include "event_manager_settings.hpp"
#include "triggers.hpp"

// Provide helpers that must be linked into the test by including this file.
namespace gaia
{
namespace rules
{
namespace test
{
void initialize_rules_engine(event_manager_settings_t& settings);

void commit_trigger(
    gaia_txn_id_t txn_id,
    const db::triggers::trigger_event_t* trigger_events,
    size_t count_events);

} // namespace test
} // namespace rules
} // namespace gaia
