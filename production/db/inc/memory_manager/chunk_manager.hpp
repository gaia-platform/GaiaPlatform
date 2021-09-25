/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "base_memory_manager.hpp"

namespace gaia
{
namespace db
{
namespace memory_manager
{

// A chunk manager is used to allocate memory from a 4MB memory "chunk".
// Memory is allocated in 64B "slots".
class chunk_manager_t
{
public:
    chunk_manager_t()
        : m_chunk_offset(c_invalid_chunk_offset), m_metadata(nullptr)
    {
    }

    inline chunk_offset_t chunk_offset()
    {
        return m_chunk_offset;
    }

    // Initialize the chunk manager with an empty chunk.
    inline bool initialized()
    {
        return (m_chunk_offset != c_invalid_chunk_offset);
    }

    // Initialize the chunk manager with an empty chunk.
    void initialize(chunk_offset_t chunk_offset);

    // Initialize the chunk manager with a used chunk.
    void load(chunk_offset_t chunk_offset);

    // Takes ownership of the current chunk from the chunk manager.
    chunk_offset_t release();

    // Allocate a new memory block inside our managed chunk.
    gaia_offset_t allocate(size_t allocation_slots_size);

    // Mark the slot corresponding to this offset as deallocated,
    // and optionally free any pages left empty as a result.
    // It is illegal to call this method twice on the same offset
    // (unless the containing chunk was reused, of course).
    void deallocate(gaia_offset_t offset);

    // This should be called by the owner immediately *after* it transitions its
    // chunk to RETIRED state, so it can deallocate the chunk if it is already
    // empty (otherwise the chunk will never be deallocated because GC tasks
    // were unable to deallocate it while it was owned, and there are no
    // remaining object deallocations to trigger a chunk deallocation).
    // Additionally, it should be called by a GC task when it has completed all
    // object deallocations within a chunk, to determine whether it should
    // deallocate the chunk as well.
    bool is_empty();

    // This returns the last allocation recorded in this chunk, so a crashed
    // session can reconcile its txn log with allocation metadata. For
    // crash-consistency, we ensure that the "last allocation metadata" is
    // reconciled before returning the last allocation.
    gaia_offset_t last_allocated_offset();

    // Decommits all unused physical pages in the managed chunk.
    void decommit_unused_physical_pages();

private:
    // The offset of the managed chunk within the global chunk array.
    chunk_offset_t m_chunk_offset;

    // A pointer to our metadata information, stored inside the memory chunk that we manage.
    chunk_manager_metadata_t* m_metadata;

private:
    void initialize_internal(chunk_offset_t chunk_offset, bool initialize_memory);

    // Checks whether a slot is currently allocated in the slot bitmaps.
    bool is_slot_allocated(slot_offset_t slot_offset) const;

    // Marks a slot in the allocation bitmap.
    void mark_slot_allocated(slot_offset_t slot_offset);

    // Marks a slot in the deallocation bitmap.
    void mark_slot_deallocated(slot_offset_t slot_offset);

    bool is_page_empty(size_t page_offset);

    void decommit_page_at_index(size_t page_index);

    void output_debugging_information(const std::string& context_description) const;
};

} // namespace memory_manager
} // namespace db
} // namespace gaia
