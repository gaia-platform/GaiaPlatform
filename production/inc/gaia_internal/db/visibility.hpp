/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "db_helpers.hpp"

#pragma once

namespace gaia
{
namespace db
{

inline bool is_visible(gaia::db::gaia_locator_t locator, gaia::db::gaia_offset_t offset)
{
    return locator_exists(locator) && locator_to_offset(locator) == offset;
}

} // namespace db
} // namespace gaia
