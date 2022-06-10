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

#include "type_index.hpp"

namespace gaia
{
namespace db
{

class type_index_cursor_t
{
public:
    inline type_index_cursor_t(type_index_t* type_index, common::gaia_type_t type);

    inline explicit operator bool() const;

    // Advances the cursor to the next node in the list.
    // Returns false if we have reached the end of the list, true otherwise.
    inline bool advance();

    // Resets the cursor to the head of the list.
    inline void reset();

    // Returns true if the current node has been logically deleted, false otherwise.
    // PRECONDITION: the cursor is positioned before the end of the list.
    inline bool is_current_node_deleted();

    // Returns the locator corresponding to the current node's position in the locator array.
    inline gaia_locator_t current_locator();

    // Returns the locator corresponding to the node pointed to by the current node.
    inline gaia_locator_t next_locator();

    // Unlinks the current node from the list by pointing the previous node to
    // the next unmarked node in the list.
    // Returns false if the previous node was already marked for deletion (so
    // the caller must unlink that node before retrying), true otherwise.
    //
    // PRECONDITION: the current node is marked for deletion.
    // POSTCONDITION: returns false if the previous node was already marked for deletion.
    //
    // NB: Because we also unlink all contiguous nodes marked for deletion after
    // the current node, this has the side effect of advancing the cursor to the
    // first node after the current node that is not marked for deletion (or to
    // the end of the list if all nodes following the current node are marked
    // for deletion).
    inline bool unlink_for_deletion();

private:
    // Pointer to parent type index structure.
    type_index_t* m_type_index;

    // Previous node in the traversal.
    locator_list_node_t* m_prev_node;

    // Current node in the traversal.
    locator_list_node_t* m_curr_node;

    // Locator index of current node in the traversal.
    gaia_locator_t m_curr_locator;

    // Type ID of node list.
    common::gaia_type_t m_type;
};

// The fields are ordered to minimize size (since this structure is passed by
// value), so check that size hasn't changed, to catch reorderings.
constexpr size_t c_expected_type_index_cursor_size = 32;
static_assert(sizeof(type_index_cursor_t) == c_expected_type_index_cursor_size);

#include "type_index_cursor.inc"

} // namespace db
} // namespace gaia
