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
    void commit_trigger_test(
        uint64_t transaction_id, 
        const trigger_event_t* triggger_events,
        size_t count_events, 
        bool immediate);
} // namespace rules
} // namespace gaia
