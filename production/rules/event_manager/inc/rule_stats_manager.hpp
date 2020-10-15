/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "timer.hpp"
#include "logger.hpp"
#include "scheduler_stats.hpp"

#include <map>
#include <thread>

namespace gaia
{
namespace rules
{

class rule_stats_manager_t
{
public:
    void initialize(bool enable_rule_stats, uint32_t count_threads, uint32_t stats_log_interval);
    void shutdown();
    void inc_executed(const char* rule_id);
    void inc_scheduled(const char* rule_id);
    void inc_retries(const char* rule_id);
    void inc_abandoned(const char* rule_id);
    void inc_pending(const char* rule_id);
    void inc_exceptions(const char* rule_id);
    void add_rule_invocation_latency(const char* rule_id, std::chrono::steady_clock::time_point& start_time);
    void add_rule_execution_time(const char* rule_id, std::chrono::steady_clock::time_point& start_time);
    void add_thread_execution_time(std::chrono::steady_clock::time_point& start_time);
    void insert_rule_stats(const char* rule_id);

private:
    void log_stats_thread_fn();
    void log_scheduler_stats();
    void log_rule_stats();

private:
    scheduler_stats_t m_scheduler_stats;
    uint32_t m_log_interval;
    uint32_t m_count_rule_worker_threads;
    std::chrono::steady_clock::time_point m_log_interval_start;
    std::map<string, rule_stats_t> m_rule_stats_map;
    mutex m_rule_stats_lock;
    bool m_rule_stats_enabled;
};

} // rules
} // gaia
