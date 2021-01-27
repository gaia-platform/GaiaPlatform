/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/common/timer.hpp"

using namespace std::chrono;

namespace gaia
{
namespace common
{

steady_clock::time_point timer_t::get_time_point()
{
    return steady_clock::now();
}

int64_t gaia::common::timer_t::get_duration(steady_clock::time_point& start_time)
{
    auto finish_time = steady_clock::now();
    return duration_cast<nanoseconds>(finish_time - start_time).count();
}

int64_t gaia::common::timer_t::get_function_duration(std::function<void()> fn)
{
    auto start_time = steady_clock::now();
    fn();
    auto finish_time = steady_clock::now();
    return duration_cast<nanoseconds>(finish_time - start_time).count();
}

// TODO[GAIAPLAT-318] Use an actual logging library when available and replace
// the console output below with actual log calls.
void gaia::common::timer_t::log_duration(steady_clock::time_point& start_time, const char* message)
{
    int64_t duration = get_duration(start_time);
    printf("[%s]: %0.2f us\n", message, timer_t::ns_to_us(duration));
}

void gaia::common::timer_t::log_function_duration(std::function<void()> fn, const char* message)
{
    int64_t result = get_function_duration(fn);
    printf("[%s]: %0.2f us\n", message, timer_t::ns_to_us(result));
}

} // namespace common
} // namespace gaia
