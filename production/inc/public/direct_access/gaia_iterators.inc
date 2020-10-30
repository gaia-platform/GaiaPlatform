/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

namespace gaia
{
namespace direct_access
{

//
// Implementation for gaia_iterator_t.
//
template <typename T_class>
gaia_iterator_t<T_class>::gaia_iterator_t(gaia_id_t id)
{
    m_obj = T_class::get(id);
}

template <typename T_class>
gaia_iterator_t<T_class>::gaia_iterator_t(gaia_id_t id, std::function<bool(const T_class&)> filter_function)
    : m_filter_fn(filter_function)
{
    m_obj = T_class::get(id);
}

template <typename T_class>
gaia_iterator_t<T_class>& gaia_iterator_t<T_class>::operator++()
{
    if (!m_obj)
    {
        return *this;
    }

    if (m_filter_fn)
    {
        for (m_obj = m_obj.get_next(); m_obj; m_obj = m_obj.get_next())
        {
            if (m_filter_fn(m_obj))
            {
                break;
            }
        }
    }
    else
    {
        if (m_obj)
        {
            m_obj = m_obj.get_next();
        }
    }

    return *this;
}

template <typename T_class>
gaia_iterator_t<T_class> gaia_iterator_t<T_class>::operator++(int)
{
    gaia_iterator_t<T_class> old = *this;
    operator++();
    return old;
}

template <typename T_class>
bool gaia_iterator_t<T_class>::operator==(const gaia_iterator_t& rhs) const
{
    return m_obj == rhs.m_obj;
}

template <typename T_class>
bool gaia_iterator_t<T_class>::operator!=(const gaia_iterator_t& rhs) const
{
    return !(m_obj == rhs.m_obj);
}

template <typename T_class>
typename gaia_iterator_t<T_class>::reference gaia_iterator_t<T_class>::operator*()
{
    return m_obj;
}

template <typename T_class>
typename gaia_iterator_t<T_class>::pointer gaia_iterator_t<T_class>::operator->()
{
    return &m_obj;
}

//
// Implementation for gaia_container_t
//

// The begin() method returns either the first element, or the first element that satisfies m_filter_fn.
template <gaia_type_t T_container, typename T_class>
gaia_iterator_t<T_class> gaia_container_t<T_container, T_class>::begin()
{
    gaia_id_t id = 0;
    gaia_ptr node_ptr;
    for (node_ptr = gaia_ptr::find_first(T_container); node_ptr; node_ptr = node_ptr.find_next())
    {
        if (m_filter_fn)
        {
            if (m_filter_fn(T_class::get(node_ptr.id())))
            {
                return gaia_iterator_t<T_class>(node_ptr.id(), m_filter_fn);
            }
        }
        else
        {
            break;
        }
    }
    if (node_ptr)
    {
        id = node_ptr.id();
    }

    return gaia_iterator_t<T_class>(id);
}

// The where() method constructs a gaia_container_t with a std::function to be used for filtering.
template <gaia_type_t T_container, typename T_class>
gaia_container_t<T_container, T_class> gaia_container_t<T_container, T_class>::where(std::function<bool(const T_class&)> filter_function)
{
    return gaia_container_t<T_container, T_class>(filter_function);
}

template <gaia_type_t T_container, typename T_class>
gaia_iterator_t<T_class> gaia_container_t<T_container, T_class>::end()
{
    return gaia_iterator_t<T_class>(0);
}

//
// Implementation for gaia_set_iterator_t.
//
template <typename T_child, size_t T_next_slot>
gaia_set_iterator_t<T_child, T_next_slot>::gaia_set_iterator_t(gaia_id_t id)
{
    m_child_obj = T_child::get(id);
}

template <typename T_child, size_t T_next_slot>
gaia_set_iterator_t<T_child, T_next_slot>::gaia_set_iterator_t(gaia_id_t id, std::function<bool(const T_child&)> filter_function)
    : m_filter_fn(filter_function)
{
    m_child_obj = T_child::get(id);
}

template <typename T_child, size_t T_next_slot>
typename gaia_set_iterator_t<T_child, T_next_slot>::reference gaia_set_iterator_t<T_child, T_next_slot>::operator*()
{
    return m_child_obj;
}

template <typename T_child, size_t T_next_slot>
typename gaia_set_iterator_t<T_child, T_next_slot>::pointer gaia_set_iterator_t<T_child, T_next_slot>::operator->()
{
    return &m_child_obj;
}

template <typename T_child, size_t T_next_slot>
gaia_set_iterator_t<T_child, T_next_slot>& gaia_set_iterator_t<T_child, T_next_slot>::operator++()
{
    if (!m_child_obj)
    {
        return *this;
    }

    if (m_filter_fn)
    {
        for (m_child_obj = T_child::get(m_child_obj.references()[T_next_slot]);
             m_child_obj;
             m_child_obj = T_child::get(m_child_obj.references()[T_next_slot]))
        {
            if (m_filter_fn(m_child_obj))
            {
                break;
            }
        }
    }
    else
    {
        if (m_child_obj)
        {
            m_child_obj = T_child::get(m_child_obj.references()[T_next_slot]);
        }
    }

    return *this;
}

template <typename T_child, size_t T_next_slot>
gaia_set_iterator_t<T_child, T_next_slot> gaia_set_iterator_t<T_child, T_next_slot>::operator++(int)
{
    gaia_set_iterator_t<T_child, T_next_slot> old = *this;
    operator++();
    return old;
}

template <typename T_child, size_t T_next_slot>
bool gaia_set_iterator_t<T_child, T_next_slot>::operator==(const gaia_set_iterator_t& rhs) const
{
    return m_child_obj == rhs.m_child_obj;
}

template <typename T_child, size_t T_next_slot>
bool gaia_set_iterator_t<T_child, T_next_slot>::operator!=(const gaia_set_iterator_t& rhs) const
{
    return !(m_child_obj == rhs.m_child_obj);
}

//
// Implementation for reference_chain_container_t.
//
template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
gaia_set_iterator_t<T_child, T_next_slot> reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::begin()
{
    gaia_id_t id = 0;

    if (m_parent_id)
    {
        auto node_ptr = gaia_ptr::open(m_parent_id);
        id = node_ptr.references()[T_child_slot];
        while (id)
        {
            node_ptr = gaia_ptr::open(id);
            if (m_filter_fn)
            {
                if (m_filter_fn(T_child::get(id)))
                {
                    break;
                }
                id = node_ptr.references()[T_next_slot];
            }
            else
            {
                break;
            }
        }
    }
    return gaia_set_iterator_t<T_child, T_next_slot>(id, m_filter_fn);
}

// The where() method saves the address of the std::function, to be called for each candidate return value.
template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>
reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::where(std::function<bool(const T_child&)> filter_function)
{
    return reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>(m_parent_id, filter_function);
}

template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
gaia_set_iterator_t<T_child, T_next_slot> reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::end()
{
    return gaia_set_iterator_t<T_child, T_next_slot>(0);
}

template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
void reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::insert(gaia_id_t child_id)
{
    auto oid = m_parent_id;

    // The gaia_id() will be zero if the row hasn't been inserted into the SE.
    if (child_id == 0 || oid == 0)
    {
        T_parent expected = T_parent::get(0);
        T_child received = T_child::get(0);
        throw edc_invalid_state(expected.gaia_typename(), received.gaia_typename());
    }

    auto node_ptr = gaia_ptr::open(oid);
    if (!node_ptr)
    {
        return;
    }

    auto child_ptr = gaia_ptr::open(child_id);
    if (!child_ptr)
    {
        return;
    }

    // This is a no-op if it is already connected to this owner.
    if (child_ptr.references()[T_parent_slot] == oid)
    {
        return;
    }

    // Cannot connect a child object that is already connected to a different parent.
    if (child_ptr.references()[T_parent_slot] != 0 && child_ptr.references()[T_parent_slot] != oid)
    {
        T_parent expected = T_parent::get(0);
        throw edc_already_inserted(child_ptr.references()[T_parent_slot], expected.gaia_typename());
    }

    child_ptr.update_child_references(T_next_slot, node_ptr.references()[T_child_slot], T_parent_slot, oid);
    node_ptr.update_child_reference(T_child_slot, child_id);
}

template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
void reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::insert(T_child& child_edc)
{
    insert(child_edc.gaia_id());
}

template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
void reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::erase(gaia_id_t child_id)
{
    auto child_ptr = gaia_ptr::open(child_id);
    if (!child_ptr)
    {
        return;
    }

    if (child_ptr.references()[T_parent_slot] != m_parent_id)
    {
        T_parent expected = T_parent::get(0);
        T_child received = T_child::get(0);
        throw edc_invalid_member(
            m_parent_id,
            T_parent::s_gaia_type,
            expected.gaia_typename(),
            received.gaia_type(),
            received.gaia_typename());
    }

    auto node_ptr = gaia_ptr::open(m_parent_id);
    if (!node_ptr)
    {
        return;
    }

    if (node_ptr.references()[T_child_slot] == child_id)
    {
        // It's the first one in the list, point the "first" to the current "next".
        node_ptr.update_child_reference(T_child_slot, child_ptr.references()[T_next_slot]);
        // Clean up the removed child.
        child_ptr.update_child_references(T_next_slot, INVALID_GAIA_ID, T_parent_slot, INVALID_GAIA_ID);
    }
    else
    {
        // Need to scan the list to find this one because it's not first on the list.
        auto cur_child_ptr = gaia_ptr::open(node_ptr.references()[T_child_slot]);

        while (cur_child_ptr && cur_child_ptr.references()[T_next_slot])
        {
            gaia_id_t next_id = cur_child_ptr.references()[T_next_slot];
            if (next_id == child_id)
            {
                // Point the current child to the child following the next.
                cur_child_ptr.update_child_reference(T_next_slot, child_ptr.references()[T_next_slot]);
                // Clean up the removed child.
                child_ptr.update_child_references(T_next_slot, INVALID_GAIA_ID, T_parent_slot, INVALID_GAIA_ID);
                return;
            }
            // Move to the next child.
            cur_child_ptr = gaia_ptr::open(next_id);
        }
        // If we end up here, the child was not found in the chain. This is an error because
        // the pointers have become inconsistent (the child's parent pointer was correct).
        throw edc_inconsistent_list(
            m_parent_id,
            T_parent::get(0).gaia_typename(),
            child_id,
            T_child::get(0).gaia_typename());
    }
}

template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
void reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::erase(T_child& child_edc)
{
    erase(child_edc.gaia_id());
}

} // namespace direct_access
} // namespace gaia
