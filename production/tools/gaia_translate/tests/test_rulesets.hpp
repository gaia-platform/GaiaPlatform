/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

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
