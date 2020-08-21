/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include <functional>
#include <chrono>

class PerfTimer
{
public:
    // Always enabled, timer only
    PerfTimer(int64_t& timer, std::function<void ()> fn)
    {
        auto t1 = std::chrono::high_resolution_clock::now();
        fn();
        auto t2 = std::chrono::high_resolution_clock::now();
        timer = std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count();
    }

    // Only enables if s_enabled is true and prints debug
    // output.
    PerfTimer(const char* message, 
        std::function<void ()> fn)
    {
        if (!s_enabled)
        {
            fn();
            return;
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        fn();
        auto t2 = std::chrono::high_resolution_clock::now();
        auto result = std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count();
        printf("%s [%0.2f us]\n", message, PerfTimer::ns_us(result));
    }

    // Define in your app to turn on perf tracing.
    static bool s_enabled;

    // ns -> s
    static double ns_s(int64_t nanoseconds)
    {
        return nanoseconds / (double) (1e9);
    }

    // ns -> ms
    static double ns_ms(int64_t nanoseconds)
    {
        return nanoseconds / (double) (1e6);
    }

    // ns -> us
    static double ns_us(int64_t nanoseconds)
    {
        return nanoseconds / (double) (1e3);
    }
};