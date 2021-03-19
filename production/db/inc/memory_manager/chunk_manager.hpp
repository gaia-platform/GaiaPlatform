/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#include "base_memory_manager.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

class chunk_manager_t : public base_memory_manager_t
{
public:
    chunk_manager_t() = default;

    // Initialize the chunk_manager_t with a specific memory chunk from which to allocate memory.
    // The start of the buffer is specified as an offset from a base address.
    void initialize(
        uint8_t* base_memory_address,
        address_offset_t memory_offset);

    // Load a specific memory chunk from which memory has already been allocated.
    // This method can be used to read the allocations made by another chunk manager instance.
    // The start of the buffer is specified as an offset from a base address.
    void load(
        uint8_t* base_memory_address,
        address_offset_t memory_offset);

    // Allocate a new memory block inside our managed chunk.
    address_offset_t allocate(
        size_t memory_size) const;

    // Mark all allocations as committed.
    void commit() const;

    // Rollback all allocations made since last commit.
    void rollback() const;

    // Public, so it can get called by the memory manager.
    void output_debugging_information(const std::string& context_description) const;

private:
    // A pointer to our metadata information, stored inside the memory chunk that we manage.
    chunk_manager_metadata_t* m_metadata;

private:
    // Initialize the chunk_manager_t with a specific memory chunk from which to allocate memory.
    // The start of the buffer is specified as an offset from a base address.
    void initialize_internal(
        uint8_t* base_memory_address,
        address_offset_t memory_offset,
        bool initialize_memory);
};

} // namespace memory_manager
} // namespace db
} // namespace gaia
