/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

constexpr uint64_t c_start_after_none = 0;
constexpr uint32_t c_row_limit_unlimited = -1;

std::string gaia_db_extract(std::string, std::string, uint64_t, uint32_t);
