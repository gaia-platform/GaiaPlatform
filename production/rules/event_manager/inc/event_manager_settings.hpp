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
// Catalog checks are enabled.
// Gathering performance statistcs for individual rules is disabled.
// Log performance statistics every 10s.
struct event_manager_settings_t
{
    event_manager_settings_t()
        : num_background_threads(SIZE_MAX)
        , enable_catalog_checks(true)
        , enable_rule_stats(false)
        , stats_log_interval(10)
    {
    }

    // Specifies the number of backround threads in the rule scheduler's thread pool.
    // Specifying 0 will make rule invocations occur synchronously.
    size_t num_background_threads;
    // Specifies whether the catalog will be used to verify rule subscriptions.
    // Specifying true will allow rule subscriptions without comparing the table
    // and fields to existing catalog definitions.
    bool enable_catalog_checks;
    // Enable logging of rule specific statistcs.
    bool enable_rule_stats;
    // Specifies the interval in seconds that performance statistics
    // are logged to the logger file. Set to 0 to disable logging statistics.
    uint32_t stats_log_interval;
};

} // rules
} // gaia
