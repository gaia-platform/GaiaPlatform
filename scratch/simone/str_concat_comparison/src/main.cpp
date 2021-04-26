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

    int total_size = 0;

    std::chrono::steady_clock::time_point start;

    std::cout << "Num Iter: " << num_iter << std::endl;

    start = gaia_timer_t::get_time_point();
    for (int i = 0; i < num_iter; i++)
    {
        std::string final = fmt::format("{}_{}_{}_{}", i, str1, i, str2);
        sizes[i] = final.size();
        // std::cout << final << std::endl;
    }
    gaia_timer_t::log_duration(start, "fmt");

    total_size = 0;
    for (int i = 0; i < num_iter; i++)
    {
        total_size += sizes[i];
    }
    std::cout << "Concatenated " << total_size << " characters" << std::endl;

    start = gaia_timer_t::get_time_point();
    for (int i = 0; i < num_iter; i++)
    {
        std::ostringstream ss;
        ss << i << "_" << str1 << "_" << i << "_" << str2;
        std::string final = ss.str();
        sizes[i] = final.size();
        // std::cout << final << std::endl;
    }
    gaia_timer_t::log_duration(start, "ostringstream");

    total_size = 0;
    for (int i = 0; i < num_iter; i++)
    {
        total_size += sizes[i];
    }
    std::cout << "Concatenated " << total_size << " characters" << std::endl;

    start = gaia_timer_t::get_time_point();
    for (int i = 0; i < num_iter; i++)
    {
        std::stringstream ss;
        ss << i << "_" << str1 << "_" << i << "_" << str2;
        std::string final = ss.str();
        sizes[i] = final.size();
        // std::cout << final << std::endl;
    }
    gaia_timer_t::log_duration(start, "stringstream");

    total_size = 0;
    for (int i = 0; i < num_iter; i++)
    {
        total_size += sizes[i];
    }
    std::cout << "Concatenated " << total_size << " characters" << std::endl;

    start = gaia_timer_t::get_time_point();
    for (int i = 0; i < num_iter; i++)
    {
        std::string final = std::to_string(i) + "_" + str1 + "_" + std::to_string(i) + "_" + str2;
        sizes[i] = final.size();
        // std::cout << final << std::endl;
    }
    gaia_timer_t::log_duration(start, "string concat");

    total_size = 0;
    for (int i = 0; i < num_iter; i++)
    {
        total_size += sizes[i];
    }
    std::cout << "Concatenated " << total_size << " characters" << std::endl;
}
