/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

void memory_manager_metadata_t::clear()
{
    // Start allocating at the first valid chunk offset.
    next_available_unused_chunk_offset = c_first_chunk_offset;
    // Mark all chunks as initially unallocated.
    std::fill(allocated_chunks_bitmap, allocated_chunks_bitmap + c_chunk_bitmap_size_in_words, 0);
    // Mark all metadata chunks as allocated.
    safe_set_bit_range_value(allocated_chunks_bitmap, c_chunk_bitmap_size_in_words, 0, c_first_chunk_offset, true);
}

std::pair<chunk_state_t, chunk_version_t>
chunk_manager_metadata_t::get_chunk_state_and_version() const
{
    constexpr chunk_version_t c_chunk_state_mask{
        ((1ULL << c_chunk_state_bit_width) - 1) << c_chunk_state_shift};
    constexpr chunk_version_t c_chunk_version_mask{
        ((1ULL << c_chunk_version_bit_width) - 1) << c_chunk_version_shift};
    chunk_state_t state{
        (chunk_state_and_version & c_chunk_state_mask) >> c_chunk_state_shift};
    chunk_version_t version{
        (chunk_state_and_version & c_chunk_version_mask) >> c_chunk_version_shift};
    return {state, version};
}

chunk_state_t chunk_manager_metadata_t::get_chunk_state() const
{
    return get_chunk_state_and_version().first;
}

chunk_version_t chunk_manager_metadata_t::get_chunk_version() const
{
    return get_chunk_state_and_version().second;
}

last_allocation_metadata_t::last_allocation_metadata_t()
    : prev_max_allocated_slot_offset(c_invalid_slot_offset)
    , max_allocated_slot_offset(c_invalid_slot_offset)
    , min_unallocated_slot_offset(c_first_slot_offset)
    , reserved(0)
{
}

slot_offset_t chunk_manager_metadata_t::max_allocated_slot_offset() const
{
    return last_allocation_metadata.load().max_allocated_slot_offset;
}

slot_offset_t chunk_manager_metadata_t::min_unallocated_slot_offset() const
{
    return last_allocation_metadata.load().min_unallocated_slot_offset;
}

slot_offset_t chunk_manager_metadata_t::prev_max_allocated_slot_offset() const
{
    return last_allocation_metadata.load().prev_max_allocated_slot_offset;
}

bool chunk_manager_metadata_t::apply_chunk_transition(
    chunk_state_t expected_state, chunk_state_t desired_state)
{
    chunk_version_t expected_version = get_chunk_version();
    return apply_chunk_transition_from_version(expected_version, expected_state, desired_state);
}

bool chunk_manager_metadata_t::apply_chunk_transition_from_version(
    chunk_version_t expected_version, chunk_state_t expected_state, chunk_state_t desired_state)
{
    // Validate the proposed transition.
    bool is_valid_transition = false;
    for (auto t : c_valid_chunk_transitions)
    {
        if (t.old_state == expected_state && t.new_state == desired_state)
        {
            is_valid_transition = true;
            break;
        }
    }
    ASSERT_PRECONDITION(is_valid_transition, "No valid transition found!");

    chunk_version_t desired_version = expected_version;

    // Special case: we increment the version when we allocate a chunk.
    if (expected_state == chunk_state_t::empty && desired_state == chunk_state_t::in_use)
    {
        ++desired_version;
    }

    // Construct the combined state and version tokens.
    chunk_version_t shifted_expected_version = expected_version << c_chunk_version_shift;
    chunk_version_t shifted_desired_version = desired_version << c_chunk_version_shift;

    chunk_version_t expected_token = shifted_expected_version | common::get_enum_value(expected_state);
    chunk_version_t desired_token = shifted_desired_version | common::get_enum_value(desired_state);

    return chunk_state_and_version.compare_exchange_strong(expected_token.value_ref(), desired_token.value());
}

void chunk_manager_metadata_t::update_last_allocation_metadata(size_t allocation_size_in_slots)
{
    last_allocation_metadata_t old_metadata = last_allocation_metadata;
    last_allocation_metadata_t new_metadata{};
    // TODO: Use designated initializers after C++20.
    new_metadata.prev_max_allocated_slot_offset = old_metadata.max_allocated_slot_offset;
    new_metadata.max_allocated_slot_offset = old_metadata.min_unallocated_slot_offset;
    new_metadata.min_unallocated_slot_offset = old_metadata.min_unallocated_slot_offset + allocation_size_in_slots;
    // Set the new "last allocation metadata" variables in a single atomic write.
    last_allocation_metadata = new_metadata;
}

// REVIEW: This helper logically belongs in memory_helpers.hpp, but is defined
// inline here to avoid a circular dependency. We can revisit this decision if
// it acquires additional callers.
static inline slot_offset_t bit_index_to_slot(size_t bit_index)
{
    return static_cast<slot_offset_t>(bit_index + c_first_slot_offset);
}

void chunk_manager_metadata_t::synchronize_allocation_metadata()
{
    // Find the last set bit in the allocation bitmap.
    size_t bit_index = find_last_set_bit(allocated_slots_bitmap, c_slot_bitmap_size_in_words);
    slot_offset_t last_allocated_slot_from_bitmap{
        (bit_index == c_max_bit_index)
            ? c_invalid_slot_offset
            : bit_index_to_slot(bit_index)};

    // Check to be sure the last set bit points to the same slot as
    // max_allocated_slot_offset. If not, then it must point to
    // prev_max_allocated_slot_offset (since metadata is updated
    // atomically).
    if (last_allocated_slot_from_bitmap != max_allocated_slot_offset())
    {
        ASSERT_INVARIANT(
            last_allocated_slot_from_bitmap == prev_max_allocated_slot_offset(),
            "Last allocated slot in bitmap must point to either last allocation or previous allocation!");

        // Fix up the allocation bitmap.
        set_bit_value(
            allocated_slots_bitmap, c_slot_bitmap_size_in_words,
            max_allocated_slot_offset(), true);
    }
}

void chunk_manager_metadata_t::clear()
{
    // NB: We cannot clear the chunk state and version!
    shared_lock.clear();
    last_allocation_metadata.store({});
    std::fill(allocated_slots_bitmap, allocated_slots_bitmap + c_slot_bitmap_size_in_words, 0);
    std::fill(deallocated_slots_bitmap, deallocated_slots_bitmap + c_slot_bitmap_size_in_words, 0);
}
