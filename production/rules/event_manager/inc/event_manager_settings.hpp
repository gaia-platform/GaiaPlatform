/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <cstddef>

namespace gaia
{
namespace rules
{

//
// This structure allows overriding default event_manager_t behavior with
// custom settings. It is only used by unit tests today but may be made
// public to allow customers to optimize the rules engine for their needs.
//
// Default event manager settings are:
// Number of background threads equals number of hardware threads (SIZE_MAX).
// Catalog checks are enabled (TRUE).
// Gathering performance statistics is disabled (FALSE).
struct event_manager_settings_t
{
    event_manager_settings_t()
        : num_background_threads(SIZE_MAX)
        , enable_catalog_checks(true)
        , enable_stats(false)
    {
    }

    // Specifies the number of backround threads in the rule scheduler's thread pool.
    // Specifying 0 will make rule invocations occur synchronously.
    size_t num_background_threads;
    // Specifies whether the catalog will be used to verify rule subscriptions.
    // Specifying true will allow rule subscriptions without comparing the table
    // and fields to existing catalog definitions.
    bool enable_catalog_checks;
    // Enable logging of event_manager performance statistics
    bool enable_stats;
};

} // rules
} // gaia
