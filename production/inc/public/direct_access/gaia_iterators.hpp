/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

namespace gaia {

/**
 * \addtogroup Gaia
 * @{
 */

namespace direct_access {

/**
 * \addtogroup Direct
 * @{
 *
 * Implementation of Extended Data Classes. This provides a direct access API
 * for CRUD operations on the database.
 */


// C++17 compliant way when std::iterator is deprecated.
//
// A gaia_iterator_t contains the methods that satisfy an iterator interface.
// Only used from gaia_container_t template, which defines the begin() and end().
//   @tparam T_gaia_ptr a pointer to the Extended Data Class type
template<typename T_gaia>
class gaia_iterator_t {
    gaia_id_t m_id;
    T_gaia m_obj;
public:
    using difference_type = std::ptrdiff_t;
    using value_type = T_gaia;
    using pointer = T_gaia*;
    using reference = T_gaia&;
    using iterator_category = std::forward_iterator_tag;

    gaia_iterator_t(gaia_id_t id);
    gaia_iterator_t();

    gaia_iterator_t<T_gaia>& operator++();

    gaia_iterator_t<T_gaia> operator++(int);

    bool operator==(const gaia_iterator_t& rhs) const;

    bool operator!=(const gaia_iterator_t& rhs) const;

    reference operator*();

    pointer operator->();
};

// A gaia_container_t is defined within each EDC and used by programmers to scan
// through all instances of the type in the database.
//   @tparam T_gaia the Extended Data Class type
template<gaia_type_t T_gaia_type, typename T_gaia>
struct gaia_container_t {
    static gaia_iterator_t<T_gaia> begin();
    static gaia_iterator_t<T_gaia> end();
};

// A gaia_set_iterator_t is only used from reference_chain_container_t. It contains the methods that implement
// an iterator for scanning through the linked list forming a "set" between a parent ("primary")
// and multiple child ("foreign") instances of a type.
//   @tparam T_foreign the Extended Data Type that is in the child ("foreign") position in the set
//   @tparam T_foreign_slot an integer indexing the list of references in the T_foreign type
template <typename T_foreign, int T_foreign_slot>
class gaia_set_iterator_t {
    gaia_id_t m_id;
    T_foreign m_foreign_obj;
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = gaia_id_t;
    using difference_type = std::ptrdiff_t;
    using pointer = gaia_id_t*;
    using reference = gaia_id_t&;

    gaia_set_iterator_t(gaia_id_t id) : m_id(id) {}

    T_foreign operator*();

    T_foreign* operator->();

    gaia_set_iterator_t<T_foreign, T_foreign_slot>& operator++();

    gaia_set_iterator_t<T_foreign, T_foreign_slot> operator++(int);

    bool operator==(const gaia_set_iterator_t& rhs) const;

    bool operator!=(const gaia_set_iterator_t& rhs) const;
};

// A reference_chain_container_t is defined within each EDC that is a parent ("primary") in a "set" relationship.
// The relationship is represented by a chain of pointers. The parent points to the first child,
// and each child points to the next child, where a null pointer indicates the end of the chain.
// Each EDC contains a fixed number of reference slots of type gaia_id_t, to form the chains it
// is a member of. Any EDC may be parent of any number of sets, and a member of any number of sets.
// The catalog is aware of each set relationship and determines the number of slots required to be
// stored in an EDC instanced in order to hold the pointers. One reference slot is needed for each
// set owned by this EDC, and two slots are needed for each set of which this EDC is a member.
// Constants have been generated to identify the particular slot assigned for its role in a set.
//   @tparam T_primary the Extended Data Type that is in the parent ("primary") position in the set
//   @tparam T_foreign the Extended Data Type that is in the child ("foreign") position in the set
//   @tparam T_parent_slot an integer identifying the location of the parent pointer in the child reference list
//   @tparam T_primary_slot an integer identifying the location of the pointer to the child in the parent reference list
//   @tparam T_foreign_slot an integer identifying the location of the pointer to the next child in the child reference list
template <typename T_primary, typename T_foreign, int T_parent_slot, int T_primary_slot, int T_foreign_slot>
class reference_chain_container_t {
    gaia_id_t m_primary_id;
public:
    gaia_set_iterator_t<T_foreign, T_foreign_slot> begin();

    gaia_set_iterator_t<T_foreign, T_foreign_slot> end();

    void set_outer(gaia_id_t primary_id) {m_primary_id = primary_id;}

    void insert(gaia_id_t foreign_id);

    void insert(T_foreign& foreign_edc);

    void erase(gaia_id_t foreign_id);

    void erase(T_foreign& foreign_edc);
};

/*@}*/
} // direct_access
/*@}*/
} // gaia


// Pick up our template implementation.  These still
// need to be in the header so that template specializations
// that are declared later will pick up the definitions.
#include "gaia_iterators.inc"
