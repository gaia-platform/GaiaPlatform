/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/common.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "type_index.hpp"

namespace gaia
{
namespace db
{

class type_index_cursor_t
{
public:
    type_index_cursor_t(type_index_t* type_index, common::gaia_type_t type)
        : m_type_index(type_index), m_type(type), m_prev_node(nullptr)
    {
        m_curr_locator = m_type_index->get_first_locator(m_type);
        m_curr_node = m_type_index->get_list_node(m_curr_locator);
    }

    inline explicit operator bool() const
    {
        return static_cast<bool>(m_curr_node);
    }

    // Advances the cursor to the next node in the list.
    // Returns false if we have reached the end of the list, true otherwise.
    inline bool advance()
    {
        if (!m_curr_node)
        {
            return false;
        }
        m_prev_node = m_curr_node;
        m_curr_locator = m_curr_node->get_next_locator();
        m_curr_node = m_type_index->get_list_node(m_curr_locator);
        return static_cast<bool>(m_curr_node);
    }

    // Resets the cursor to the head of the list.
    inline void reset()
    {
        // Create a new cursor with the same type and copy it into this cursor
        // using the default copy constructor.
        *this = type_index_cursor_t(m_type_index, m_type);
    }

    // Returns true if the current node has been logically deleted, false otherwise.
    // PRECONDITION: the cursor is positioned before the end of the list.
    inline bool is_current_node_deleted()
    {
        ASSERT_PRECONDITION(m_curr_node, "Cannot call is_current_node_deleted() at end of list!");
        return m_curr_node->is_marked_for_deletion();
    }

    // Returns the locator corresponding to the current node's position in the locator array.
    inline gaia_locator_t current_locator()
    {
        return m_curr_locator;
    }

    // Returns the locator corresponding to the node pointed to by the current node.
    inline gaia_locator_t next_locator()
    {
        return m_curr_node->get_next_locator();
    }

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
    inline bool unlink_for_deletion()
    {
        ASSERT_PRECONDITION(current_locator().is_valid(), "Cannot unlink invalid locator!");
        ASSERT_PRECONDITION(is_current_node_deleted(), "Cannot unlink a locator not marked for deletion!");

        // Save the previous node and the current locator since they'll be
        // overwritten by advance().
        locator_list_node_t* prev_node = m_prev_node;
        gaia_locator_t unlinked_locator = current_locator();

        // Traverse through all marked nodes, stopping at the first unmarked
        // node or at the end of the list, whichever comes first.
        while (advance() && is_current_node_deleted())
            ;

        // Handle the special case where the first marked node is at the head of the list.
        if (!prev_node)
        {
            return m_type_index->set_first_locator(m_type, unlinked_locator, current_locator());
        }

        // Otherwise, set the previous node to point to the new current locator
        // (either the first unmarked node or the invalid locator, indicating
        // end of list).
        // Return whether the CAS succeeded (it could fail if the previous node
        // has either been marked for deletion or pointed to a new successor).
        return prev_node->set_next_locator(unlinked_locator, current_locator());
    }

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

} // namespace db
} // namespace gaia
