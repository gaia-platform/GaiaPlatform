/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "perf_timer.hpp"

using namespace gaia::common;

std::chrono::high_resolution_clock::time_point perf_timer_t::get_time_point()
{
    return std::chrono::high_resolution_clock::now();
}

int64_t perf_timer_t::get_duration(std::chrono::high_resolution_clock::time_point& start)
{
    auto finish = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
}

int64_t perf_timer_t::get_function_duration(std::function<void ()> fn)
{
    auto t1 = std::chrono::high_resolution_clock::now();
    fn();
    auto t2 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count();
}

// TODO[GAIAPLAT-318] Use an actual logging library when available and replace
// the console output below with actual log calls.
void perf_timer_t::log_duration(std::chrono::high_resolution_clock::time_point& start,
    const char* message)
{
    int64_t duration = get_duration(start);
    printf("[%s]: %0.2f us\n", message, perf_timer_t::ns_us(duration));
}


void perf_timer_t::log_function_duration(std::function<void ()> fn, const char * message)
{
    int64_t result = get_function_duration(fn);
    printf("[%s]: %0.2f us\n", message, perf_timer_t::ns_us(result));
}
