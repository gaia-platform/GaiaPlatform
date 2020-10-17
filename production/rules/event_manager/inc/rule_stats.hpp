/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <atomic>
#include <string>

namespace gaia
{
namespace rules
{

// Performance counters for rules. For the rules engine scheduler, a single
// instance of this class (scheduler_stats_t inherits from rules_stats_t) is
// created and these stats are updated for all rules. If indvidual rule statistics
// are enabled, then an instance of this class per rule is created.
class rule_stats_t
{
public:
    rule_stats_t();
    rule_stats_t(const char* a_rule_id);
    
    std::string rule_id;
    std::atomic<uint32_t> count_executed;
    std::atomic<uint32_t> count_scheduled;
    std::atomic<uint32_t> count_pending;
    std::atomic<uint32_t> count_abandoned;
    std::atomic<uint32_t> count_retries;
    std::atomic<uint32_t> count_exceptions;
    std::atomic<int64_t> total_rule_invocation_latency;
    std::atomic<int64_t> total_rule_execution_time;
    // These aggregates are only accessed via the stats
    // logger thread.
    int64_t max_rule_invocation_latency;
    int64_t max_rule_execution_time;

    void reset_counters();
    void add_rule_execution_time(const int64_t& duration);
    void add_rule_invocation_latency(const int64_t& duration);
    void log();
    void log(float load);
    void compute_averages(float& latency, float& execution_time);
};

} // rules
} // gaia
