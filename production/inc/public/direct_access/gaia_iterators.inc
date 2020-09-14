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
template <typename T_gaia>
gaia_iterator_t<T_gaia>& gaia_iterator_t<T_gaia>::operator++()
{
    if (!m_id)
    {
        return *this;
    }

    auto ptr = gaia_ptr::open(m_id);
    if (!ptr)
    {
        m_id = 0;
        return *this;
    }

    auto ptr_next = ptr.find_next();
    if (!ptr_next)
    {
        m_id = 0;
        return *this;
    }

    m_id = ptr_next.id();
    return *this;
}

template <typename T_gaia>
gaia_iterator_t<T_gaia> gaia_iterator_t<T_gaia>::operator++(int)
{
    gaia_iterator_t<T_gaia> old(m_id);
    operator++();
    return old;
}

template <typename T_gaia>
bool gaia_iterator_t<T_gaia>::operator==(const gaia_iterator_t& rhs) const
{
    return m_id == rhs.m_id;
}

template <typename T_gaia>
bool gaia_iterator_t<T_gaia>::operator!=(const gaia_iterator_t& rhs) const
{
    return m_id != rhs.m_id;
}

template <typename T_gaia>
T_gaia gaia_iterator_t<T_gaia>::operator*()
{
    return T_gaia::get(m_id);
}

template <typename T_gaia>
T_gaia* gaia_iterator_t<T_gaia>::operator->()
{
    m_obj = T_gaia::get(m_id);
    return &m_obj;
}

//
// Implementation for gaia_container_t
//
template<gaia_type_t T_gaia_type, typename T_gaia>
gaia_iterator_t<T_gaia> gaia_container_t<T_gaia_type, T_gaia>::begin()
{
    gaia_id_t id = 0;
    auto node_ptr = gaia_ptr::find_first(T_gaia_type);
    if (node_ptr)
    {
        id = node_ptr.id();
    }

    return gaia_iterator_t<T_gaia>(id);
}

template<gaia_type_t T_gaia_type, typename T_gaia>
gaia_iterator_t<T_gaia> gaia_container_t<T_gaia_type, T_gaia>::end()
{
    return gaia_iterator_t<T_gaia>(0);
}


//
// Implementation for gaia_set_iterator_t.
//
template <typename T_foreign, int T_foreign_slot>
T_foreign gaia_set_iterator_t<T_foreign, T_foreign_slot>::operator*()
{
    return T_foreign::get(m_id);
}

template <typename T_foreign, int T_foreign_slot>
T_foreign* gaia_set_iterator_t<T_foreign, T_foreign_slot>::operator->()
{
    m_foreign_obj = T_foreign::get(m_id);
    return &m_foreign_obj;
}

template<typename T_foreign, int T_foreign_slot>
gaia_set_iterator_t<T_foreign, T_foreign_slot>& gaia_set_iterator_t<T_foreign, T_foreign_slot>::operator++()
{
    if (!m_id)
    {
        return *this;
    }

    auto ptr = gaia_ptr::open(m_id);
    if (!ptr)
    {
        m_id = 0;
        return *this;
    }

    m_id = ptr.references()[T_foreign_slot];
    return *this;
}

template <typename T_foreign, int T_foreign_slot>
gaia_set_iterator_t<T_foreign, T_foreign_slot> gaia_set_iterator_t<T_foreign, T_foreign_slot>::operator++(int)
{
    gaia_set_iterator_t<T_foreign, T_foreign_slot> old(m_id);
    operator++();
    return old;
}

template <typename T_foreign, int T_foreign_slot>
bool gaia_set_iterator_t<T_foreign, T_foreign_slot>::operator==(const gaia_set_iterator_t& rhs) const
{
    return m_id == rhs.m_id;
}

template <typename T_foreign, int T_foreign_slot>
bool gaia_set_iterator_t<T_foreign, T_foreign_slot>::operator!=(const gaia_set_iterator_t& rhs) const
{
    return m_id != rhs.m_id;
}


//
// Implementation for reference_chain_container_t.
//
template <typename T_primary, typename T_foreign, int T_parent_slot, int T_primary_slot, int T_foreign_slot>
gaia_set_iterator_t<T_foreign, T_foreign_slot> reference_chain_container_t<T_primary, T_foreign, T_parent_slot, T_primary_slot, T_foreign_slot>::begin()
{
    gaia_id_t id = 0;

    if (m_primary_id)
    {
        auto node_ptr = gaia_ptr::open(m_primary_id);
        if (node_ptr)
        {
            id = node_ptr.references()[T_primary_slot];
        }
    }
    return gaia_set_iterator_t<T_foreign, T_foreign_slot>(id);
}

template <typename T_primary, typename T_foreign, int T_parent_slot, int T_primary_slot, int T_foreign_slot>
gaia_set_iterator_t<T_foreign, T_foreign_slot> reference_chain_container_t<T_primary, T_foreign, T_parent_slot, T_primary_slot, T_foreign_slot>::end()
{
    return gaia_set_iterator_t<T_foreign, T_foreign_slot>(0);
}

template <typename T_primary, typename T_foreign, int T_parent_slot, int T_primary_slot, int T_foreign_slot>
void reference_chain_container_t<T_primary, T_foreign, T_parent_slot, T_primary_slot, T_foreign_slot>::insert(gaia_id_t foreign_id)
{
    auto oid = m_primary_id;

    // The gaia_id() will be zero if the row hasn't been inserted into the SE.
    if (foreign_id == 0 || oid == 0) {
        T_primary expected = T_primary::get(0);
        T_foreign received = T_foreign::get(0);
        throw edc_invalid_state(expected.gaia_typename(), received.gaia_typename());
    }

    auto node_ptr = gaia_ptr::open(oid);
    if (!node_ptr)
    {
        return;
    }

    auto foreign_ptr = gaia_ptr::open(foreign_id);
    if (!foreign_ptr)
    {
        return;
    }

    // This is a no-op if it is already connected to this owner.
    if (foreign_ptr.references()[T_parent_slot] == oid) {
        return;
    }

    // Cannot connect a foreign object that is already connected to a differrent owner.
    if (foreign_ptr.references()[T_parent_slot] != 0 && foreign_ptr.references()[T_parent_slot] != oid) {
        T_primary expected = T_primary::get(0);
        throw edc_already_inserted(foreign_ptr.references()[T_parent_slot], expected.gaia_typename());
    }

    foreign_ptr.references()[T_foreign_slot] = node_ptr.references()[T_primary_slot];
    foreign_ptr.references()[T_parent_slot]  = oid;
    node_ptr.references()[T_primary_slot] = foreign_id;

    node_ptr.log_in_place_update();
    foreign_ptr.log_in_place_update();
}

template <typename T_primary, typename T_foreign, int T_parent_slot, int T_primary_slot, int T_foreign_slot>
void reference_chain_container_t<T_primary, T_foreign, T_parent_slot, T_primary_slot, T_foreign_slot>::insert(T_foreign& foreign_edc)
{
    insert(foreign_edc.gaia_id());
}

template <typename T_primary, typename T_foreign, int T_parent_slot, int T_primary_slot, int T_foreign_slot>
void reference_chain_container_t<T_primary, T_foreign, T_parent_slot, T_primary_slot, T_foreign_slot>::erase(gaia_id_t foreign_id)
{
    auto foreign_ptr = gaia_ptr::open(foreign_id);
    if (!foreign_ptr)
    {
        return;
    }

    if (foreign_ptr.references()[T_parent_slot] != m_primary_id)
    {
        T_primary expected = T_primary::get(0);
        T_foreign received = T_foreign::get(0);
        throw edc_invalid_member(
            m_primary_id,
            T_primary::s_gaia_type,
            expected.gaia_typename(),
            received.gaia_type(),
            received.gaia_typename());
    }

    auto node_ptr = gaia_ptr::open(m_primary_id);
    if (!node_ptr)
    {
        return;
    }

    if (node_ptr.references()[T_primary_slot] == foreign_id)
    {
        // It's the first one in the list, point the "first" to the current "next".
        node_ptr.references()[T_primary_slot] = foreign_ptr.references()[T_foreign_slot];
        // Clean up the removed child.
        foreign_ptr.references()[T_foreign_slot] = 0;
        foreign_ptr.references()[T_parent_slot] = 0;

        node_ptr.log_in_place_update();
        foreign_ptr.log_in_place_update();
    }
    else
    {
        // Need to scan the list to find this one because it's not first on the list.
        auto cur_child_ptr = gaia_ptr::open(node_ptr.references()[T_primary_slot]);

        //auto cur_child = T_foreign::get(m_outer.references()[T_primary_slot]);
        while (cur_child_ptr && cur_child_ptr.references()[T_foreign_slot])
        {
            gaia_id_t next_id = cur_child_ptr.references()[T_foreign_slot];
            if (next_id == foreign_id)
            {
                // Point the current child to the child following the next.
                cur_child_ptr.references()[T_foreign_slot] = foreign_ptr.references()[T_foreign_slot];
                // Clean up the removed child.
                foreign_ptr.references()[T_foreign_slot] = 0;
                foreign_ptr.references()[T_parent_slot] = 0;

                node_ptr.log_in_place_update();
                foreign_ptr.log_in_place_update();
                return;
            }
            // Move to the next child.
            cur_child_ptr = gaia_ptr::open(next_id);
        }
        // If we end up here, the child was not found in the chain. This is an error because
        // the pointers have become inconsistent (the child's parent pointer was correct).
        throw edc_inconsistent_list(
                    m_primary_id,
                    T_primary::get(0).gaia_typename(),
                    foreign_id,
                    T_foreign::get(0).gaia_typename());
    }
}

template <typename T_primary, typename T_foreign, int T_parent_slot, int T_primary_slot, int T_foreign_slot>
void reference_chain_container_t<T_primary, T_foreign, T_parent_slot, T_primary_slot, T_foreign_slot>::erase(T_foreign& foreign_edc)
{
    erase(foreign_edc.gaia_id());
}

} // direct_access
} // gaia
