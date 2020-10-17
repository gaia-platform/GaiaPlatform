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
    std::atomic<int64_t> total_thread_execution_time;
    void initialize(uint32_t log_interval, size_t count_threads);
    void reset_counters();
    void log(bool print_header);

private:
    int64_t m_log_interval_ns;
    size_t m_count_worker_threads;
};

} // rules
} // gaia
