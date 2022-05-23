////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

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

int64_t gaia::common::timer_t::get_duration(steady_clock::time_point& start_time, steady_clock::time_point& finish_time)
{
    return duration_cast<nanoseconds>(finish_time - start_time).count();
}

int64_t gaia::common::timer_t::get_function_duration(std::function<void()> fn)
{
    auto start_time = steady_clock::now();
    fn();
    auto finish_time = steady_clock::now();
    return duration_cast<nanoseconds>(finish_time - start_time).count();
}

// TODO: Consider using a logging library to replace the printf statements below.
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
