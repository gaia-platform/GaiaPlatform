/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <atomic>

constexpr char c_status_pending[] = "pending";
constexpr char c_status_eligible[] = "eligible";
constexpr char c_grade_none[] = "";
constexpr char c_grade_a[] = "A";
constexpr char c_grade_b[] = "B";
constexpr char c_grade_c[] = "C";
constexpr char c_grade_d[] = "D";

enum class test_error_result_t
{
    e_none = 0,
    e_tag_field_mismatch,
    e_tag_implicit_mismatch,
    e_tag_anchor_mismatch
};

// Test helpers
namespace rule_test_helpers
{
bool wait_for_rule(bool& rule_called);
bool wait_for_rule(std::atomic<int32_t>& count_times, int32_t expected_count);
} // namespace rule_test_helpers