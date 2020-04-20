/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include <functional>
#include <chrono>

class performance_timer
{
public:
    performance_timer(int64_t& result_in_ns, 
        std::function<void ()> fn)
    {
        auto t1 = std::chrono::high_resolution_clock::now();
        fn();
        auto t2 = std::chrono::high_resolution_clock::now();
        result_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count();
    }

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
