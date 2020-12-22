/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "scheduler_stats.hpp"

#include "logger_internal.hpp"
#include "timer.hpp"

using namespace gaia::common;
using namespace std;
using namespace gaia::rules;

scheduler_stats_t::scheduler_stats_t(uint32_t log_interval_s, size_t count_worker_threads)
{
    // If no log interval specified then just set to 1ns to avoid a divide by zero error
    // if log is called.  Note that this should never happen in release code since a value of 0
    // for log_interval_s won't create the logging thread that calls the log method below.
    // However, for the test purposes, we allow calling this function directly and independently
    // from the rule_stats_manager_t class.
    m_log_interval_ns = log_interval_s ? static_cast<int64_t>(log_interval_s) * c_nanoseconds_per_s : c_nanoseconds_per_s;

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
    float thread_load
        = (static_cast<float>(total_thread_execution_time) / m_log_interval_ns * c_percent) / m_count_worker_threads;

    if (print_header)
    {
        gaia_log::re_stats().info(
            "{:->25}{: >6}{: >6}{: >6}{: >6}{: >6}{: >6}{: >13}{: >13}{: >13}{: >13}", "",
            "sched", "invoc", "pend", "aband", "retry", "excep", "avg lat", "max lat",
            "avg exec", "max exec");
    }

    rule_stats_t::log(thread_load);
    reset_counters();
}
