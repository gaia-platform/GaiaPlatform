/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "scheduler_stats.hpp"
#include "timer.hpp"
#include "logger.hpp"

using namespace gaia::common;
using namespace std;
using namespace gaia::rules;

scheduler_stats_t::scheduler_stats_t()
{
    reset_counters();
}

void scheduler_stats_t::reset_counters()
{
    total_thread_execution_time = 0;
    rule_stats_t::reset_counters();
}

void scheduler_stats_t::log(const int64_t& interval, uint32_t count_threads)
{
    float avg_latency;
    float avg_execution_time;
    compute_averages(avg_latency, avg_execution_time);

    // Estimate the CPU utilization time of the threads in the thread pool
    float utilization_time = ((total_thread_execution_time / interval) * 100) / count_threads;

    gaia_log::rules_stats().info("thread utilization: {:02.2f}%, scheduled: {}, executed: {}, pending: {}, abandoned: {}, retries: {}, exceptions {}, "
        "average latency: {:03.2f} ms, max latency: {:03.2f} ms, average execution time: {:03.2f} ms, max execution time: {:03.2f} ms.",
        utilization_time,
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
