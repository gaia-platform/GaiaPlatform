/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <stddef.h>

namespace gaia
{
namespace rules
{

//
// This structure allows overriding default event_manager_t behavior with
// custom settings. It is only used by unit tests today but may be made
// public to allow customers to optimize the rules engine for their needs.
//
struct event_manager_settings_t {
    // Specifies the number of backround threads in the rule scheduler's thread pool.
    // Specifying 0 will make rule invocations occur synchronously.
    size_t num_background_threads;
    // Specifies whether the catalog will be used to verify rule subscriptions.
    // Specifying false will allow rule subscriptions without comparing the table
    // and fields to existing catalog definitions.
    bool disable_catalog_checks;
};

} // rules
} // gaia
