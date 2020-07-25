/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "triggers.hpp"

// Provide helpers that must be linked into the test by including this file
namespace gaia
{
namespace rules
{
namespace test
{

    void initialize_rules_engine(size_t num_threads);

    void commit_trigger(
        uint64_t transaction_id,
        const trigger_event_t* triggger_events,
        size_t count_events);

} // test
} // rules
} // gaia
