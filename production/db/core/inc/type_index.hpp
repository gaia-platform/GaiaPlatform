/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/common.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/exceptions.hpp"

#include "db_internal_types.hpp"

namespace gaia
{
namespace db
{

// Node in a linked list representing the set of locators for a registered type.
// (Each node is implicitly associated with a locator by its index in an array
// of nodes.) Holds a "deleted" bit indicating that the corresponding locator
// has been logically deleted, and a "next" locator pointing to the next node in
// the list. The last node in the list contains an invalid "next" locator.
struct locator_list_node_t
{
    // This 64-bit word logically contains a 32-bit "metadata" word, followed by
    // a 32-bit locator. The high bit of the "metadata" word is used to mark a
    // node as logically deleted. These two words must be modified atomically as
    // a single word, to preserve the invariant that a node marked for deletion
    // cannot allow any thread to modify its locator word. Ideally we would
    // steal a single bit from the locator word to use for the deletion mark,
    // but we need the entire 32-bit word to cover our 32-bit locator address
    // space.
    //
    // REVIEW: The "metadata" word could also store a locator version to enable
    // locator reuse.
    std::atomic<uint64_t> data_word;

    static constexpr size_t c_locator_bit_width{32ULL};
    static constexpr uint64_t c_locator_shift{0ULL};
    static constexpr uint64_t c_locator_mask{((1ULL << c_locator_bit_width) - 1) << c_locator_shift};

    static constexpr size_t c_deleted_flag_bit_width{1ULL};
    static constexpr size_t c_deleted_flag_shift{
        common::c_uint64_bit_count - c_deleted_flag_bit_width};
    static constexpr uint64_t c_deleted_flag_mask{1ULL << c_deleted_flag_shift};

    // Returns the locator index of the current node's successor, or the invalid
    // locator if it has no successor.
    inline gaia_locator_t get_next_locator()
    {
        return static_cast<gaia_locator_t>((data_word & c_locator_mask) >> c_locator_shift);
    }

    // Returns true if the "deleted" bit is set, false otherwise.
    inline bool is_marked_for_deletion()
    {
        return static_cast<bool>((data_word & c_deleted_flag_mask) >> c_deleted_flag_shift);
    }

    inline uint64_t data_word_from_locator(gaia_locator_t locator)
    {
        return locator.value() << c_locator_shift;
    }

    // Returns false if the "deleted" bit is set or the successor has been changed, true otherwise.
    inline bool try_set_next_locator(gaia_locator_t expected_locator, gaia_locator_t desired_locator)
    {
        // The "expected" word has the "deleted" bit unset, because we want to
        // fail if this node has been marked for deletion.
        uint64_t expected_word{data_word_from_locator(expected_locator)};
        uint64_t desired_word{data_word_from_locator(desired_locator)};
        return data_word.compare_exchange_strong(expected_word, desired_word);
    }

    // Returns false if the "deleted" bit was already set, true otherwise.
    inline bool mark_for_deletion()
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
};

static_assert(
    sizeof(locator_list_node_t) == sizeof(uint64_t),
    "Expected locator_list_node_t to occupy 8 bytes!");

static_assert(std::atomic<locator_list_node_t>::is_always_lock_free);

// Holds a type ID and the head of its linked list of locators.
struct type_index_entry_t
{
    // The type ID of this locator list.
    std::atomic<common::gaia_type_t::value_type> type;

    // The first locator in the list.
    std::atomic<gaia_locator_t::value_type> first_locator;
};

static_assert(sizeof(type_index_entry_t) == 8, "Expected sizeof(type_index_entry_t) to be 8!");

// An index enabling efficient retrieval of all locators belonging to a
// registered type. Each type ID maps to a linked list containing all locators
// of that type.
struct type_index_t
{
    // Mapping of registered type IDs to the heads of their locator lists.
    type_index_entry_t type_index_entries[c_max_types];

    // Atomic counter incremented during type registration to determine the
    // index of the registered type in the `type_index_entries` array.
    std::atomic<size_t> type_index_entries_count;

    // Pool of nodes used for the linked lists representing the sets of locators
    // belonging to each registered type. The array index of each node
    // corresponds to the locator it represents.
    locator_list_node_t locator_lists_array[c_max_locators + 1];

    // Claims a slot in `type_index_entries` by atomically incrementing
    // `type_index_entries_count` (slots are not reused).
    inline void register_type(common::gaia_type_t type)
    {
        if (type_index_entries_count >= c_max_types)
        {
            throw type_limit_exceeded_internal();
        }
        type_index_entries[type_index_entries_count++].type = type;
    }

    // Returns the head of the locator list for the given type.
    inline gaia_locator_t get_first_locator(common::gaia_type_t type)
    {
        // REVIEW: With our current limit of 64 types, linear search should be
        // fine (the whole array is at most 8 cache lines, so should almost
        // always be in L1 cache), but with more types we'll eventually need
        // sublinear search complexity.
        for (size_t i = 0; i < type_index_entries_count; ++i)
        {
            if (type_index_entries[i].type == type)
            {
                return gaia_locator_t(type_index_entries[i].first_locator);
            }
        }
        ASSERT_UNREACHABLE("Type must be registered before accessing its locator list!");
    }

    // Changes the head of the locator list for the given type to
    // `desired_locator`, if the head is still `expected_locator`.
    // Returns true if the head is still `expected_locator`, false otherwise.
    // This has CAS semantics because we need to retry if the head of the list
    // changes during the operation.
    inline bool set_first_locator(
        common::gaia_type_t type, gaia_locator_t expected_locator, gaia_locator_t desired_locator)
    {
        gaia_locator_t::value_type expected_value = expected_locator.value();
        gaia_locator_t::value_type desired_value = desired_locator.value();

        for (size_t i = 0; i < type_index_entries_count; ++i)
        {
            if (type_index_entries[i].type == type)
            {
                return type_index_entries[i].first_locator.compare_exchange_strong(expected_value, desired_value);
            }
        }
        ASSERT_UNREACHABLE("Type must be registered before accessing its locator list!");
    }

    // Gets the list node corresponding to the given locator.
    inline locator_list_node_t* get_list_node(gaia_locator_t locator)
    {
        if (!locator.is_valid())
        {
            return nullptr;
        }

        return &locator_lists_array[locator];
    }

    // Inserts the node for a locator at the head of the list for its type.
    // PRECONDITION: `type` is already registered in `type_index_entries`.
    // PRECONDITION: The list node for `locator` has not been previously used.
    // POSTCONDITION: `type_index_cursor_t(type).current_locator()` returns
    // `locator` (in the absence of concurrent invocations).
    inline void add_locator(common::gaia_type_t type, gaia_locator_t locator)
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
            if (set_first_locator(type, first_locator, locator))
            {
                break;
            }
        }
    }

    // Logically deletes the given locator from the list for its type.
    // Returns false if the given locator was already logically deleted, true otherwise.
    // PRECONDITION: `locator` was previously allocated.
    // POSTCONDITION: returns false if list node for `locator` was already marked for deletion.
    inline bool delete_locator(gaia_locator_t locator)
    {
        ASSERT_PRECONDITION(locator.is_valid(), "Cannot call delete_locator() with an invalid locator!");

        // To avoid traversing the list, we only mark the node corresponding to
        // this locator as deleted. A subsequent traversal will unlink the node.
        locator_list_node_t* node = get_list_node(locator);
        return node->mark_for_deletion();
    }
};

} // namespace db
} // namespace gaia
