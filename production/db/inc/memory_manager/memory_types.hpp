/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

namespace gaia
{
namespace db
{
namespace memory_manager
{

// An opaque value associated with a memory allocation.
// The stack allocator just stores these in its allocation records.
typedef size_t slot_id_t;

// For representing offsets from a base memory address.
typedef size_t address_offset_t;

}
}
}
