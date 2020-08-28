/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include <functional>
#include <chrono>

// Simple timer utility classes for doing
// profiling or collecting statistics.
namespace gaia {
namespace common {

class perf_timer_t
{
public:
    // Create a new time_point to measure from
    static std::chrono::high_resolution_clock::time_point get_time_point();

    // Return a duration calculated as now() - start
    static int64_t get_duration(std::chrono::high_resolution_clock::time_point& start);

    // Convenience method to calculate the duration and print out the result
    static void log_duration(std::chrono::high_resolution_clock::time_point& start, const char* message);

    // Time the function and return result in nanoseconds.
    static int64_t get_function_duration(std::function<void ()> fn);

    // Time the function, and output a message to the console
    // for the function exclusive time.
    static void log_function_duration(std::function<void ()> fn, const char* function_name);

    // ns -> s
    static double ns_s(int64_t nanoseconds) { return nanoseconds / (double) (1e9); }
    // ns -> ms
    static double ns_ms(int64_t nanoseconds) { return nanoseconds / (double) (1e6); }
    // ns -> us
    static double ns_us(int64_t nanoseconds) { return nanoseconds / (double) (1e3); }
};

}
}