/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

#include "gaia_internal/db/gaia_ptr.hpp"

constexpr gaia::common::gaia_id_t c_start_after_none = -1;
constexpr uint32_t c_row_limit_unlimited = -1;

typedef std::vector<gaia::common::gaia_id_t> type_vector;

std::string gaia_db_extract(std::string, std::string, gaia::common::gaia_id_t, uint32_t);
