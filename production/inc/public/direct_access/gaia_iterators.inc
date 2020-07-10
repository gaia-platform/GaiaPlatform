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
template <typename T_gaia_ptr>
gaia_iterator_t<T_gaia_ptr>::gaia_iterator_t(T_gaia_ptr ptr)
: m_ptr(ptr)
{
}

template <typename T_gaia_ptr>
gaia_iterator_t<T_gaia_ptr>::gaia_iterator_t(const gaia_iterator_t& it)
: m_ptr(it.m_ptr)
{
}

template <typename T_gaia_ptr>
gaia_iterator_t<T_gaia_ptr>& gaia_iterator_t<T_gaia_ptr>::operator++()
{
    m_ptr = m_ptr->get_next();
    return *this;
}

template <typename T_gaia_ptr>
T_gaia_ptr gaia_iterator_t<T_gaia_ptr>::operator++(int) 
{
    T_gaia_ptr temp = m_ptr;
    operator++();
    return temp;
}

template <typename T_gaia_ptr>
bool gaia_iterator_t<T_gaia_ptr>::operator==(const gaia_iterator_t& rhs) const 
{
    return m_ptr->gaia_id() == rhs.m_ptr->gaia_id();
}

template <typename T_gaia_ptr>
bool gaia_iterator_t<T_gaia_ptr>::operator!=(const gaia_iterator_t& rhs) const 
{
    if (m_ptr && rhs.m_ptr) 
    {
        return m_ptr->gaia_id() != rhs.m_ptr->gaia_id();
    }
    return (m_ptr != rhs.m_ptr);
}

//
// Implementation for gaia_set_iterator_t.
//
template<typename T_foreign, int T_foreign_slot>
gaia_set_iterator_t<T_foreign, T_foreign_slot>& gaia_set_iterator_t<T_foreign, T_foreign_slot>::operator++()
{
    m_edc_ptr = T_foreign::get_row_by_id(m_edc_ptr->m_references[T_foreign_slot]);
    return *this;
}

//
// Implementation for reference_chain_container_t.
//
template <typename T_primary, typename T_foreign, int T_parent_slot, int T_primary_slot, int T_foreign_slot>
gaia_set_iterator_t<T_foreign, T_foreign_slot> reference_chain_container_t<T_primary, T_foreign, T_parent_slot, T_primary_slot, T_foreign_slot>::begin()
{
    T_foreign* edc_ptr = T_foreign::get_row_by_id(m_outer->m_references[T_primary_slot]);
    return gaia_set_iterator_t<T_foreign, T_foreign_slot>(edc_ptr);
}

template <typename T_primary, typename T_foreign, int T_parent_slot, int T_primary_slot, int T_foreign_slot>
void reference_chain_container_t<T_primary, T_foreign, T_parent_slot, T_primary_slot, T_foreign_slot>::insert(T_foreign* foreign_ptr)
{
    auto fid = foreign_ptr->gaia_id();
    auto oid = m_outer->gaia_id();
    // The gaia_id() will be zero if the row hasn't been inserted into the SE.
    if (fid == 0 || oid == 0) {
        throw edc_unstored_row(m_outer->gaia_typename(), foreign_ptr->gaia_typename());
    }
    foreign_ptr->m_references[T_foreign_slot] = m_outer->m_references[T_primary_slot];
    foreign_ptr->m_references[T_parent_slot]  = oid;
    m_outer->m_references[T_primary_slot] = fid;
}

template <typename T_primary, typename T_foreign, int T_parent_slot, int T_primary_slot, int T_foreign_slot>
void reference_chain_container_t<T_primary, T_foreign, T_parent_slot, T_primary_slot, T_foreign_slot>::erase(T_foreign* foreign_ptr)
{
    if (foreign_ptr->m_references[T_parent_slot] != m_outer->gaia_id()) 
    {
        throw edc_invalid_member(
            m_outer->gaia_id(),
            m_outer->gaia_type(),
            m_outer->gaia_typename(),
            foreign_ptr->gaia_type(),
            foreign_ptr->gaia_typename());
    }
    auto foreign_id = foreign_ptr->gaia_id();
    if (m_outer->m_references[T_primary_slot] == foreign_id) 
    {
        // It's the first one in the list, point the "first" to the current "next".
        m_outer->m_references[T_primary_slot] = foreign_ptr->m_references[T_foreign_slot];
        // Clean up the removed child.
        foreign_ptr->m_references[T_foreign_slot] = 0;
        foreign_ptr->m_references[T_parent_slot] = 0;
    }
    else 
    {
        // Need to scan the list to find this one because it's not first on the list.
        auto cur_child = T_foreign::get_row_by_id(m_outer->m_references[T_primary_slot]);
        while (cur_child->m_references[T_foreign_slot]) 
        {
            auto next_id = cur_child->m_references[T_foreign_slot];
            if (next_id == foreign_id) 
            {
                // Point the current child to the child following the next.
                cur_child->m_references[T_foreign_slot] = foreign_ptr->m_references[T_foreign_slot];
                // Clean up the removed child.
                foreign_ptr->m_references[T_foreign_slot] = 0;
                foreign_ptr->m_references[T_parent_slot] = 0;
                return;
            }
            // Move to the next child.
            cur_child = T_foreign::get_row_by_id(next_id);
        }
        // If we end up here, the child was not found in the chain. This is an error because
        // the pointers have become inconsistent (the child's parent pointer was correct).
        throw edc_inconsistent_list(
                    m_outer->gaia_id(),
                    m_outer->gaia_typename(),
                    foreign_id,
                    foreign_ptr->gaia_typename());
    }
}

} // direct_access
} // gaia
