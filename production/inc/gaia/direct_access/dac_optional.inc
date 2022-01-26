/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

template <typename T_value>
optional_t<T_value>::optional_storage_t::optional_storage_t()
    : dummy(0)
{
}

template <typename T_value>
optional_t<T_value>::optional_storage_t::optional_storage_t(T_value value)
    : val(value), m_has_value(true)
{
}

template <typename T_value>
optional_t<T_value>::optional_t()
    : m_storage()
{
}

template <typename T_value>
optional_t<T_value>::optional_t(const T_value& t)
    : m_storage(t)
{
}

template <typename T_value>
void optional_t<T_value>::reset()
{
    m_storage = optional_storage_t();
}

template <typename T_value>
T_value& optional_t<T_value>::value() &
{
    return m_storage.m_has_value ? m_storage.val : throw optional_value_not_found();
}

template <typename T_value>
const T_value& optional_t<T_value>::value() const&
{
    return m_storage.m_has_value ? m_storage.val : throw optional_value_not_found();
}

template <typename T_value>
T_value& optional_t<T_value>::operator*() &
{
    return m_storage.m_has_value ? m_storage.val : throw optional_value_not_found();
}

template <typename T_value>
const T_value& optional_t<T_value>::operator*() const&
{
    return m_storage.m_has_value ? m_storage.val : throw optional_value_not_found();
}

template <typename T_value>
template <typename T_default>
T_value optional_t<T_value>::value_or(T_default&& default_value) const&
{
    return m_storage.m_has_value ? m_storage.val : default_value;
}

template <typename T_value>
bool optional_t<T_value>::has_value() const
{
    return m_storage.m_has_value;
}
