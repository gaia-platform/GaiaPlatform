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
template <typename T_child>
gaia_set_iterator_t<T_child>::gaia_set_iterator_t(gaia::common::gaia_id_t id, size_t const_next_slot)
    : m_const_next_slot(const_next_slot)
{
    m_child_obj = T_child::get(id);
}

template <typename T_child>
gaia_set_iterator_t<T_child>::gaia_set_iterator_t(gaia::common::gaia_id_t id, std::function<bool(const T_child&)> filter_function, size_t const_next_slot)
    : m_filter_fn(filter_function)
    , m_const_next_slot(const_next_slot)
{
    m_child_obj = T_child::get(id);
}

template <typename T_child>
typename gaia_set_iterator_t<T_child>::reference gaia_set_iterator_t<T_child>::operator*()
{
    return m_child_obj;
}

template <typename T_child>
typename gaia_set_iterator_t<T_child>::pointer gaia_set_iterator_t<T_child>::operator->()
{
    return &m_child_obj;
}

template <typename T_child>
gaia_set_iterator_t<T_child>& gaia_set_iterator_t<T_child>::operator++()
{
    if (!m_child_obj)
    {
        return *this;
    }

    if (m_filter_fn)
    {
        for (m_child_obj = T_child::get(m_child_obj.references()[m_const_next_slot]);
             m_child_obj;
             m_child_obj = T_child::get(m_child_obj.references()[m_const_next_slot]))
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
            m_child_obj = T_child::get(m_child_obj.references()[m_const_next_slot]);
        }
    }

    return *this;
}

template <typename T_child>
gaia_set_iterator_t<T_child> gaia_set_iterator_t<T_child>::operator++(int)
{
    gaia_set_iterator_t<T_child> old = *this;
    operator++();
    return old;
}

template <typename T_child>
bool gaia_set_iterator_t<T_child>::operator==(const gaia_set_iterator_t& rhs) const
{
    return m_child_obj == rhs.m_child_obj;
}

template <typename T_child>
bool gaia_set_iterator_t<T_child>::operator!=(const gaia_set_iterator_t& rhs) const
{
    return !(m_child_obj == rhs.m_child_obj);
}

//
// Implementation for reference_chain_container_t.
//
template <typename T_child>
gaia_set_iterator_t<T_child> reference_chain_container_t<T_child>::begin() const
{
    gaia::common::gaia_id_t id = 0;

    if (m_parent_id)
    {
        id = gaia_base_t::get_reference(m_parent_id, m_const_child_slot);
        while (id)
        {
            if (m_filter_fn)
            {
                if (m_filter_fn(T_child::get(id)))
                {
                    break;
                }
                id = gaia_base_t::get_reference(id, m_const_next_slot);
            }
            else
            {
                break;
            }
        }
    }
    return gaia_set_iterator_t<T_child>(id, m_filter_fn, m_const_next_slot);
}

// The where() method saves the address of the std::function, to be called for each candidate return value.
template <typename T_child>
reference_chain_container_t<T_child>
reference_chain_container_t<T_child>::where(std::function<bool(const T_child&)> filter_function) const
{
    return reference_chain_container_t<T_child>(m_parent_id, filter_function, m_const_next_slot, m_const_child_slot, m_const_next_slot);
}

template <typename T_child>
gaia_set_iterator_t<T_child> reference_chain_container_t<T_child>::end() const
{
    return gaia_set_iterator_t<T_child>(0, 0);
}

template <typename T_child>
void reference_chain_container_t<T_child>::insert(gaia::common::gaia_id_t child_id)
{
    // The gaia_id() will be zero if the row hasn't been inserted into the SE.
    if (child_id == 0 || m_parent_id == 0)
    {
        T_child received = T_child::get(0);
        throw edc_invalid_state(m_parent_id, child_id, received.gaia_typename());
    }

    gaia_base_t::insert_child_reference(m_parent_id, child_id, m_const_child_slot);
}

template <typename T_child>
void reference_chain_container_t<T_child>::insert(T_child& child_edc)
{
    insert(child_edc.gaia_id());
}

template <typename T_child>
void reference_chain_container_t<T_child>::erase(gaia::common::gaia_id_t child_id)
{
    gaia_base_t::remove_child_reference(m_parent_id, child_id, m_const_child_slot);
}

template <typename T_child>
void reference_chain_container_t<T_child>::erase(T_child& child_edc)
{
    erase(child_edc.gaia_id());
}

} // namespace direct_access
} // namespace gaia
