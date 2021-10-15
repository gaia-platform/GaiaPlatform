/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

#include "gaia_internal/db/gaia_ptr.hpp"

typedef std::vector<gaia::common::gaia_id_t> type_vector;

std::stringstream gaia_db_extract(gaia::common::gaia_id_t, gaia::common::gaia_id_t, bool, bool, bool, int, type_vector);
