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

    // Allocates a new chunk of memory.
    address_offset_t allocate_chunk();

    // TODO: Remove this method after the client is updated to use allocate_chunk() and chunk managers.
    address_offset_t allocate(size_t size);

    void deallocate(address_offset_t memory_offset);

private:
    // A pointer to our metadata information, stored inside the memory range that we manage.
    memory_manager_metadata_t* m_metadata;

private:
    void initialize_internal(
        uint8_t* memory_address,
        size_t memory_size,
        bool initialize_memory);

    size_t get_available_memory_size() const;

    // Internal method for allocating blocks of any size.
    address_offset_t allocate_internal(size_t size);

    void output_debugging_information(const std::string& context_description) const;
};

} // namespace memory_manager
} // namespace db
} // namespace gaia
