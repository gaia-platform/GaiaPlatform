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
gaia_id_t gaia_iterator_t<T_gaia>::operator++()
{
    if (!m_id)
    {
        return m_id;
    }

    auto ptr = gaia_ptr::open(m_id);
    if (!ptr)
    {
        m_id = 0;
        return m_id;
    }

    auto ptr_next = ptr.find_next();
    if (!ptr_next)
    {
        m_id = 0;
        return m_id;
    }

    m_id = ptr_next->id;

    return m_id;
}

template <typename T_gaia>
gaia_id_t gaia_iterator_t<T_gaia>::operator++(int) 
{
    gaia_id_t id = m_id;
    operator++();
    return id;
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
    return T_gaia(gaia_ptr::open(m_id));
}

//
// Implementation for gaia_container_t
//
template<gaia_type_t T_gaia_type, typename T_gaia>
gaia_iterator_t<T_gaia> gaia_container_t<T_gaia_type, T_gaia>::begin()
{
    auto node_ptr = gaia_ptr::find_first(T_gaia_type);
    if (node_ptr)
    {
        return node_ptr.id();
    }

    return 0;
}

template<gaia_type_t T_gaia_type, typename T_gaia>
gaia_iterator_t<T_gaia> gaia_container_t<T_gaia_type, T_gaia>::end()
{
    return 0;
}


//
// Implementation for gaia_set_iterator_t.
//
template <typename T_foreign, int T_foreign_slot>
T_foreign gaia_set_iterator_t<T_foreign, T_foreign_slot>::operator*()
{
    return T_foreign::get(m_id);
}

template<typename T_foreign, int T_foreign_slot>
gaia_id_t gaia_set_iterator_t<T_foreign, T_foreign_slot>::operator++()
{
    if (!m_id)
    {
        return m_id;
    }

    auto ptr = gaia_ptr::open(m_id);
    if (!ptr)
    {
        m_id = 0;
        return m_id;
    }

    m_id = ptr.references()[T_foreign_slot];

    return m_id;
}

template <typename T_foreign, int T_foreign_slot>
gaia_id_t gaia_set_iterator_t<T_foreign, T_foreign_slot>::operator++(int)
{
    gaia_id_t id = m_id;
    operator++();
    return id;
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
void reference_chain_container_t<T_primary, T_foreign, T_parent_slot, T_primary_slot, T_foreign_slot>::insert(T_foreign& foreign_edc)
{
    auto fid = foreign_edc.gaia_id();
    auto oid = m_primary_id;

    // The gaia_id() will be zero if the row hasn't been inserted into the SE.
    if (fid == 0 || oid == 0) {
        T_primary expected = T_primary::get(0);
        throw edc_unstored_row(expected.gaia_typename(), foreign_edc.gaia_typename());
    }

    auto node_ptr = gaia_ptr::open(oid);
    if (!node_ptr)
    {
        return;
    }
    foreign_edc.references()[T_foreign_slot] = node_ptr.references()[T_primary_slot];
    foreign_edc.references()[T_parent_slot]  = oid;
    node_ptr.references()[T_primary_slot] = fid;
}

template <typename T_primary, typename T_foreign, int T_parent_slot, int T_primary_slot, int T_foreign_slot>
void reference_chain_container_t<T_primary, T_foreign, T_parent_slot, T_primary_slot, T_foreign_slot>::erase(T_foreign& foreign_edc)
{
    if (foreign_edc.references()[T_parent_slot] != m_primary_id) 
    {
        T_primary expected = T_primary::get(0);
        throw edc_invalid_member(
            m_primary_id,
            T_primary::s_gaia_type,
            expected.gaia_typename(),
            foreign_edc.gaia_type(),
            foreign_edc.gaia_typename());
    }
    gaia_id_t foreign_id = foreign_edc.gaia_id();

    auto node_ptr = gaia_ptr::open(m_primary_id);
    if (!node_ptr)
    {
        return;
    }

    if (node_ptr.references()[T_primary_slot] == foreign_id) 
    {
        // It's the first one in the list, point the "first" to the current "next".
        node_ptr.references()[T_primary_slot] = foreign_edc.references()[T_foreign_slot];
        // Clean up the removed child.
        foreign_edc.references()[T_foreign_slot] = 0;
        foreign_edc.references()[T_parent_slot] = 0;
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
                cur_child_ptr.references()[T_foreign_slot] = foreign_edc.references()[T_foreign_slot];
                // Clean up the removed child.
                foreign_edc.references()[T_foreign_slot] = 0;
                foreign_edc.references()[T_parent_slot] = 0;
                return;
            }
            // Move to the next child.
            cur_child_ptr = gaia_ptr::open(next_id);
        }
        // If we end up here, the child was not found in the chain. This is an error because
        // the pointers have become inconsistent (the child's parent pointer was correct).
        T_primary expected = T_primary::get(0);

        throw edc_inconsistent_list(
                    m_primary_id,
                    expected.gaia_typename(),
                    foreign_id,
                    foreign_edc.gaia_typename());
    }
}

} // direct_access
} // gaia
