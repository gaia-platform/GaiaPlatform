/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sys/mman.h>

#include "db_shared_data.hpp"
#include "memory_structures.hpp"
#include "memory_types.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

inline chunk_offset_t chunk_from_offset(gaia_offset_t offset);

inline slot_offset_t slot_from_offset(gaia_offset_t offset);

inline gaia_offset_t offset_from_chunk_and_slot(
    chunk_offset_t chunk_offset, slot_offset_t slot_offset);

inline void* page_address_from_offset(gaia_offset_t offset);

inline size_t calculate_allocation_size_in_slots(size_t allocation_size_in_bytes);

// Converts a slot offset to its bitmap index.
inline size_t slot_to_bit_index(slot_offset_t slot_offset);

// Converts a slot offset to its index within a bitmap word.
inline size_t slot_to_bit_index_in_word(slot_offset_t slot_offset);

// Converts a slot offset to the index of its bitmap word.
// The index of its data page within its chunk corresponds to the index of this
// bitmap word.
inline size_t slot_to_page_index(slot_offset_t slot_offset);

inline slot_offset_t page_index_to_first_slot_in_page(size_t page_index);

// This helper is only used for DEBUG allocation mode, where we allocate all
// objects at page granularity and write-protect allocated pages after all
// updates to the allocated object are complete.
inline void write_protect_allocation_page_for_offset(gaia_offset_t offset);

#include "memory_helpers.inc"

} // namespace memory_manager
} // namespace db
} // namespace gaia
