////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include "gaia/common.hpp"

#include "gaia_internal/common/assert.hpp"
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

    static constexpr size_t c_locator_bit_width{32UL};
    static constexpr uint64_t c_locator_shift{0UL};
    static constexpr uint64_t c_locator_mask{((1UL << c_locator_bit_width) - 1) << c_locator_shift};

    static constexpr size_t c_deleted_flag_bit_width{1UL};
    static constexpr size_t c_deleted_flag_shift{
        common::c_uint64_bit_count - c_deleted_flag_bit_width};
    static constexpr uint64_t c_deleted_flag_mask{1UL << c_deleted_flag_shift};

    // Returns the locator index of the current node's successor, or the invalid
    // locator if it has no successor.
    inline gaia_locator_t get_next_locator();

    // Changes the node's successor to `locator`.
    // A caller can specify `relaxed_store=true` when the store will be made
    // globally visible by a subsequent "release" operation (e.g., a CAS).
    inline void set_next_locator(gaia_locator_t locator, bool relaxed_store = false);

    // Returns false if the "deleted" bit is set or the successor has been changed, true otherwise.
    inline bool try_set_next_locator(gaia_locator_t expected_locator, gaia_locator_t desired_locator);

    // Returns true if the "deleted" bit is set, false otherwise.
    inline bool is_marked_for_deletion();

    // Returns false if the "deleted" bit was already set, true otherwise.
    inline bool mark_for_deletion();
};

static_assert(
    sizeof(locator_list_node_t) == sizeof(uint64_t),
    "Expected locator_list_node_t to occupy 8 bytes!");

static_assert(std::atomic<locator_list_node_t>::is_always_lock_free);

// Holds a type ID and the head of its linked list of locators.
// This structure is always updated atomically via CAS.
struct type_index_entry_t
{
    // The type ID of this locator list.
    common::gaia_type_t::value_type type;

    // The first locator in the list.
    gaia_locator_t::value_type first_locator;
};

static_assert(
    sizeof(type_index_entry_t) == sizeof(uint64_t),
    "Expected type_index_entry_t to occupy 8 bytes!");
static_assert(std::atomic<type_index_entry_t>::is_always_lock_free);

// An index enabling efficient retrieval of all locators of a registered type.
// Each type ID maps to a linked list containing all locators of that type.
class type_index_t
{
public:
    // Inserts the node for `locator` at the head of the list for `type`.
    // PRECONDITION: The list node for `locator` has not been previously used.
    // POSTCONDITION: `type_index_cursor_t(type).current_locator()` returns
    // `locator` (in the absence of concurrent invocations).
    inline void add_locator(common::gaia_type_t type, gaia_locator_t locator);

    // Logically deletes `locator` from the list for its type.
    // Returns false if `locator` was already logically deleted, true otherwise.
    // PRECONDITION: `locator` was previously allocated.
    // POSTCONDITION: returns false if list node for `locator` was already marked for deletion.
    inline bool delete_locator(gaia_locator_t locator);

    // Gets the list node corresponding to `locator`.
    inline locator_list_node_t* get_list_node(gaia_locator_t locator);

    // Returns the head of the locator list for `type`.
    inline gaia_locator_t get_first_locator(common::gaia_type_t type);

    // Changes the head of the locator list for `type` to `desired_locator`, if
    // the head is still `expected_locator`.
    // Returns true if the head is still `expected_locator`, false otherwise.
    // (This has CAS semantics because we need to retry if the head of the list
    // changes during the operation.)
    inline bool try_set_first_locator(
        common::gaia_type_t type, gaia_locator_t expected_locator, gaia_locator_t desired_locator);

private:
    // Returns a reference to the type index entry containing `type`, creating
    // it if it does not exist.
    inline std::atomic<type_index_entry_t>&
    get_or_create_type_index_entry(common::gaia_type_t type);

private:
    // Mapping of registered type IDs to the heads of their locator lists.
    std::atomic<type_index_entry_t> m_type_index_entries[c_max_types];

    // Pool of nodes used for the linked lists representing the sets of locators
    // belonging to each registered type. The array index of each node
    // corresponds to the locator it represents.
    locator_list_node_t m_locator_lists_array[c_max_locators + 1];
};

#include "type_index.inc"

} // namespace db
} // namespace gaia
