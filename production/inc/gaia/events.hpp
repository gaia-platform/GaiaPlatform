/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace gaia
{
namespace db
{
namespace triggers
{

enum class event_type_t : uint32_t
{
    not_set = 0,
    // Row events.
    row_update = 1 << 0,
    row_insert = 1 << 1,
    row_delete = 1 << 2,
};

/**
 * Returns a string representation of the event type.
 */
const char* event_type_name(event_type_t event_type);

} // namespace triggers
} // namespace db
} // namespace gaia
