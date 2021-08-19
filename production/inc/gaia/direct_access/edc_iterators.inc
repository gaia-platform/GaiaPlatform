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
edc_iterator_t<T_class>::edc_iterator_t(std::shared_ptr<edc_base_iterator_state_t> iterator_state)
    : m_iterator_state(iterator_state)
{
    gaia::common::gaia_id_t id = edc_db_t::get_iterator_value(m_iterator_state);
    m_obj = T_class::get(id);
}

template <typename T_class>
edc_iterator_t<T_class>::edc_iterator_t(
    std::shared_ptr<edc_base_iterator_state_t> iterator_state,
    std::function<bool(const T_class&)> filter_function)
    : m_iterator_state(iterator_state), m_filter_fn(filter_function)
{
    gaia::common::gaia_id_t id = edc_db_t::get_iterator_value(m_iterator_state);
    m_obj = T_class::get(id);
}

template <typename T_class>
edc_iterator_t<T_class>::edc_iterator_t(gaia::common::gaia_id_t id)
{
    m_obj = T_class::get(id);
}

template <typename T_class>
edc_iterator_t<T_class>& edc_iterator_t<T_class>::operator++()
{
    if (!m_obj)
    {
        return *this;
    }

    gaia::common::gaia_id_t id = m_obj.gaia_id();
    while (id != gaia::common::c_invalid_gaia_id)
    {
        edc_db_t::advance_iterator(m_iterator_state);
        id = edc_db_t::get_iterator_value(m_iterator_state);
        m_obj = T_class::get(id);

        // If we have an object and a filter function and the object doesn't satisfy the filter,
        // then keep looking for another object that does.
        if (m_obj && m_filter_fn && !m_filter_fn(m_obj))
        {
            continue;
        }

        break;
    }

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
template <gaia::common::gaia_type_t container_type_id, typename T_class>
edc_iterator_t<T_class> edc_container_t<container_type_id, T_class>::begin() const
{
    std::shared_ptr<edc_base_iterator_state_t> iterator_state
        = edc_db_t::initialize_iterator(container_type_id);

    gaia::common::gaia_id_t id;
    while ((id = edc_db_t::get_iterator_value(iterator_state)) != common::c_invalid_gaia_id)
    {
        if (m_filter_fn)
        {
            T_class obj = T_class::get(id);
            if (obj && m_filter_fn(obj))
            {
                return edc_iterator_t<T_class>(iterator_state, m_filter_fn);
            }
        }
        else
        {
            break;
        }

        edc_db_t::advance_iterator(iterator_state);
    }

    return edc_iterator_t<T_class>(iterator_state);
}

// The where() method constructs a edc_container_t with a std::function to be used for filtering.
template <gaia::common::gaia_type_t container_type_id, typename T_class>
edc_container_t<container_type_id, T_class> edc_container_t<container_type_id, T_class>::where(
    std::function<bool(const T_class&)> filter_function)
{
    return edc_container_t<container_type_id, T_class>(filter_function);
}

template <gaia::common::gaia_type_t container_type_id, typename T_class>
edc_iterator_t<T_class> edc_container_t<container_type_id, T_class>::end() const
{
    return edc_iterator_t<T_class>(gaia::common::c_invalid_gaia_id);
}

template <gaia::common::gaia_type_t container_type_id, typename T_class>
size_t edc_container_t<container_type_id, T_class>::size() const
{
    return std::distance(begin(), end());
}

//
// Implementation for edc_set_iterator_t.
//
template <typename T_child>
edc_set_iterator_t<T_child>::edc_set_iterator_t(gaia::common::gaia_id_t id, common::reference_offset_t next_offset)
    : m_next_offset(next_offset)
{
    m_child_obj = T_child::get(id);
}

template <typename T_child>
edc_set_iterator_t<T_child>::edc_set_iterator_t(gaia::common::gaia_id_t id, std::function<bool(const T_child&)> filter_function, common::reference_offset_t next_offset)
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
        id = edc_db_t::get_reference(m_parent_id, m_child_offset);
        while (id != gaia::common::c_invalid_gaia_id)
        {
            if (m_filter_fn)
            {
                if (m_filter_fn(T_child::get(id)))
                {
                    break;
                }
                id = edc_db_t::get_reference(id, m_next_offset);
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
    return edc_set_iterator_t<T_child>(gaia::common::c_invalid_gaia_id, common::c_invalid_reference_offset);
}

template <typename T_child>
void reference_chain_container_t<T_child>::insert(gaia::common::gaia_id_t child_id)
{
    // The gaia_id() will be zero if the row hasn't been inserted into the SE.
    if (child_id == gaia::common::c_invalid_gaia_id || m_parent_id == gaia::common::c_invalid_gaia_id)
    {
        throw edc_invalid_state(m_parent_id, child_id, T_child::gaia_typename());
    }

    edc_db_t::insert_child_reference(m_parent_id, child_id, m_child_offset);
}

template <typename T_child>
void reference_chain_container_t<T_child>::insert(const T_child& child_edc)
{
    insert(child_edc.gaia_id());
}

template <typename T_child>
void reference_chain_container_t<T_child>::remove(gaia::common::gaia_id_t child_id)
{
    edc_db_t::remove_child_reference(m_parent_id, child_id, m_child_offset);
}

template <typename T_child>
void reference_chain_container_t<T_child>::remove(const T_child& child_edc)
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
