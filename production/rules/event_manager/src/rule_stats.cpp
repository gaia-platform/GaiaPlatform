////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "rule_stats.hpp"

#include "gaia_internal/common/logger.hpp"
#include "gaia_internal/common/timer.hpp"

using namespace gaia::common;
using namespace std;
using namespace gaia::rules;

rule_stats_t::rule_stats_t()
{
    reset_counters();
}

rule_stats_t::rule_stats_t(const std::string& a_rule_id)
    : rule_stats_t()
{
    // Keep a local copy of the string because the thread that logs statistics
    // may outlive the rule subscription (and binding) that owns the name.
    if (!a_rule_id.empty())
    {
        rule_id = a_rule_id;
        // Truncate long rule ids for pretty formatting.
        if (rule_id.length() > c_max_rule_id_len)
        {
            truncated_rule_id = rule_id.substr(0, c_max_rule_id_len);
            truncated_rule_id[c_max_rule_id_len - 1] = c_truncate_char;
        }
        else
        {
            truncated_rule_id = rule_id;
        }
    }
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

void rule_stats_t::log_cumulative(float thread_utilization)
{
    auto avg_latency = count_executed ? static_cast<float>(total_rule_invocation_latency / count_executed) : 0.0;
    auto avg_execution_time = count_executed ? static_cast<float>(total_rule_execution_time / count_executed) : 0.0;

    gaia_log::rules_stats().info(
        c_cumulative_stats_format, c_thread_load, thread_utilization, c_thread_load_padding,
        count_scheduled, count_executed, count_pending, count_abandoned, count_retries, count_exceptions,
        gaia::common::timer_t::ns_to_ms(avg_latency),
        gaia::common::timer_t::ns_to_ms(max_rule_invocation_latency),
        gaia::common::timer_t::ns_to_ms(avg_execution_time),
        gaia::common::timer_t::ns_to_ms(max_rule_execution_time));

    reset_counters();
}

void rule_stats_t::log_individual()
{
    auto avg_latency = count_executed ? static_cast<float>(total_rule_invocation_latency / count_executed) : 0.0;
    auto avg_execution_time = count_executed ? static_cast<float>(total_rule_execution_time / count_executed) : 0.0;

    gaia_log::rules_stats().info(
        c_individual_stats_format, truncated_rule_id.c_str(), c_max_rule_id_len,
        count_scheduled, count_executed, count_pending, count_abandoned, count_retries, count_exceptions,
        gaia::common::timer_t::ns_to_ms(avg_latency),
        gaia::common::timer_t::ns_to_ms(max_rule_invocation_latency),
        gaia::common::timer_t::ns_to_ms(avg_execution_time),
        gaia::common::timer_t::ns_to_ms(max_rule_execution_time));

    reset_counters();
}
