/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <map>
#include <thread>

#include "logger.hpp"
#include "scheduler_stats.hpp"
#include "timer.hpp"

namespace gaia
{
namespace rules
{

class rule_stats_manager_t
{
public:
    void initialize(bool enable_rule_stats, size_t count_threads, uint32_t stats_log_interval);
    void inc_executed(const char* rule_id);
    void inc_scheduled(const char* rule_id);
    void inc_retries(const char* rule_id);
    void inc_abandoned(const char* rule_id);
    void inc_pending(const char* rule_id);
    void inc_exceptions(const char* rule_id);
    void compute_rule_invocation_latency(const char* rule_id, std::chrono::steady_clock::time_point& start_time);
    void compute_rule_execution_time(const char* rule_id, std::chrono::steady_clock::time_point& start_time);
    void compute_thread_execution_time(std::chrono::steady_clock::time_point& start_time);
    void insert_rule_stats(const char* rule_id);

private:
    void log_stats_thread_fn(uint32_t count_threads);
    void log_stats();

private:
    // Write column headers every c_stats_group_size.
    static const uint8_t c_stats_group_size;

    // Manages the total stats for all rules over each log interval
    scheduler_stats_t m_scheduler_stats;
    // Manages individual rule statistics.  The key is generated from the translation engine.
    std::map<string, rule_stats_t> m_rule_stats_map;
    // Protects adding rules to the map above.
    mutex m_rule_stats_lock;
    // Individual rule stats are off by default.  Must be explicitly enabled by the user.
    bool m_rule_stats_enabled;
    // Tracks how many log rows have been written out.  We write a header initially and
    // then every c_stats_group_size thereafter.
    uint8_t m_count_entries_logged;
};

} // namespace rules
} // namespace gaia
