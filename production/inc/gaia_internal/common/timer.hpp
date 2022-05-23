////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <cstdio>

#include <chrono>
#include <functional>

// Simple timer utility classes for doing profiling.
// This class uses steady_clock based on guidance from
// https://en.cppreference.com/w/cpp/chrono/high_resolution_clock.
// Namely, use steady_clock for duration measurements instead of
// high_resolution_clock.
namespace gaia
{
namespace common
{

class timer_t
{
public:
    // Create a new time_point to measure from
    static std::chrono::steady_clock::time_point get_time_point();

    // Return a duration calculated as now() - start
    static int64_t get_duration(std::chrono::steady_clock::time_point& start_time);

    // Return a duration calculated as end - start
    static int64_t get_duration(std::chrono::steady_clock::time_point& start_time, std::chrono::steady_clock::time_point& finish_time);

    // Convenience method to calculate the duration and print out the result
    static void log_duration(std::chrono::steady_clock::time_point& start_time, const char* message);

    // Time the function and return result in nanoseconds.
    static int64_t get_function_duration(std::function<void()> fn);

    // Time the function and output a message to the console.
    static void log_function_duration(std::function<void()> fn, const char* function_name);

    // ns -> s
    static double ns_to_s(int64_t nanoseconds)
    {
        return nanoseconds / (double)(1e9);
    }

    // ns -> ms
    static double ns_to_ms(int64_t nanoseconds)
    {
        return nanoseconds / (double)(1e6);
    }

    // ns -> us
    static double ns_to_us(int64_t nanoseconds)
    {
        return nanoseconds / (double)(1e3);
    }
};

// Convenience class for enabling and disabling performance measurements
// at runtime.
class optional_timer_t
{
public:
    bool is_enabled()
    {
        return m_enabled;
    }

    void set_enabled(bool enabled)
    {
        m_enabled = enabled;
    }

    std::chrono::steady_clock::time_point get_time_point()
    {
        if (m_enabled)
        {
            return gaia::common::timer_t::get_time_point();
        }

        return std::chrono::steady_clock::time_point::min();
    }

    int64_t get_duration(std::chrono::steady_clock::time_point& start_time)
    {
        if (m_enabled)
        {
            return gaia::common::timer_t::get_duration(start_time);
        }

        return 0;
    }

    void log_duration(std::chrono::steady_clock::time_point& start_time, const char* message)
    {
        if (m_enabled)
        {
            gaia::common::timer_t::log_duration(start_time, message);
        }
    }

    // Time the function and return result in nanoseconds.
    int64_t get_function_duration(std::function<void()> fn)
    {
        if (m_enabled)
        {
            return gaia::common::timer_t::get_function_duration(fn);
        }

        fn();
        return 0;
    }

    void log_function_duration(std::function<void()> fn, const char* fn_name)
    {
        if (m_enabled)
        {
            gaia::common::timer_t::log_function_duration(fn, fn_name);
        }
        else
        {
            fn();
        }
    }

private:
    bool m_enabled = false;
};

} // namespace common
} // namespace gaia
