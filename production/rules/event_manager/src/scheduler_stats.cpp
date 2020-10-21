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
    m_log_interval_ns = log_interval_s * 1e9;
    m_count_worker_threads = count_worker_threads;
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
    // Estimate the CPU utilization time of the threads in the thread pool
    float thread_load = ((total_thread_execution_time / m_log_interval_ns) * 100) / m_count_worker_threads;

    if (print_header)
    {
        gaia_log::rules_stats().info("{:->25}{: >6}{: >6}{: >6}{: >6}{: >6}{: >6}{: >13}{: >13}{: >13}{: >13}",
                                     "", "sched", "invoc", "pend", "aband", "retry", "excep", "avg lat", "max lat", "avg exec", "max exec");
    }

    rule_stats_t::log(thread_load);
    reset_counters();
}
