/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "scheduler_stats.hpp"

#include "logger.hpp"
#include "timer.hpp"

using namespace gaia::common;
using namespace std;
using namespace gaia::rules;

void scheduler_stats_t::initialize(uint32_t log_interval_s, size_t count_worker_threads)
{
    m_log_interval_ns = static_cast<int64_t>(log_interval_s) * c_nanoseconds_per_s;

    // If 0 worker threads are specified then we only have a single thread in terms of our
    // thread utilization % calculation.
    m_count_worker_threads = count_worker_threads ? count_worker_threads : 1;
    reset_counters();
}

void scheduler_stats_t::reset_counters()
{
    total_thread_execution_time = 0;
    rule_stats_t::reset_counters();
}

// If print_header is true then the following row is output:
// ------------------------- sched invoc  pend aband retry excep      avg lat      max lat     avg exec     max exec
void scheduler_stats_t::log(bool print_header)
{
    // Estimate the CPU utilization time percentage of the threads in the thread pool.
    auto thread_load
        = static_cast<float>((total_thread_execution_time / m_log_interval_ns) * c_percent) / m_count_worker_threads;

    if (print_header)
    {
        gaia_log::rules_stats().info("{:->25}{: >6}{: >6}{: >6}{: >6}{: >6}{: >6}{: >13}{: >13}{: >13}{: >13}", "",
                                     "sched", "invoc", "pend", "aband", "retry", "excep", "avg lat", "max lat",
                                     "avg exec", "max exec");
    }

    rule_stats_t::log(thread_load);
    reset_counters();
}
