/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/triggers.hpp"

#include "event_manager_settings.hpp"

// Provide helpers that must be linked into the test by including this file.
namespace gaia
{
namespace rules
{
namespace test
{
void initialize_rules_engine(const event_manager_settings_t& settings);

void commit_trigger(
    const db::triggers::trigger_event_t* trigger_events,
    size_t count_events);

} // namespace test
} // namespace rules
} // namespace gaia
