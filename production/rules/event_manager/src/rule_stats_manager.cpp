/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "rule_stats_manager.hpp"

using namespace gaia::common;
using namespace gaia::rules;
using namespace std;
using namespace std::chrono;

const uint8_t rule_stats_manager_t::c_stats_group_size = 40;

void rule_stats_manager_t::log_stats_thread_fn(uint32_t log_interval)
{
    std::chrono::seconds interval(log_interval);

    // Just keep running as long as the process is running.
    while (true)
    {
        std::this_thread::sleep_for(interval);
        log_stats();
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

void rule_stats_manager_t::compute_rule_invocation_latency(const char* rule_id,
                                                           std::chrono::steady_clock::time_point& start_time)
{
    int64_t duration = gaia::common::timer_t::get_duration(start_time);
    m_scheduler_stats.add_rule_invocation_latency(duration);
    if (m_rule_stats_enabled)
    {
        m_rule_stats_map[rule_id].add_rule_invocation_latency(duration);
    }
}

void rule_stats_manager_t::compute_rule_execution_time(const char* rule_id,
                                                       std::chrono::steady_clock::time_point& start_time)
{
    int64_t duration = gaia::common::timer_t::get_duration(start_time);
    m_scheduler_stats.add_rule_execution_time(duration);
    if (m_rule_stats_enabled)
    {
        m_rule_stats_map[rule_id].add_rule_execution_time(duration);
    }
}

void rule_stats_manager_t::compute_thread_execution_time(std::chrono::steady_clock::time_point& start_time)
{
    int64_t duration = gaia::common::timer_t::get_duration(start_time);
    m_scheduler_stats.total_thread_execution_time += duration;
}

void rule_stats_manager_t::log_stats()
{
    if (m_count_entries_logged >= c_stats_group_size)
    {
        m_count_entries_logged = 0;
    }

    m_scheduler_stats.log(m_rule_stats_enabled || m_count_entries_logged == 0);
    m_count_entries_logged++;
    if (m_rule_stats_enabled)
    {
        for (auto& rule_it : m_rule_stats_map)
        {
            // Only log stats for a rule if it has been scheduled or invoked
            // in this interval.
            if (rule_it.second.count_scheduled || rule_it.second.count_executed)
            {
                rule_it.second.log();
                m_count_entries_logged++;
            }
        }
    }
}

void rule_stats_manager_t::initialize(bool enable_rule_stats, size_t count_threads, uint32_t stats_log_interval)
{
    m_rule_stats_enabled = enable_rule_stats;
    m_count_entries_logged = 0;
    if (stats_log_interval)
    {
        m_scheduler_stats.initialize(stats_log_interval, count_threads);
        thread logger_thread = thread(&rule_stats_manager_t::log_stats_thread_fn, this, stats_log_interval);
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
