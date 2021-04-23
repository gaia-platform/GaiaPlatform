/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include <string>

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "timer.hpp"

using gaia_timer_t = gaia::common::timer_t;

int main(int argc, char* argv[])
{
    std::string str1 = "Hello World";
    const char* str2 = "Ciao Mondo";

    int num_iter = 10000;

    int sizes[num_iter];

    std::chrono::steady_clock::time_point start;

    std::cout << "Num Iter: " << num_iter << std::endl;

    start = gaia_timer_t::get_time_point();
    for (int i = 0; i < num_iter; i++)
    {
        std::string final = fmt::format("{}_{}_{}", str1, str2, i);
        sizes[i] = final.size();
    }
    gaia_timer_t::log_duration(start, "fmt");

    start = gaia_timer_t::get_time_point();
    for (int i = 0; i < num_iter; i++)
    {
        std::ostringstream ss;
        ss << str1 << "_" << str2 << "_" << i;
        std::string final = ss.str();
        sizes[i] = final.size();
    }
    gaia_timer_t::log_duration(start, "ostringstream");

    start = gaia_timer_t::get_time_point();
    for (int i = 0; i < num_iter; i++)
    {
        std::stringstream ss;
        ss << str1 << "_" << str2 << "_" << i;
        std::string final = ss.str();
        sizes[i] = final.size();
    }
    gaia_timer_t::log_duration(start, "stringstream");

    start = gaia_timer_t::get_time_point();
    for (int i = 0; i < num_iter; i++)
    {
        std::string final = str1 + "_" + str2 + "_" + std::to_string(i);
        sizes[i] = final.size();
    }
    gaia_timer_t::log_duration(start, "string concat");
}
