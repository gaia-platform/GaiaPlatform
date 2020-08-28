/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "perf_timer.hpp"

namespace gaia
{
namespace rules
{

// This class record performance timings for the rules scheduler.  It
// is designed to allow enabling and disabling stats collection at
// rules engine initialization time.
class event_manager_stats_t 
{
public:
    operator bool() const 
    {
        return m_enabled;
    }
    
    void set_enabled(bool enabled) { m_enabled = enabled;}

    inline void save_time_point()
    {
        if (m_enabled) 
        { 
            m_time_point = gaia::common::perf_timer_t::get_time_point();
        }
    }

    inline std::chrono::high_resolution_clock::time_point get_saved_time_point()
    {
        return m_time_point;
    }

    inline void log_function_duration(std::function<void ()> fn, const char* fn_name)
    {
        if (m_enabled)
        {
            gaia::common::perf_timer_t::log_function_duration(fn, fn_name);
        }
        else
        {
            fn();
        }
    }

    inline void log_duration(const char* message)
    {
        if (m_enabled) 
        {
            gaia::common::perf_timer_t::log_duration(m_time_point, message);
        }
    }

private:
    bool m_enabled = false;
    std::chrono::high_resolution_clock::time_point m_time_point;
};

} // rules
} // gaia
