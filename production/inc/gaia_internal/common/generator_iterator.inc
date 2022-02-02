/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

template <typename T_output>
std::optional<T_output> generator_t<T_output>::operator()()
{
    return m_function();
}

template <typename T_output>
generator_iterator_t<T_output>::~generator_iterator_t()
{
    if (m_generator)
    {
        m_generator->cleanup();
    }
}

template <typename T_output>
const T_output& generator_iterator_t<T_output>::operator*() const
{
    // If we de-reference m_state via *m_state, we can run into undefined behavior
    // if m_state does not contain a value, so we'll use the value() method instead,
    // which throws an exception in that case.
    return m_state.value();
}

template <typename T_output>
const T_output* generator_iterator_t<T_output>::operator->() const
{
    return m_state ? &m_state.value() : nullptr;
}

template <typename T_output>
generator_iterator_t<T_output>& generator_iterator_t<T_output>::operator++()
{
    while ((m_state = (*m_generator)()))
    {
        if (m_predicate(*m_state))
        {
            break;
        }
    }
    return *this;
}

template <typename T_output>
generator_iterator_t<T_output> generator_iterator_t<T_output>::operator++(int)
{
    auto old = *this;
    operator++();
    return old;
}

template <typename T_output>
generator_iterator_t<T_output>::operator bool() const noexcept
{
    return m_state.has_value();
}

template <typename T_output>
void generator_iterator_t<T_output>::init_generator()
{
    // We need to initialize the iterator to the first valid state.
    while ((m_state = (*m_generator)()))
    {
        if (m_predicate(*m_state))
        {
            break;
        }
    }
}

template <typename T_output>
const generator_iterator_t<T_output>& generator_range_t<T_output>::begin() const
{
    return begin_it;
}

template <typename T_output>
const std::nullopt_t generator_range_t<T_output>::end() const
{
    return std::nullopt;
}
