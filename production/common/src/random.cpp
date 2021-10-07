/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/common/random.hpp"

#include <iostream>
#include <random>

namespace gaia
{
namespace common
{

constexpr char static c_alphanum[] = "0123456789"
                                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz";

// The actual length of c_alphanum, excluding the \0.
constexpr size_t c_alphanum_len = sizeof(c_alphanum) - 1;

int gen_random_num(int low, int high)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(low, high);

    return dist(rng);
}

std::string gen_random_str(size_t len)
{
    std::string random_str;
    random_str.reserve(len);

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<std::string::size_type> dis;

    while (len--)
    {
        random_str += c_alphanum[dis(gen) % c_alphanum_len];
    }
    return random_str;
}

} // namespace common
} // namespace gaia
