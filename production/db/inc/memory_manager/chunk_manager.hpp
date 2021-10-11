/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>

#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "memory_structures.hpp"

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

    inline chunk_offset_t chunk_offset() const
    {
        return m_chunk_offset;
    }

    inline bool initialized() const
    {
        ASSERT_INVARIANT(
            (m_chunk_offset != c_invalid_chunk_offset) == bool(m_metadata),
            "The chunk offset and the metadata pointer must both be set or both be unset!");
        return (m_chunk_offset != c_invalid_chunk_offset);
    }

    // Initialize the chunk manager with an empty chunk.
    // (The chunk manager must not already be initialized.)
    void initialize(chunk_offset_t chunk_offset);

    // Initialize the chunk manager with a used chunk.
    // (The chunk manager must not already be initialized.)
    void load(chunk_offset_t chunk_offset);

    // Takes ownership of the current chunk from the chunk manager. This
    // facilitates reusing the same chunk manager instance for many chunks.
    chunk_offset_t release();

    // Returns an opaque version token for detecting chunk reuse.
    chunk_version_t get_version() const;

    // Returns the current state of the chunk.
    chunk_state_t get_state() const;

    // Allocate a new memory block inside our managed chunk.
    gaia_offset_t allocate(size_t allocation_slots_size);

    // Mark the slot corresponding to this offset as deallocated.
    // It is illegal to call this method twice on the same offset
    // (unless the containing chunk was reused, of course).
    void deallocate(gaia_offset_t offset);

    // Transitions the chunk from EMPTY to IN_USE.
    // Returns false if the chunk is concurrently allocated by another thread.
    bool allocate_chunk();

    // Transitions the chunk from IN_USE to RETIRED and deallocates it if empty
    // (transitioning it to DEALLOCATING and then to EMPTY).
    // A chunk in RETIRED state cannot be used for any new allocations until
    // it is transitioned to EMPTY state, making it eligible for reuse.
    // When all allocations in the chunk have been deallocated, the chunk is
    // eligible to transition to EMPTY state (via a temporary transition to
    // DEALLOCATING for decommitting data pages).
    // This should only be called by an owning thread like a session, so it
    // cannot fail. The supplied version is used to safely deallocate the
    // retired chunk if it is empty.
    void retire_chunk(chunk_version_t version);

    // Deallocates the chunk if empty, using the given version to detect
    // concurrent reuse. Transitions the chunk from RETIRED to DEALLOCATING and
    // decommits all data pages, then transitions the chunk from DEALLOCATING to
    // EMPTY and marks it as deallocated in the allocated chunk bitmap.
    // Returns false if the chunk was not deallocated, either because it was
    // non-empty or because it was concurrently reused.
    // This can safely be called by non-owning threads like GC tasks.
    bool try_deallocate_chunk(chunk_version_t initial_version);

    // This returns the last allocation recorded in this chunk, so a crashed
    // session can reconcile its txn log with allocation metadata. For
    // crash-consistency, we ensure that the "last allocation metadata" is
    // reconciled before returning the last allocation.
    gaia_offset_t last_allocated_offset();

private:
    // The offset of the managed chunk within the global chunk array.
    chunk_offset_t m_chunk_offset;

    // A pointer to our metadata information, stored inside the memory chunk that we manage.
    chunk_manager_metadata_t* m_metadata;

private:
    void initialize_internal(chunk_offset_t chunk_offset, bool initialize_memory);

    inline void validate_initialized() const
    {
        ASSERT_PRECONDITION(
            (m_chunk_offset != c_invalid_chunk_offset) && (m_metadata != nullptr),
            "Chunk manager was not initialized!");
    }

    inline void validate_uninitialized() const
    {
        ASSERT_PRECONDITION(
            (m_chunk_offset == c_invalid_chunk_offset) && (m_metadata == nullptr),
            "Chunk manager was already initialized!");
    }

    // Checks whether a slot is currently allocated in the slot bitmaps.
    // (A slot is considered "allocated" if it is the first slot of an
    // allocation, and that allocation has not been subsequently deallocated.
    // Equivalently, its bit in the allocation bitmap is set, and its bit in the
    // deallocation bitmap is not set.)
    bool is_slot_allocated(slot_offset_t slot_offset) const;

    // Marks a slot in the allocation or deallocation bitmap.
    void mark_slot(slot_offset_t slot_offset, bool allocating_slot);

    // Marks a slot in the allocation bitmap.
    void mark_slot_allocated(slot_offset_t slot_offset);

    // Marks a slot in the deallocation bitmap.
    void mark_slot_deallocated(slot_offset_t slot_offset);

    // Returns whether a chunk has no live allocations.
    // Specify `version` to fail if the chunk version changes during the
    // metadata scan (which indicates concurrent reuse).
    bool is_empty(chunk_version_t initial_version) const;

    // Decommits all data pages in this chunk.
    void decommit_data_pages();
};

} // namespace memory_manager
} // namespace db
} // namespace gaia
