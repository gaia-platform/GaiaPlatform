////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "scheduler_stats.hpp"

#include "gaia_internal/common/logger.hpp"
#include "gaia_internal/common/timer.hpp"

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

void scheduler_stats_t::log(bool print_header)
{
    // Estimate the CPU utilization time percentage of the threads in the thread pool.
    float thread_load
        = (static_cast<float>(total_thread_execution_time) / m_log_interval_ns * c_percent) / m_count_worker_threads;

    if (print_header)
    {
        gaia_log::rules_stats().info(
            c_header_format, "", c_max_rule_id_len, c_scheduled_column, c_invoked_column, c_pending_column,
            c_abandoned_column, c_retries_column, c_exceptions_column, c_avg_latency_column, c_max_latency_column,
            c_avg_execution_column, c_max_execution_column);
    }

    rule_stats_t::log_cumulative(thread_load);
    reset_counters();
}
