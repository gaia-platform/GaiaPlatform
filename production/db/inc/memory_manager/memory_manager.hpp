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
    memory_manager_t() = default;

    // Tells the memory manager which memory area it should manage.
    // Memory will be treated as a blank slate.
    //
    // All addresses will be offsets relative to the beginning of this block
    // and will be represented as address_offset_t.
    void initialize(
        uint8_t* memory_address,
        size_t memory_size);

    // Tells the memory manager which memory area it should manage.
    // Memory will be treated as having been initialized already.
    void load(
        uint8_t* memory_address,
        size_t memory_size);

    // Allocates the next available free chunk.
    chunk_offset_t allocate_chunk();

    // Updates the "allocated chunk bitmap" after allocating or deallocating a chunk.
    void update_chunk_allocation_status(chunk_offset_t chunk_offset, bool is_allocated);

private:
    // A pointer to our metadata information, stored inside the memory range that we manage.
    memory_manager_metadata_t* m_metadata;

private:
    void initialize_internal(
        uint8_t* memory_address,
        size_t memory_size,
        bool initialize_memory);

    // Get the amount of memory that has never been used yet.
    size_t get_unused_memory_size() const;

    // Internal method for making allocations from the unused portion of memory.
    chunk_offset_t allocate_unused_chunk();

    // Internal method for making allocations from deallocated memory.
    chunk_offset_t allocate_reused_chunk();

    void output_debugging_information(const std::string& context_description) const;
};

} // namespace memory_manager
} // namespace db
} // namespace gaia
