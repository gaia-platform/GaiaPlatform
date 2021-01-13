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
gaia_iterator_t<T_class>::gaia_iterator_t(gaia::common::gaia_id_t id)
{
    m_obj = T_class::get(id);
}

template <typename T_class>
gaia_iterator_t<T_class>::gaia_iterator_t(gaia::common::gaia_id_t id, std::function<bool(const T_class&)> filter_function)
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
template <gaia::common::gaia_type_t T_container, typename T_class>
gaia_iterator_t<T_class> gaia_container_t<T_container, T_class>::begin() const
{
    gaia::common::gaia_id_t id = gaia_base_t::find_first(T_container);
    while (id != common::c_invalid_gaia_id)
    {
        if (m_filter_fn)
        {
            if (m_filter_fn(T_class::get(id)))
            {
                return gaia_iterator_t<T_class>(id, m_filter_fn);
            }
        }
        else
        {
            break;
        }

        id = edc_db_t::find_next(id);
    }
    return gaia_iterator_t<T_class>(id);
}

// The where() method constructs a gaia_container_t with a std::function to be used for filtering.
template <gaia::common::gaia_type_t T_container, typename T_class>
gaia_container_t<T_container, T_class> gaia_container_t<T_container, T_class>::where(std::function<bool(const T_class&)> filter_function)
{
    return gaia_container_t<T_container, T_class>(filter_function);
}

template <gaia::common::gaia_type_t T_container, typename T_class>
gaia_iterator_t<T_class> gaia_container_t<T_container, T_class>::end() const
{
    return gaia_iterator_t<T_class>(0);
}

//
// Implementation for gaia_set_iterator_t.
//
template <typename T_child, size_t T_next_slot>
gaia_set_iterator_t<T_child, T_next_slot>::gaia_set_iterator_t(gaia::common::gaia_id_t id)
{
    m_child_obj = T_child::get(id);
}

template <typename T_child, size_t T_next_slot>
gaia_set_iterator_t<T_child, T_next_slot>::gaia_set_iterator_t(gaia::common::gaia_id_t id, std::function<bool(const T_child&)> filter_function)
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
        for (m_child_obj = T_child::get(m_child_obj.get_references()[T_next_slot]);
             m_child_obj;
             m_child_obj = T_child::get(m_child_obj.get_references()[T_next_slot]))
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
            m_child_obj = T_child::get(m_child_obj.get_references()[T_next_slot]);
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
gaia_set_iterator_t<T_child, T_next_slot> reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::begin() const
{
    gaia::common::gaia_id_t id = 0;

    if (m_parent_id)
    {
        id = gaia_base_t::get_reference(m_parent_id, T_child_slot);
        while (id)
        {
            if (m_filter_fn)
            {
                if (m_filter_fn(T_child::get(id)))
                {
                    break;
                }
                id = gaia_base_t::get_reference(id, T_next_slot);
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
reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::where(std::function<bool(const T_child&)> filter_function) const
{
    return reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>(m_parent_id, filter_function);
}

template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
gaia_set_iterator_t<T_child, T_next_slot> reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::end() const
{
    return gaia_set_iterator_t<T_child, T_next_slot>(0);
}

template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
void reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::insert(gaia::common::gaia_id_t child_id)
{
    // The gaia_id() will be zero if the row hasn't been inserted into the SE.
    if (child_id == 0 || m_parent_id == 0)
    {
        T_parent expected = T_parent::get(0);
        T_child received = T_child::get(0);
        throw edc_invalid_state(expected.gaia_typename(), received.gaia_typename());
    }

    gaia_base_t::insert_child_reference(m_parent_id, child_id, T_child_slot);
}

template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
void reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::insert(T_child& child_edc)
{
    insert(child_edc.gaia_id());
}

template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
void reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::erase(gaia::common::gaia_id_t child_id)
{
    gaia_base_t::remove_child_reference(m_parent_id, child_id, T_child_slot);
}

template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
void reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot>::erase(T_child& child_edc)
{
    erase(child_edc.gaia_id());
}

} // namespace direct_access
} // namespace gaia
