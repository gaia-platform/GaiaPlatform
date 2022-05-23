////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

#include "gaia_internal/db/gaia_ptr.hpp"

typedef std::vector<gaia::common::gaia_id_t> type_vector;

std::string gaia_dump(gaia::common::gaia_id_t, gaia::common::gaia_id_t, bool, bool, bool, int, type_vector);
