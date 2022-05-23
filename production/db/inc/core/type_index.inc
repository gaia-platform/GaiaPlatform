////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

gaia_locator_t locator_list_node_t::get_next_locator()
{
    return static_cast<gaia_locator_t>((data_word & c_locator_mask) >> c_locator_shift);
}

static uint64_t data_word_from_locator(gaia_locator_t locator)
{
    return locator.value() << locator_list_node_t::c_locator_shift;
}

void locator_list_node_t::set_next_locator(gaia_locator_t locator, bool relaxed_store)
{
    uint64_t word{data_word_from_locator(locator)};
    std::memory_order order = relaxed_store ? std::memory_order_relaxed : std::memory_order_seq_cst;
    return data_word.store(word, order);
}

bool locator_list_node_t::try_set_next_locator(
    gaia_locator_t expected_locator, gaia_locator_t desired_locator)
{
    // The "expected" word has the "deleted" bit unset, because we want to
    // fail if this node has been marked for deletion.
    uint64_t expected_word{data_word_from_locator(expected_locator)};
    uint64_t desired_word{data_word_from_locator(desired_locator)};
    return data_word.compare_exchange_strong(expected_word, desired_word);
}

bool locator_list_node_t::is_marked_for_deletion()
{
    return static_cast<bool>((data_word & c_deleted_flag_mask) >> c_deleted_flag_shift);
}

bool locator_list_node_t::mark_for_deletion()
{
    while (true)
    {
        uint64_t expected_word{data_word};
        if (expected_word & c_deleted_flag_mask)
        {
            return false;
        }

        uint64_t desired_word{data_word | c_deleted_flag_mask};
        if (data_word.compare_exchange_strong(expected_word, desired_word))
        {
            break;
        }
    }

    return true;
}

std::atomic<type_index_entry_t>& type_index_t::get_or_create_type_index_entry(common::gaia_type_t type)
{
    ASSERT_PRECONDITION(
        type.is_valid(),
        "Cannot call get_or_create_type_index_entry() with an invalid type!");

    // This implements the insert operation on a lock-free set. Inserting a
    // duplicate element is prevented by CAS semantics: each concurrent insert
    // uses the next uninitialized array entry (there can be no "holes" in the
    // array because entries only go from zero to nonzero, and we never scan
    // past an entry initially read as zero until a CAS shows it is nonzero), so
    // for any two concurrent inserts, one of them (the one that initializes the
    // higher-indexed entry) must see the other's insert, and abort if has the
    // same value.
    //
    // REVIEW: With our current limit of 64 types, linear search should be
    // fine (the whole array is at most 8 cache lines, so should almost
    // always be in L1 cache), but with more types we'll eventually need
    // sublinear search complexity (e.g., a hash table).
    //
    // Scan until the first uninitialized entry or the end of the array,
    // whichever comes first.
    for (size_t i = 0; i < std::size(m_type_index_entries); ++i)
    {
        auto& entry_ref = m_type_index_entries[i];
        // This load can be relaxed, because the subsequent CAS will detect a
        // stale read.
        auto entry_val = entry_ref.load(std::memory_order_relaxed);
        // The type was already registered.
        if (entry_val.type == type)
        {
            return entry_ref;
        }

        // Try to initialize the first uninitialized entry.
        if (entry_val.type == common::c_invalid_gaia_type)
        {
            type_index_entry_t expected_entry{common::c_invalid_gaia_type, c_invalid_gaia_locator};
            type_index_entry_t desired_entry{type, c_invalid_gaia_locator};

            // If the CAS succeeds, we are done. Otherwise, we need to check if
            // the type was already registered in this entry. If so, we are
            // done. If not, try the next entry.
            if (entry_ref.compare_exchange_strong(expected_entry, desired_entry)
                || expected_entry.type == type)
            {
                return entry_ref;
            }
        }
    }

    // We reached the end of the array without finding an uninitialized entry.
    throw type_limit_exceeded_internal();
}

static gaia_locator_t get_first_locator_internal(
    std::atomic<type_index_entry_t>& type_index_entry, bool relaxed_load = false)
{
    std::memory_order order = relaxed_load ? std::memory_order_relaxed : std::memory_order_seq_cst;
    return type_index_entry.load(order).first_locator;
}

gaia_locator_t type_index_t::get_first_locator(common::gaia_type_t type)
{
    auto& type_index_entry = get_or_create_type_index_entry(type);
    return get_first_locator_internal(type_index_entry);
}

static bool try_set_first_locator_internal(
    std::atomic<type_index_entry_t>& type_index_entry,
    gaia_locator_t expected_locator, gaia_locator_t desired_locator)
{
    // This relaxed load is correct because the calling thread has already
    // called `get_or_create_type_index_entry()` to retrieve `type_index_entry`,
    // in which it verified that the value of `type_index_entry.type` was equal
    // to the desired type. Because a type index entry is immutable once
    // initialized, and even relaxed loads cannot go backward in time, the
    // caller is guaranteed to read the same value of `type_index_entry.type`
    // that it read previously.
    common::gaia_type_t type = type_index_entry.load(std::memory_order_relaxed).type;
    ASSERT_INVARIANT(type.is_valid(), "Type index entry must have valid type!");

    type_index_entry_t expected_entry{type, expected_locator};
    type_index_entry_t desired_entry{type, desired_locator};

    return type_index_entry.compare_exchange_strong(expected_entry, desired_entry);
}

bool type_index_t::try_set_first_locator(
    common::gaia_type_t type, gaia_locator_t expected_locator, gaia_locator_t desired_locator)
{
    auto& type_index_entry = get_or_create_type_index_entry(type);
    return try_set_first_locator_internal(type_index_entry, expected_locator, desired_locator);
}

locator_list_node_t* type_index_t::get_list_node(gaia_locator_t locator)
{
    if (!locator.is_valid())
    {
        return nullptr;
    }

    return &m_locator_lists_array[locator];
}

void type_index_t::add_locator(common::gaia_type_t type, gaia_locator_t locator)
{
    ASSERT_PRECONDITION(type.is_valid(), "Cannot call add_locator() with an invalid type!");
    ASSERT_PRECONDITION(locator.is_valid(), "Cannot call add_locator() with an invalid locator!");

    locator_list_node_t* new_node = get_list_node(locator);

    // REVIEW: Checking the new node for logically deleted status and for a
    // valid next pointer will not detect all cases of reuse, since a reused
    // node may not be deleted, and the tail node of a list will always
    // point to the invalid locator. We probably need to reserve an
    // additional metadata bit to track allocated status (there was one
    // originally but it was removed because I thought it introduced too
    // much complexity).
    ASSERT_INVARIANT(new_node, "A new locator must have a valid list node!");
    ASSERT_INVARIANT(!new_node->get_next_locator().is_valid(), "A new locator cannot point to another locator!");
    ASSERT_INVARIANT(!new_node->is_marked_for_deletion(), "A new locator cannot be logically deleted!");

    auto& type_index_entry = get_or_create_type_index_entry(type);

    while (true)
    {
        // Take a snapshot of the first node in the list. We use a relaxed load
        // in get_first_locator(), because a stale read will cause the CAS in
        // try_set_first_locator() to fail and we'll retry the read.
        bool relaxed_load = true;
        gaia_locator_t first_locator = get_first_locator_internal(type_index_entry, relaxed_load);

        // Point the new node to the snapshot of the first node in the list. We
        // explicitly set the next locator even if it's invalid, and use a
        // relaxed store because when the subsequent CAS eventually succeeds, it
        // will ensure this store is globally visible (due to release semantics).
        bool relaxed_store = true;
        new_node->set_next_locator(first_locator, relaxed_store);

        // Now try to point the list head to the new node, retrying if it
        // was concurrently pointed to another node.
        if (try_set_first_locator_internal(type_index_entry, first_locator, locator))
        {
            break;
        }
    }
}

bool type_index_t::delete_locator(gaia_locator_t locator)
{
    ASSERT_PRECONDITION(locator.is_valid(), "Cannot call delete_locator() with an invalid locator!");

    // To avoid traversing the list, we only mark the node corresponding to
    // this locator as deleted. A subsequent traversal will unlink the node.
    locator_list_node_t* node = get_list_node(locator);
    return node->mark_for_deletion();
}
