/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

gaia_locator_t locator_list_node_t::get_next_locator()
{
    return static_cast<gaia_locator_t>((data_word & c_locator_mask) >> c_locator_shift);
}

bool locator_list_node_t::is_marked_for_deletion()
{
    return static_cast<bool>((data_word & c_deleted_flag_mask) >> c_deleted_flag_shift);
}

static uint64_t data_word_from_locator(gaia_locator_t locator)
{
    return locator.value() << locator_list_node_t::c_locator_shift;
}

bool locator_list_node_t::try_set_next_locator(gaia_locator_t expected_locator, gaia_locator_t desired_locator)
{
    // The "expected" word has the "deleted" bit unset, because we want to
    // fail if this node has been marked for deletion.
    uint64_t expected_word{data_word_from_locator(expected_locator)};
    uint64_t desired_word{data_word_from_locator(desired_locator)};
    return data_word.compare_exchange_strong(expected_word, desired_word);
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

std::atomic<type_index_entry_t>& type_index_t::get_type_index_entry(common::gaia_type_t type)
{
    ASSERT_PRECONDITION(type.is_valid(), "Cannot call get_type_index_entry() with an invalid type!");

    // REVIEW: With our current limit of 64 types, linear search should be
    // fine (the whole array is at most 8 cache lines, so should almost
    // always be in L1 cache), but with more types we'll eventually need
    // sublinear search complexity (e.g., a hash table).

    // Scan until the end of the array. (We could stop at the first
    // uninitialized entry, but the branch is likely not worth it for such a
    // small array; see e.g.
    // https://dirtyhandscoding.wordpress.com/2017/08/25/performance-comparison-linear-search-vs-binary-search/.)
    for (size_t i = 0; i < std::size(type_index_entries); ++i)
    {
        auto& entry_ref = type_index_entries[i];
        auto entry_val = entry_ref.load();
        if (entry_val.type == type)
        {
            return entry_ref;
        }
    }
    // If we reach the end of the array without finding the entry for this type,
    // the precondition has been violated.
    ASSERT_UNREACHABLE("Type must be registered before calling get_type_index_entry()!");
}

bool type_index_t::register_type(common::gaia_type_t type)
{
    ASSERT_PRECONDITION(type.is_valid(), "Cannot call register_type() with an invalid type!");

    // This implements the insert operation on a lock-free set. Inserting a
    // duplicate element is prevented by CAS semantics: each concurrent insert
    // uses the next uninitialized array entry (there can be no "holes" in the
    // array because entries only go from zero to nonzero, and we never scan
    // past an entry initially read as zero until a CAS shows it is nonzero), so
    // for any two concurrent inserts, one of them (the one that initializes the
    // higher-indexed entry) must see the other's insert, and abort if has the
    // same value.
    //
    // Scan until the first uninitialized entry or the end of the array,
    // whichever comes first.
    for (size_t i = 0; i < std::size(type_index_entries); ++i)
    {
        auto& entry_ref = type_index_entries[i];
        auto entry_val = entry_ref.load();
        // The type was already registered.
        if (entry_val.type == type)
        {
            return false;
        }

        // Try to initialize the first uninitialized entry.
        //
        // REVIEW: This could technically be a relaxed load, because the
        // subsequent CAS will detect a stale read. However, we don't currently
        // specify non-default memory orderings anywhere, and I think we should
        // only change this policy on the basis of profiling data.
        if (entry_val.type == common::c_invalid_gaia_type)
        {
            type_index_entry_t expected_entry{common::c_invalid_gaia_type, c_invalid_gaia_locator};
            type_index_entry_t desired_entry{type, c_invalid_gaia_locator};

            // If the CAS succeeds, we are done, otherwise try the next entry.
            if (entry_ref.compare_exchange_strong(expected_entry, desired_entry))
            {
                return true;
            }
        }
    }

    // We reached the end of the array without finding an uninitialized entry.
    throw type_limit_exceeded_internal();
}

gaia_locator_t type_index_t::get_first_locator(common::gaia_type_t type)
{
    return get_type_index_entry(type).load().first_locator;
}

bool type_index_t::try_set_first_locator(
    common::gaia_type_t type, gaia_locator_t expected_locator, gaia_locator_t desired_locator)
{
    type_index_entry_t expected_entry{type, expected_locator};
    type_index_entry_t desired_entry{type, desired_locator};

    return get_type_index_entry(type).compare_exchange_strong(expected_entry, desired_entry);
}

locator_list_node_t* type_index_t::get_list_node(gaia_locator_t locator)
{
    if (!locator.is_valid())
    {
        return nullptr;
    }

    return &locator_lists_array[locator];
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
    ASSERT_INVARIANT(!new_node->get_next_locator().is_valid(), "A new locator cannot point to another locator!");
    ASSERT_INVARIANT(!new_node->is_marked_for_deletion(), "A new locator cannot be logically deleted!");

    while (true)
    {
        // Take a snapshot of the first node in the list.
        gaia_locator_t first_locator = get_first_locator(type);

        // Point the new node to the snapshot of the first node in the list.
        // We explicitly set the next locator even if it's invalid. There
        // can be no concurrent changes to the next locator of the new node,
        // so we just use its previous value for the CAS.
        bool has_succeeded = new_node->try_set_next_locator(new_node->get_next_locator(), first_locator);
        ASSERT_POSTCONDITION(has_succeeded, "Setting the next locator on a new node cannot fail!");

        // Now try to point the list head to the new node, retrying if it
        // was concurrently pointed to another node.
        if (try_set_first_locator(type, first_locator, locator))
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
