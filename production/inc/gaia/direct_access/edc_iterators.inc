/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

namespace gaia
{
namespace direct_access
{

//
// Implementation for edc_iterator_t.
//
template <typename T_class>
edc_iterator_t<T_class>::edc_iterator_t(gaia::common::gaia_id_t id)
{
    m_obj = T_class::get(id);
}

template <typename T_class>
edc_iterator_t<T_class>::edc_iterator_t(gaia::common::gaia_id_t id, std::function<bool(const T_class&)> filter_function)
    : m_filter_fn(filter_function)
{
    m_obj = T_class::get(id);
}

template <typename T_class>
edc_iterator_t<T_class>& edc_iterator_t<T_class>::operator++()
{
    // if (!m_obj)
    // {
    //     return *this;
    // }

    // if (m_filter_fn)
    // {
    //     for (m_obj = m_obj.get_next(); m_obj; m_obj = m_obj.get_next())
    //     {
    //         if (m_filter_fn(m_obj))
    //         {
    //             break;
    //         }
    //     }
    // }
    // else
    // {
    //     if (m_obj)
    //     {
    //         m_obj = m_obj.get_next();
    //     }
    // }

    return *this;
}

template <typename T_class>
edc_iterator_t<T_class> edc_iterator_t<T_class>::operator++(int)
{
    edc_iterator_t<T_class> old = *this;
    operator++();
    return old;
}

template <typename T_class>
bool edc_iterator_t<T_class>::operator==(const edc_iterator_t& rhs) const
{
    return m_obj == rhs.m_obj;
}

template <typename T_class>
bool edc_iterator_t<T_class>::operator!=(const edc_iterator_t& rhs) const
{
    return !(m_obj == rhs.m_obj);
}

template <typename T_class>
typename edc_iterator_t<T_class>::reference edc_iterator_t<T_class>::operator*()
{
    return m_obj;
}

template <typename T_class>
typename edc_iterator_t<T_class>::pointer edc_iterator_t<T_class>::operator->()
{
    return &m_obj;
}

//
// Implementation for edc_container_t
//

// The begin() method returns either the first element, or the first element that satisfies m_filter_fn.
template <gaia::common::gaia_type_t container_type, typename T_class>
edc_iterator_t<T_class> edc_container_t<container_type, T_class>::begin() const
{
    gaia::common::gaia_id_t id = common::c_invalid_gaia_id;
    // edc_base_t::find_first(container_type);
    // while (id != common::c_invalid_gaia_id)
    // {
    //     if (m_filter_fn)
    //     {
    //         if (m_filter_fn(T_class::get(id)))
    //         {
    //             return edc_iterator_t<T_class>(id, m_filter_fn);
    //         }
    //     }
    //     else
    //     {
    //         break;
    //     }

    //     id = edc_db_t::find_next(id);
    // }
    return edc_iterator_t<T_class>(id);
}

// The where() method constructs a edc_container_t with a std::function to be used for filtering.
template <gaia::common::gaia_type_t container_type, typename T_class>
edc_container_t<container_type, T_class> edc_container_t<container_type, T_class>::where(std::function<bool(const T_class&)> filter_function)
{
    return edc_container_t<container_type, T_class>(filter_function);
}

template <gaia::common::gaia_type_t container_type, typename T_class>
edc_iterator_t<T_class> edc_container_t<container_type, T_class>::end() const
{
    return edc_iterator_t<T_class>(gaia::common::c_invalid_gaia_id);
}

template <gaia::common::gaia_type_t container_type, typename T_class>
size_t edc_container_t<container_type, T_class>::size() const
{
    return std::distance(begin(), end());
}

//
// Implementation for edc_set_iterator_t.
//
template <typename T_child>
edc_set_iterator_t<T_child>::edc_set_iterator_t(gaia::common::gaia_id_t id, size_t next_offset)
    : m_next_offset(next_offset)
{
    m_child_obj = T_child::get(id);
}

template <typename T_child>
edc_set_iterator_t<T_child>::edc_set_iterator_t(gaia::common::gaia_id_t id, std::function<bool(const T_child&)> filter_function, size_t next_offset)
    : m_filter_fn(filter_function)
    , m_next_offset(next_offset)
{
    m_child_obj = T_child::get(id);
}

template <typename T_child>
typename edc_set_iterator_t<T_child>::reference edc_set_iterator_t<T_child>::operator*()
{
    return m_child_obj;
}

template <typename T_child>
typename edc_set_iterator_t<T_child>::pointer edc_set_iterator_t<T_child>::operator->()
{
    return &m_child_obj;
}

template <typename T_child>
edc_set_iterator_t<T_child>& edc_set_iterator_t<T_child>::operator++()
{
    if (!m_child_obj)
    {
        return *this;
    }

    if (m_filter_fn)
    {
        for (m_child_obj = T_child::get(m_child_obj.references()[m_next_offset]);
             m_child_obj;
             m_child_obj = T_child::get(m_child_obj.references()[m_next_offset]))
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
            m_child_obj = T_child::get(m_child_obj.references()[m_next_offset]);
        }
    }

    return *this;
}

template <typename T_child>
edc_set_iterator_t<T_child> edc_set_iterator_t<T_child>::operator++(int)
{
    edc_set_iterator_t<T_child> old = *this;
    operator++();
    return old;
}

template <typename T_child>
bool edc_set_iterator_t<T_child>::operator==(const edc_set_iterator_t& rhs) const
{
    return m_child_obj == rhs.m_child_obj;
}

template <typename T_child>
bool edc_set_iterator_t<T_child>::operator!=(const edc_set_iterator_t& rhs) const
{
    return !(m_child_obj == rhs.m_child_obj);
}

//
// Implementation for reference_chain_container_t.
//
template <typename T_child>
edc_set_iterator_t<T_child> reference_chain_container_t<T_child>::begin() const
{
    gaia::common::gaia_id_t id = gaia::common::c_invalid_gaia_id;

    if (m_parent_id != gaia::common::c_invalid_gaia_id)
    {
        id = edc_base_t::get_reference(m_parent_id, m_child_offset);
        while (id != gaia::common::c_invalid_gaia_id)
        {
            if (m_filter_fn)
            {
                if (m_filter_fn(T_child::get(id)))
                {
                    break;
                }
                id = edc_base_t::get_reference(id, m_next_offset);
            }
            else
            {
                break;
            }
        }
    }
    return edc_set_iterator_t<T_child>(id, m_filter_fn, m_next_offset);
}

// The where() method saves the address of the std::function, to be called for each candidate return value.
template <typename T_child>
reference_chain_container_t<T_child>
reference_chain_container_t<T_child>::where(std::function<bool(const T_child&)> filter_function) const
{
    return reference_chain_container_t<T_child>(m_parent_id, filter_function, m_child_offset, m_next_offset);
}

template <typename T_child>
edc_set_iterator_t<T_child> reference_chain_container_t<T_child>::end() const
{
    // REVIEW: replace the second argument with a constant.
    return edc_set_iterator_t<T_child>(gaia::common::c_invalid_gaia_id, 0);
}

template <typename T_child>
void reference_chain_container_t<T_child>::insert(gaia::common::gaia_id_t child_id)
{
    // The gaia_id() will be zero if the row hasn't been inserted into the SE.
    if (child_id == gaia::common::c_invalid_gaia_id || m_parent_id == gaia::common::c_invalid_gaia_id)
    {
        T_child received = T_child::get(gaia::common::c_invalid_gaia_id);
        throw edc_invalid_state(m_parent_id, child_id, received.gaia_typename());
    }

    edc_base_t::insert_child_reference(m_parent_id, child_id, m_child_offset);
}

template <typename T_child>
void reference_chain_container_t<T_child>::insert(T_child& child_edc)
{
    insert(child_edc.gaia_id());
}

template <typename T_child>
void reference_chain_container_t<T_child>::remove(gaia::common::gaia_id_t child_id)
{
    edc_base_t::remove_child_reference(m_parent_id, child_id, m_child_offset);
}

template <typename T_child>
void reference_chain_container_t<T_child>::remove(T_child& child_edc)
{
    remove(child_edc.gaia_id());
}

template <typename T_child>
edc_set_iterator_t<T_child> reference_chain_container_t<T_child>::erase(edc_set_iterator_t<T_child> position)
{
    auto current = *position;
    position++;
    remove(current);
    return position;
}

template <typename T_child>
void reference_chain_container_t<T_child>::clear()
{
    for (auto it = begin(); it != end();)
    {
        it = erase(it);
    }
}

template <typename T_child>
size_t reference_chain_container_t<T_child>::size() const
{
    return std::distance(begin(), end());
}

} // namespace direct_access
} // namespace gaia
