/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "rule_stats_manager.hpp"

using namespace gaia::common;
using namespace gaia::rules;
using namespace std;
using namespace std::chrono;

void rule_stats_manager_t::log_stats_thread_fn()
{
    std::chrono::seconds interval(m_log_interval);

    // Just keep running as long as the process is running.
    while (true)
    {
        std::this_thread::sleep_for(interval);
        log_scheduler_stats();
        log_rule_stats();
    }
}

void rule_stats_manager_t::inc_executed(const char* rule_id)
{
    m_scheduler_stats.count_executed++;
    if (m_rule_stats_enabled)
    {
        m_rule_stats_map[rule_id].count_executed++;
    }
}

void rule_stats_manager_t::inc_scheduled(const char* rule_id)
{
    m_scheduler_stats.count_scheduled++;
    if (m_rule_stats_enabled)
    {
        m_rule_stats_map[rule_id].count_scheduled++;
    }
}

void rule_stats_manager_t::inc_retries(const char* rule_id)
{
    m_scheduler_stats.count_retries++;
    if (m_rule_stats_enabled)
    {
        m_rule_stats_map[rule_id].count_retries++;
    }
}

void rule_stats_manager_t::inc_abandoned(const char* rule_id)
{
    m_scheduler_stats.count_abandoned++;
    if (m_rule_stats_enabled)
    {
        m_rule_stats_map[rule_id].count_abandoned++;
    }
}

void rule_stats_manager_t::inc_pending(const char* rule_id)
{
    m_scheduler_stats.count_pending++;
    if (m_rule_stats_enabled)
    {
        m_rule_stats_map[rule_id].count_pending++;
    }
}

void rule_stats_manager_t::inc_exceptions(const char* rule_id)
{
    m_scheduler_stats.count_exceptions++;
    if (m_rule_stats_enabled)
    {
        m_rule_stats_map[rule_id].count_exceptions++;
    }
}

void rule_stats_manager_t::add_rule_invocation_latency(const char* rule_id, std::chrono::steady_clock::time_point& start_time)
{
    int64_t duration = gaia::common::timer_t::get_duration(start_time);
    m_scheduler_stats.add_rule_invocation_latency(duration);
    if (m_rule_stats_enabled)
    {
        m_rule_stats_map[rule_id].add_rule_invocation_latency(duration);
    }
}

void rule_stats_manager_t::add_rule_execution_time(const char* rule_id, std::chrono::steady_clock::time_point& start_time)
{
    int64_t duration = gaia::common::timer_t::get_duration(start_time);
    m_scheduler_stats.add_rule_execution_time(duration);
    if (m_rule_stats_enabled)
    {
        m_rule_stats_map[rule_id].add_rule_execution_time(duration);
    }
}

void rule_stats_manager_t::add_thread_execution_time(std::chrono::steady_clock::time_point& start_time)
{
    int64_t duration = gaia::common::timer_t::get_duration(start_time);
    m_scheduler_stats.total_thread_execution_time += duration;
}

void rule_stats_manager_t::log_scheduler_stats()
{
    m_scheduler_stats.log(gaia::common::timer_t::get_duration(m_log_interval_start), 
        m_count_rule_worker_threads);
    m_log_interval_start = gaia::common::timer_t::get_time_point();
}

void rule_stats_manager_t::log_rule_stats()
{
    if (!m_rule_stats_enabled)
    {
        return;
    }

    for (auto& rule_it : m_rule_stats_map)
    {
        rule_it.second.log();
    }
}

void rule_stats_manager_t::initialize(bool enable_rule_stats, uint32_t count_threads, uint32_t stats_log_interval)
{
    m_rule_stats_enabled = enable_rule_stats;
    if (stats_log_interval)
    {
        m_log_interval = stats_log_interval;
        m_count_rule_worker_threads = count_threads;
        m_log_interval_start = gaia::common::timer_t::get_time_point();
        thread logger_thread = thread(&rule_stats_manager_t::log_stats_thread_fn, this);
        logger_thread.detach();
    }
}

void rule_stats_manager_t::insert_rule_stats(const char* rule_id)
{
    if (!m_rule_stats_enabled)
    {
        return;
    }

    unique_lock<mutex> lock(m_rule_stats_lock);
    m_rule_stats_map.try_emplace(rule_id, rule_id);
}

void rule_stats_manager_t::shutdown()
{
    if (m_log_interval)
    {
        // Be sure to write a row in the logger to capture any stats
        // we've gathered so far.  This may generate an extra statistics
        // log entry on shutdown which mainly is useful for testing
        // short-lived applications whose runtime is less than the 
        // statistics log interval.
        log_scheduler_stats();
        log_rule_stats();
    }
}
