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

rule_stats_t::rule_stats_t()
:  rule_stats_t(nullptr)
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

void rule_stats_t::add_rule_invocation_latency(const int64_t& duration)
{
    if (duration > max_rule_invocation_latency)
    {
        max_rule_invocation_latency = duration;
    }
    total_rule_invocation_latency += duration;
}

void rule_stats_t::add_rule_execution_time(const int64_t& duration)
{
    // TODO: do this in an efficient and threadsafe manner?
    if (duration > max_rule_execution_time)
    {
        max_rule_execution_time = duration;
    }
    total_rule_execution_time += duration;
}

void rule_stats_t::compute_averages(float& avg_latency, float& avg_execution_time)
{
    avg_latency = count_scheduled ? (total_rule_invocation_latency / count_scheduled) : 0;
    avg_execution_time = count_executed ? (total_rule_execution_time / count_executed) : 0;
}

//
// This log overload takes care of writing the rollup statistics
// TODO: combine these two overloads into one?
//
void rule_stats_t::log(float load)
{
    float avg_latency;
    float avg_execution_time;
    compute_averages(avg_latency, avg_execution_time);

    gaia_log::rules_stats().info("[thread load: {:8.2f} %]{:6}{:6}{:6}{:6}{:6}{:6}{:10.2f} ms{:10.2f} ms{:10.2f} ms{:10.2f} ms",
        load,
        count_scheduled,
        count_executed,
        count_pending,
        count_abandoned,
        count_retries,
        count_exceptions,
        gaia::common::timer_t::ns_to_ms(avg_latency),
        gaia::common::timer_t::ns_to_ms(max_rule_invocation_latency),
        gaia::common::timer_t::ns_to_ms(avg_execution_time),
        gaia::common::timer_t::ns_to_ms(max_rule_execution_time));
}

void rule_stats_t::log()
{
    float avg_latency;
    float avg_execution_time;
    compute_averages(avg_latency, avg_execution_time);

    gaia_log::rules_stats().info("{: <25}{:6}{:6}{:6}{:6}{:6}{:6}{:10.2f} ms{:10.2f} ms{:10.2f} ms{:10.2f} ms",
        rule_id,
        count_scheduled,
        count_executed,
        count_pending,
        count_abandoned,
        count_retries,
        count_exceptions,
        gaia::common::timer_t::ns_to_ms(avg_latency),
        gaia::common::timer_t::ns_to_ms(max_rule_invocation_latency),
        gaia::common::timer_t::ns_to_ms(avg_execution_time),
        gaia::common::timer_t::ns_to_ms(max_rule_execution_time));

    reset_counters();
}
