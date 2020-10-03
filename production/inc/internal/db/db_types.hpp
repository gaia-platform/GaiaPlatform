/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace gaia {
namespace db {

/**
 * The type of a Gaia transaction id.
 */
typedef uint64_t gaia_xid_t;
/**
 * The type of a Gaia locator id.
 */
typedef uint64_t gaia_locator_t;

/**
 * The type of a Gaia data offset.
 */
typedef uint64_t gaia_offset_t;

}  // namespace db
}  // namespace gaia
