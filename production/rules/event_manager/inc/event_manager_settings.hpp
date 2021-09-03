/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <cstddef>

#include "cpptoml.h"

namespace gaia
{
namespace rules
{

// This structure allows overriding default event_manager_t behavior with
// custom settings.
struct event_manager_settings_t
{
    // Section and key names for the Rules configuration section
    // of a gaia.conf file.
    static inline const char* c_rules_section = "Rules";
    static inline const char* c_thread_count_key = "thread_pool_count";
    static inline const char* c_stats_log_interval_key = "stats_log_interval";
    static inline const char* c_log_individual_rule_stats_key = "log_individual_rule_stats";
    static inline const char* c_rule_retry_count_key = "rule_retry_count";

    static void parse_rules_config(
        std::shared_ptr<cpptoml::table>& root_config,
        event_manager_settings_t& settings);

    event_manager_settings_t()
        : num_background_threads(SIZE_MAX)
        , enable_catalog_checks(true)
        , enable_db_checks(true)
        , enable_rule_stats(false)
        , stats_log_interval(10)
        , max_rule_retries(1)
    {
    }

    // Specifies the number of backround threads in the rule scheduler's thread pool.
    // Specifying 0 will make rule invocations occur synchronously.  Specifying SIZE_MAX
    // causes num_background_threads to be set to the number of available hardware threads.
    size_t num_background_threads;
    // Specifies whether the catalog will be used to verify rule subscriptions.
    // Specifying true will allow rule subscriptions without comparing the table
    // and fields to existing catalog definitions.
    bool enable_catalog_checks;
    // Specifies whether rule anchor rows are validated by the database
    // before invoking a rule.
    bool enable_db_checks;
    // Enable logging of rule specific statistcs.
    bool enable_rule_stats;
    // Specifies the interval in seconds that performance statistics
    // are logged to the logger file. Set to 0 to disable logging statistics.
    uint32_t stats_log_interval;
    // Specifies the maximum number of times to retry executing a rule if an update
    // conflict is detected.  If set to 0 then no retry attempts are made.
    uint32_t max_rule_retries;
};

} // namespace rules
} // namespace gaia
