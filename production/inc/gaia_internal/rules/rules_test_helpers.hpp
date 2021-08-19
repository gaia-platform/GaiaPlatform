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

// This function will block until ALL currently scheduled rules and any rules they cause to be invoked by
// changes to the database are done executing.  Note that this is a best effort API.  It is the application's
// responsibility to ensure that other threads are not making database changes that also will schedule rules.
// In this case, the function may never return (a never ending cycle of rules being run), or may return after
// the current set of rules has finished but before the next set of rules has been scheduled.  A common usage
// pattern of this function is the following:
//
// begin_transaction();
// ... make db changes that would fire a set of rules
// commit_transaction();
// gaia::rules::test::wait_for_rules_to_complete();
// ... verify that rules were or were not executed per the test
//
void wait_for_rules_to_complete();

} // namespace test
} // namespace rules
} // namespace gaia
