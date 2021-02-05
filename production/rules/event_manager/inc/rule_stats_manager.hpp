/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <condition_variable>

#include <atomic>
#include <map>
#include <mutex>
#include <thread>

#include "gaia_internal/common/timer.hpp"

#include "scheduler_stats.hpp"

namespace gaia
{
namespace rules
{

// Calculates statistics over a user-specified time interval (10s if not defined) and lazily writes them to a
// stats log file.
// TODO[GAIAPLAT-431] Consider gathering more useful statistics like median and percentile instead of just
// average and max calculations.
class rule_stats_manager_t
{
public:
    rule_stats_manager_t() = delete;
    rule_stats_manager_t(
        bool enable_rule_stats,
        size_t count_threads,
        uint32_t stats_log_interval);
    ~rule_stats_manager_t();

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

protected:
    // Protected so that tests can inherit from this class and have access
    // to the statistics.
    void log_stats_thread_fn(uint32_t count_threads);
    void log_stats();

    // Manages the total stats for all rules over each log interval
    scheduler_stats_t m_scheduler_stats;
    // Manages individual rule statistics.  The key is generated from the translation engine.
    std::map<std::string, rule_stats_t> m_rule_stats_map;
    // Write column headers every c_stats_group_size.
    inline static const uint8_t c_stats_group_size{40};
    // Tracks how many log rows have been written out.  We write a header initially and
    // then every c_stats_group_size thereafter.
    uint8_t m_count_entries_logged;

private:
    // Protects adding rules to the map above.
    std::mutex m_rule_stats_lock;
    // Individual rule stats are off by default.  Must be explicitly enabled by the user.
    bool m_rule_stats_enabled;
    // Management of stats logger thread
    std::thread m_logger_thread;
    std::mutex m_logging_lock;
    std::condition_variable m_exit_signal;
    bool m_exit;
};

} // namespace rules
} // namespace gaia
