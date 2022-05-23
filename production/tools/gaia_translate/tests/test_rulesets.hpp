////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <atomic>

constexpr char c_status_pending[] = "pending";
constexpr char c_status_eligible[] = "eligible";
constexpr float c_grade_none = -1.0;
constexpr float c_grade_plus = 0.5;
constexpr float c_grade_a = 4.0;
constexpr float c_grade_b = 3.0;
constexpr float c_grade_c = 2.0;
constexpr float c_grade_d = 1.0;
constexpr float c_grade_f = 0.0;

enum class test_error_result_t
{
    e_none = 0,
    e_tag_field_mismatch,
    e_tag_implicit_mismatch,
    e_tag_anchor_mismatch
};
