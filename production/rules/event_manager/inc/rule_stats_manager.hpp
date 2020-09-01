/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "perf_timer.hpp"

#include <memory>

namespace gaia
{
namespace rules
{

struct rule_stats_t 
{
    const char* tag;
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point enqueue_time;
    std::chrono::high_resolution_clock::time_point before_invoke;
    std::chrono::high_resolution_clock::time_point before_rule;
    std::chrono::high_resolution_clock::time_point after_rule;
    std::chrono::high_resolution_clock::time_point after_invoke;
};


// This static class uses the perf timer to take time points
// and calculate durations.
class rule_stats_manager_t 
{
public:
    static bool s_enabled;
    static const char* s_rule_tag;
    static const char* s_log_event_tag;

    static std::shared_ptr<rule_stats_t> create_rule_stats(std::chrono::high_resolution_clock::time_point& start_time,
        const char* tag)
    {
        if (s_enabled)
        {
            auto ptr = std::make_shared<rule_stats_t>();
            ptr->tag = tag;
            ptr->start_time = start_time;
            return ptr;
        }
        else
        {
            return nullptr;
        }
    }

    static void record_enqueue_time(std::shared_ptr<rule_stats_t>& stats_ptr)
    {
        if (s_enabled)
        {
            stats_ptr->enqueue_time = gaia::common::perf_timer_t::get_time_point();
        }
    }

    static void record_invoke_time(std::shared_ptr<rule_stats_t>& stats_ptr, std::function<void()>fn)
    {
        if (s_enabled)
        {
            stats_ptr->before_invoke = gaia::common::perf_timer_t::get_time_point();
            fn();
            stats_ptr->after_invoke = gaia::common::perf_timer_t::get_time_point();
        }
        else
        {
            fn();
        }
    }

    static void record_rule_fn_time(std::shared_ptr<rule_stats_t>& stats_ptr, std::function<void()>fn)
    {
        if (s_enabled)
        {
            stats_ptr->before_rule = gaia::common::perf_timer_t::get_time_point();
            fn();
            stats_ptr->after_rule = gaia::common::perf_timer_t::get_time_point();
        }
        else
        {
            fn();
        }
    }

    static void log(std::shared_ptr<rule_stats_t>& stats_ptr);
};

} // rules
} // gaia
