/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "rule_stats.hpp"

#include "logger.hpp"
#include "timer.hpp"

using namespace gaia::common;
using namespace std;
using namespace gaia::rules;

// Cumulative rule statistics include the thread worker load as the first agrument.  Example output is:
// [thread load:    16.00 %]   175   175     1     0     0     0     27.31 ms     86.68 ms     29.25 ms    131.57 ms
const char* rule_stats_t::c_cumulative_rule_stats
    = "[thread load: {:8.2f} %]{:6}{:6}{:6}{:6}{:6}{:6}{:10.2f} ms{:10.2f} ms{:10.2f} ms{:10.2f} ms";

// Individual rule statistics include the rule_id as the first argument.  Example output is:
// rule_field_phone_number      54    54     0     0     0     0     49.59 ms     86.68 ms      0.05 ms      1.96 ms
const char* rule_stats_t::c_rule_stats = "{: <25}{:6}{:6}{:6}{:6}{:6}{:6}{:10.2f} ms{:10.2f} ms{:10.2f} ms{:10.2f} ms";

rule_stats_t::rule_stats_t()
    : rule_stats_t(nullptr)
{
}

rule_stats_t::rule_stats_t(const char* a_rule_id)
{
    // Keep a local copy of the string because the thread that logs statistics
    // may outlive the rule subscription (and binding) that owns the name.
    if (a_rule_id)
    {
        rule_id = a_rule_id;
    }
    reset_counters();
}

void rule_stats_t::reset_counters()
{
    count_executed = 0;
    count_scheduled = 0;
    count_pending = 0;
    count_retries = 0;
    count_abandoned = 0;
    count_exceptions = 0;
    total_rule_invocation_latency = 0;
    total_rule_execution_time = 0;
    max_rule_execution_time = 0;
    max_rule_invocation_latency = 0;
}

void rule_stats_t::add_rule_invocation_latency(int64_t duration)
{
    if (duration > max_rule_invocation_latency)
    {
        max_rule_invocation_latency = duration;
    }
    total_rule_invocation_latency += duration;
}

void rule_stats_t::add_rule_execution_time(int64_t duration)
{
    if (duration > max_rule_execution_time)
    {
        max_rule_execution_time = duration;
    }
    total_rule_execution_time += duration;
}

template <typename T_param>
void rule_stats_t::log(
    gaia_log::logger_t& logger,
    const char* stats_format,
    T_param first_param)
{
    auto avg_latency = count_executed ? static_cast<float>(total_rule_invocation_latency / count_executed) : 0.0;
    auto avg_execution_time = count_executed ? static_cast<float>(total_rule_execution_time / count_executed) : 0.0;

    logger.info(
        stats_format, first_param, count_scheduled, count_executed, count_pending, count_abandoned, count_retries,
        count_exceptions, gaia::common::timer_t::ns_to_ms(avg_latency),
        gaia::common::timer_t::ns_to_ms(max_rule_invocation_latency),
        gaia::common::timer_t::ns_to_ms(avg_execution_time), gaia::common::timer_t::ns_to_ms(max_rule_execution_time));

    reset_counters();
}

// Only support template args of const char* and float so explicitly
// define the specializations here for linkage.
template void rule_stats_t::log(gaia_log::logger_t& logger, const char* format, const char* rule_id);
template void rule_stats_t::log(gaia_log::logger_t& logger, const char* format, float worker_thread_utilization);
