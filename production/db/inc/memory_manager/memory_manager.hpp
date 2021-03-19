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

    // TODO: Remove this method after client is updated to use allocate_chunk() and chunk managers.
    address_offset_t allocate(size_t size);

    void deallocate(address_offset_t memory_offset);

private:
    // As we keep allocating memory, the remaining contiguous available memory block
    // will keep shrinking. We'll use this offset to track the start of the block.
    address_offset_t m_next_allocation_offset;

private:
    size_t get_main_memory_available_size() const;

    // Attempt to allocate a chunk from our main memory block.
    address_offset_t allocate_chunk_from_main_memory();

    // Attempt to allocate a chunk from one of the already allocated and freed chunks.
    address_offset_t allocate_chunk_from_freed_memory();

    // Internal method for allocating blocks of any size from main memory.
    address_offset_t allocate_from_main_memory(size_t size);

    void output_debugging_information(const std::string& context_description) const;
};

} // namespace memory_manager
} // namespace db
} // namespace gaia
