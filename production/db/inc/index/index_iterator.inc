/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

template <typename T_structure, typename T_index_it>
index_iterator_t<T_structure, T_index_it>::index_iterator_t(base_index_t* db_idx, T_index_it it, T_index_it end_it)
{
    m_db_idx = db_idx;
    m_index_it = it;
    m_end_it = end_it;

    if (m_db_idx != nullptr)
    {
        m_db_idx->get_lock().lock();
    }
}

template <typename T_structure, typename T_index_it>
index_iterator_t<T_structure, T_index_it>::index_iterator_t(const index_iterator_t& other)
{
    m_db_idx = other.m_db_idx;
    m_index_it = other.m_index_it;
    m_end_it = other.m_end_it;

    if (m_db_idx != nullptr)
    {
        m_db_idx->get_lock().lock();
    }
}

template <typename T_structure, typename T_index_it>
index_iterator_t<T_structure, T_index_it>& index_iterator_t<T_structure, T_index_it>::operator=(const index_iterator_t& other)
{
    if (this != &other)
    {
        m_db_idx = other.m_db_idx;
        m_index_it = other.m_index_it;
        m_end_it = other.m_end_it;

        if (m_db_idx != nullptr)
        {
            m_db_idx->get_lock().lock();
        }
    }

    return *this;
}

template <typename T_structure, typename T_index_it>
index_iterator_t<T_structure, T_index_it>::~index_iterator_t()
{
    if (m_db_idx != nullptr)
    {
        m_db_idx->get_lock().unlock();
    }
}

template <typename T_structure, typename T_index_it>
index_iterator_t<T_structure, T_index_it>& index_iterator_t<T_structure, T_index_it>::operator++()
{
    ++m_index_it;
    return *this;
}

template <typename T_structure, typename T_index_it>
index_iterator_t<T_structure, T_index_it> index_iterator_t<T_structure, T_index_it>::operator++(int)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

template <typename T_structure, typename T_index_it>
index_iterator_t<T_structure, T_index_it>& index_iterator_t<T_structure, T_index_it>::next_key() const
{
    // Already at the end, return immediately.
    if (m_index_it == m_end_it)
    {
        return *this;
    }

    auto key = m_index_it->first;

    do
    {
        ++m_index_it;
    } while (m_index_it != m_end_it && m_index_it->first == key);

    return *this;
}

template <typename T_structure, typename T_index_it>
const typename T_index_it::value_type& index_iterator_t<T_structure, T_index_it>::operator*() const
{
    return *m_index_it;
}

template <typename T_structure, typename T_index_it>
const typename T_index_it::value_type* index_iterator_t<T_structure, T_index_it>::operator->() const
{
    return &(*m_index_it);
}

template <typename T_structure, typename T_index_it>
bool index_iterator_t<T_structure, T_index_it>::operator==(const index_iterator_t& other) const
{
    return m_index_it == other.m_index_it;
}

template <typename T_structure, typename T_index_it>
bool index_iterator_t<T_structure, T_index_it>::operator!=(const index_iterator_t& other) const
{
    return !(*this == other);
}
