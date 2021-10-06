/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "rule_stats.hpp"

namespace gaia
{
namespace rules
{

// Performance counters for the rules engine scheduler.
// This class tracks the rule worker thread pool usage in addition
// to the same set of statistics used for individual rules.  In this
// context however, the counters apply to all rules, not just
// an individual rule.
class scheduler_stats_t : public rule_stats_t
{
public:
    scheduler_stats_t() = delete;
    scheduler_stats_t(uint32_t log_interval, size_t count_threads);

    std::atomic<int64_t> total_thread_execution_time;

    void reset_counters();
    void log(bool print_header);

private:
    static const int c_percent = 100;
    static const int c_nanoseconds_per_s = 1e9;
    int64_t m_log_interval_ns;
    size_t m_count_worker_threads;
};

} // namespace rules
} // namespace gaia
