/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

//
// Implementation for dac_iterator_t.
//
template <typename T_class>
dac_iterator_t<T_class>::dac_iterator_t(std::shared_ptr<dac_base_iterator_state_t> iterator_state)
    : m_iterator_state(std::move(iterator_state))
{
    gaia::common::gaia_id_t id = dac_db_t::get_iterator_value(m_iterator_state);
    m_obj = (id.is_valid()) ? T_class::get(id) : T_class();
}

template <typename T_class>
dac_iterator_t<T_class>::dac_iterator_t(
    std::shared_ptr<dac_base_iterator_state_t> iterator_state,
    std::function<common::optional_t<bool>(const T_class&)> filter_function)
    : m_iterator_state(std::move(iterator_state)), m_filter_fn(filter_function)
{
    gaia::common::gaia_id_t id = dac_db_t::get_iterator_value(m_iterator_state);
    m_obj = (id.is_valid()) ? T_class::get(id) : T_class();
}

template <typename T_class>
dac_iterator_t<T_class>::dac_iterator_t(gaia::common::gaia_id_t id)
{
    m_obj = (id.is_valid()) ? T_class::get(id) : T_class();
}

template <typename T_class>
dac_iterator_t<T_class>& dac_iterator_t<T_class>::operator++()
{
    if (!m_obj)
    {
        return *this;
    }

    gaia::common::gaia_id_t id = m_obj.gaia_id();
    while (id.is_valid())
    {
        dac_db_t::advance_iterator(m_iterator_state);
        id = dac_db_t::get_iterator_value(m_iterator_state);
        m_obj = (id.is_valid()) ? T_class::get(id) : T_class();

        // Check m_filter_fn first which is much cheaper than dac_object_t:::operator bool().
        if (m_filter_fn && m_obj)
        {
            common::optional_t<bool> predicate_value = m_filter_fn(m_obj);
            if (!predicate_value.has_value() || !predicate_value.value())
            {
                continue;
            }
        }

        break;
    }

    return *this;
}

template <typename T_class>
dac_iterator_t<T_class> dac_iterator_t<T_class>::operator++(int)
{
    auto old = *this;
    operator++();
    return old;
}

template <typename T_class>
bool dac_iterator_t<T_class>::operator==(const dac_iterator_t& rhs) const
{
    return m_obj == rhs.m_obj;
}

template <typename T_class>
bool dac_iterator_t<T_class>::operator!=(const dac_iterator_t& rhs) const
{
    return !(m_obj == rhs.m_obj);
}

template <typename T_class>
typename dac_iterator_t<T_class>::reference dac_iterator_t<T_class>::operator*()
{
    return m_obj;
}

template <typename T_class>
typename dac_iterator_t<T_class>::pointer dac_iterator_t<T_class>::operator->()
{
    return &m_obj;
}

//
// Implementation for dac_container_t
//

// The begin() method returns either the first element, or the first element that satisfies m_filter_fn.
template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class>
dac_iterator_t<T_class> dac_container_t<container_type_id, T_class>::begin() const
{
    std::shared_ptr<dac_base_iterator_state_t> iterator_state
        = dac_db_t::initialize_iterator(container_type_id);

    gaia::common::gaia_id_t id;
    while ((id = dac_db_t::get_iterator_value(iterator_state)).is_valid())
    {
        if (!m_filter_fn)
        {
            break;
        }

        // The id has already been validated,
        // so we must be able to retrieve a valid object instance with it.
        T_class obj = T_class::get(id);

        auto predicate_value = m_filter_fn(obj);
        if (predicate_value.has_value() && predicate_value.value())
        {
            return dac_iterator_t<T_class>(iterator_state, m_filter_fn);
        }

        dac_db_t::advance_iterator(iterator_state);
    }

    return dac_iterator_t<T_class>(iterator_state);
}

// The where() method constructs a dac_container_t with a std::function to be used for filtering.
template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class>
dac_container_t<container_type_id, T_class> dac_container_t<container_type_id, T_class>::where(
    std::function<common::optional_t<bool>(const T_class&)> filter_function)
{
    return dac_container_t<container_type_id, T_class>(filter_function);
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class>
dac_iterator_t<T_class> dac_container_t<container_type_id, T_class>::end() const
{
    return dac_iterator_t<T_class>(gaia::common::c_invalid_gaia_id);
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class>
size_t dac_container_t<container_type_id, T_class>::size() const
{
    return std::distance(begin(), end());
}

//
// Implementation for dac_set_iterator_t.
//
template <typename T_child>
dac_set_iterator_t<T_child>::dac_set_iterator_t(gaia::common::gaia_id_t id, common::reference_offset_t next_offset)
    : m_next_offset(next_offset)
{
    m_child_obj = (id.is_valid()) ? T_child::get(id) : T_child();
}

template <typename T_child>
dac_set_iterator_t<T_child>::dac_set_iterator_t(
    gaia::common::gaia_id_t id,
    std::function<common::optional_t<bool>(const T_child&)> filter_function,
    common::reference_offset_t next_offset)
    : m_filter_fn(filter_function)
    , m_next_offset(next_offset)
{
    m_child_obj = (id.is_valid()) ? T_child::get(id) : T_child();
}

template <typename T_child>
typename dac_set_iterator_t<T_child>::reference dac_set_iterator_t<T_child>::operator*()
{
    return m_child_obj;
}

template <typename T_child>
typename dac_set_iterator_t<T_child>::pointer dac_set_iterator_t<T_child>::operator->()
{
    return &m_child_obj;
}

template <typename T_child>
dac_set_iterator_t<T_child>& dac_set_iterator_t<T_child>::operator++()
{
    if (!m_child_obj)
    {
        return *this;
    }

    gaia::common::gaia_id_t id;
    if (m_filter_fn)
    {
        for (id = m_child_obj.references()[m_next_offset], m_child_obj = (id.is_valid()) ? T_child::get(id) : T_child();
             m_child_obj;
             id = m_child_obj.references()[m_next_offset], m_child_obj = (id.is_valid()) ? T_child::get(id) : T_child())
        {
            auto predicate_value = m_filter_fn(m_child_obj);
            if (predicate_value.has_value() && predicate_value.value())
            {
                break;
            }
        }
    }
    else
    {
        if (m_child_obj)
        {
            id = m_child_obj.references()[m_next_offset];
            m_child_obj = (id.is_valid()) ? T_child::get(id) : T_child();
        }
    }

    return *this;
}

template <typename T_child>
dac_set_iterator_t<T_child> dac_set_iterator_t<T_child>::operator++(int)
{
    dac_set_iterator_t<T_child> old = *this;
    operator++();
    return old;
}

template <typename T_child>
bool dac_set_iterator_t<T_child>::operator==(const dac_set_iterator_t& rhs) const
{
    return m_child_obj == rhs.m_child_obj;
}

template <typename T_child>
bool dac_set_iterator_t<T_child>::operator!=(const dac_set_iterator_t& rhs) const
{
    return !(m_child_obj == rhs.m_child_obj);
}

//
// Implementation for value_linked_reference_container_t.
//
template <typename T_child>
dac_set_iterator_t<T_child> value_linked_reference_container_t<T_child>::begin() const
{
    gaia::common::gaia_id_t id = dac_db_t::get_reference(m_anchor_id, gaia::common::c_ref_anchor_first_child_offset);
    gaia::common::reference_offset_t next_offset = m_anchor_offset + 1;
    while (id.is_valid())
    {
        if (m_filter_fn)
        {
            auto predicate_value = m_filter_fn(T_child::get(id));
            if (predicate_value.has_value() && predicate_value.value())
            {
                break;
            }
            id = dac_db_t::get_reference(id, next_offset);
        }
        else
        {
            break;
        }
    }
    return dac_set_iterator_t<T_child>(id, m_filter_fn, next_offset);
}

template <typename T_child>
value_linked_reference_container_t<T_child>
value_linked_reference_container_t<T_child>::where(std::function<common::optional_t<bool>(const T_child&)> filter_function) const
{
    return value_linked_reference_container_t<T_child>(m_anchor_id, m_anchor_offset, filter_function);
}

template <typename T_child>
dac_set_iterator_t<T_child> value_linked_reference_container_t<T_child>::end() const
{
    return dac_set_iterator_t<T_child>(gaia::common::c_invalid_gaia_id, common::c_invalid_reference_offset);
}

template <typename T_child>
size_t value_linked_reference_container_t<T_child>::size() const
{
    return std::distance(begin(), end());
}

//
// Implementation for reference_container_t.
//
template <typename T_child>
dac_set_iterator_t<T_child> reference_container_t<T_child>::begin() const
{
    if (!m_parent_id.is_valid())
    {
        return end_dac_set_iterator();
    }

    gaia::common::gaia_id_t anchor_id = dac_db_t::get_reference(m_parent_id, m_child_offset);
    if (!anchor_id.is_valid())
    {
        return end_dac_set_iterator();
    }

    gaia::common::gaia_id_t id = dac_db_t::get_reference(anchor_id, gaia::common::c_ref_anchor_first_child_offset);
    if (!m_filter_fn)
    {
        return dac_set_iterator_t<T_child>(id, m_next_offset);
    }

    while (id.is_valid())
    {
        auto predicate_value = m_filter_fn(T_child::get(id));
        if (predicate_value.has_value() && predicate_value.value())
        {
            break;
        }
        id = dac_db_t::get_reference(id, m_next_offset);
    }

    return dac_set_iterator_t<T_child>(id, m_filter_fn, m_next_offset);
}

template <typename T_child>
reference_container_t<T_child>
reference_container_t<T_child>::where(std::function<common::optional_t<bool>(const T_child&)> filter_function) const
{
    return reference_container_t<T_child>(m_parent_id, filter_function, m_child_offset, m_next_offset);
}

template <typename T_child>
dac_set_iterator_t<T_child> reference_container_t<T_child>::end() const
{
    return end_dac_set_iterator();
}

template <typename T_child>
void reference_container_t<T_child>::insert(gaia::common::gaia_id_t id)
{
    connect(id);
}

template <typename T_child>
void reference_container_t<T_child>::insert(const T_child& child)
{
    insert(child.gaia_id());
}

template <typename T_child>
bool reference_container_t<T_child>::connect(gaia::common::gaia_id_t id)
{
    if (!id.is_valid() || !m_parent_id.is_valid())
    {
        report_invalid_object_id(id);
    }
    return dac_db_t::insert_into_reference_container(m_parent_id, id, m_child_offset);
}

template <typename T_child>
bool reference_container_t<T_child>::connect(const T_child& child)
{
    return connect(child.gaia_id());
}

template <typename T_child>
void reference_container_t<T_child>::remove(gaia::common::gaia_id_t id)
{
    disconnect(id);
}

template <typename T_child>
void reference_container_t<T_child>::remove(const T_child& child)
{
    remove(child.gaia_id());
}

template <typename T_child>
bool reference_container_t<T_child>::disconnect(gaia::common::gaia_id_t id)
{
    return dac_db_t::remove_from_reference_container(m_parent_id, id, m_child_offset);
}

template <typename T_child>
bool reference_container_t<T_child>::disconnect(const T_child& child)
{
    return disconnect(child.gaia_id());
}

template <typename T_child>
dac_set_iterator_t<T_child> reference_container_t<T_child>::erase(dac_set_iterator_t<T_child> position)
{
    auto current = *position;
    position++;
    remove(current);
    return position;
}

template <typename T_child>
void reference_container_t<T_child>::clear()
{
    for (auto it = begin(); it != end();)
    {
        it = erase(it);
    }
}

template <typename T_child>
size_t reference_container_t<T_child>::size() const
{
    return std::distance(begin(), end());
}
