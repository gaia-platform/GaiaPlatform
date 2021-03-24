/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

enum class test_error_result_t : int8_t
{
    e_none = 0,
    e_tag_implicit_mismatch,
    e_tag_anchor_mismatch
};