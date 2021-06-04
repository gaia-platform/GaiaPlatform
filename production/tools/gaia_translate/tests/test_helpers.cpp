/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include <atomic>

#include "test_rulesets.hpp"

const int c_rule_execution_step_delay = 1;

namespace rule_test_helpers
{

// Wait for the rule to be called.
bool wait_for_rule(bool& rule_called)
{
    while (!rule_called)
    {
        usleep(c_rule_execution_step_delay);
    }
    return true;
}

// Wait for the the rule to be called 'count_times'.
bool wait_for_rule(std::atomic<int32_t>& count_times, int32_t expected_count)
{
    while (count_times < expected_count)
    {
        usleep(c_rule_execution_step_delay);
    }
    return true;
}

} // namespace rule_test_helpers
