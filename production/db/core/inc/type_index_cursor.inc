/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

type_index_cursor_t::type_index_cursor_t(type_index_t* type_index, common::gaia_type_t type)
    : m_type_index(type_index), m_prev_node(nullptr), m_type(type)
{
    // Existing clients (e.g., the catalog bootstrapping implementation) expect
    // that creating an iterator for an unregistered type will automatically
    // register that type.
    // Ignore failure if type is already registered.
    m_type_index->register_type(type);
    m_curr_locator = m_type_index->get_first_locator(m_type);
    m_curr_node = m_type_index->get_list_node(m_curr_locator);
}

type_index_cursor_t::operator bool() const
{
    return static_cast<bool>(m_curr_node);
}

bool type_index_cursor_t::advance()
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

void type_index_cursor_t::reset()
{
    // Create a new cursor with the same type and copy it into this cursor
    // using the default copy constructor.
    *this = type_index_cursor_t(m_type_index, m_type);
}

bool type_index_cursor_t::is_current_node_deleted()
{
    ASSERT_PRECONDITION(m_curr_node, "Cannot call is_current_node_deleted() at end of list!");
    return m_curr_node->is_marked_for_deletion();
}

gaia_locator_t type_index_cursor_t::current_locator()
{
    return m_curr_locator;
}

gaia_locator_t type_index_cursor_t::next_locator()
{
    return m_curr_node->get_next_locator();
}

bool type_index_cursor_t::unlink_for_deletion()
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
        return m_type_index->try_set_first_locator(m_type, unlinked_locator, current_locator());
    }

    // Otherwise, set the previous node to point to the new current locator
    // (either the first unmarked node or the invalid locator, indicating
    // end of list).
    // Return whether the CAS succeeded (it could fail if the previous node
    // has either been marked for deletion or pointed to a new successor).
    return prev_node->try_set_next_locator(unlinked_locator, current_locator());
}
