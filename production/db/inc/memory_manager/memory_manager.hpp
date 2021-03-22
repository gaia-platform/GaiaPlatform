/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <list>
#include <shared_mutex>

#include "base_memory_manager.hpp"
#include "chunk_manager.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

// A memory manager is used to manage the memory range allocated for our process.
// We allocate memory from this range in 4MB "chunks".
// Chunks are then used via a chunk manager to allocate memory in multiples of 64B allocation units.
class memory_manager_t : public base_memory_manager_t
{
public:
    memory_manager_t();

    // Tells the memory manager which memory area it should manage.
    //
    // All addresses will be offsets relative to the beginning of this block
    // and will be represented as address_offset_t.
    void manage(
        uint8_t* memory_address,
        size_t memory_size);

    // Allocates a new chunk of memory.
    address_offset_t allocate_chunk();

    // TODO: Remove this method after the client is updated to use allocate_chunk() and chunk managers.
    address_offset_t allocate(size_t size);

    void deallocate(address_offset_t memory_offset);

private:
    // As we keep allocating memory, the remaining contiguous available memory block
    // will keep shrinking. We'll use this offset to track the start of the block.
    address_offset_t m_next_allocation_offset;

private:
    size_t get_available_memory_size() const;

    // Internal method for allocating blocks of any size.
    address_offset_t allocate_internal(size_t size);

    void output_debugging_information(const std::string& context_description) const;
};

} // namespace memory_manager
} // namespace db
} // namespace gaia
