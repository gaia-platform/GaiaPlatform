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
    scheduler_stats_t();
    std::atomic<int64_t> total_thread_execution_time;

    void reset_counters();
    void log(const int64_t& interval, uint32_t count_threads);
};

} // rules
} // gaia
