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
template<typename T_gaia_ptr>
class gaia_iterator_t {
    T_gaia_ptr m_ptr;
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T_gaia_ptr;
    using difference_type = int;
    using pointer = T_gaia_ptr*;
    using reference = T_gaia_ptr&;

    gaia_iterator_t(T_gaia_ptr ptr);

    gaia_iterator_t(const gaia_iterator_t& it);

    gaia_iterator_t& operator++();

    T_gaia_ptr operator++(int);

    bool operator==(const gaia_iterator_t& rhs) const;

    bool operator!=(const gaia_iterator_t& rhs) const;

    T_gaia_ptr& operator*() { return m_ptr; }
};

// A gaia_container_t is defined within each EDC and used by programmers to scan
// through all instances of the type in the database.
//   @tparam T_gaia the Extended Data Class type
template<typename T_gaia>
struct gaia_container_t {
    static gaia_iterator_t<T_gaia*> begin() { return gaia_iterator_t<T_gaia*>(T_gaia::get_first()); }
    static gaia_iterator_t<T_gaia*> end() { return gaia_iterator_t<T_gaia*>(nullptr); }
};

// A gaia_set_iterator_t is only used from reference_chain_container_t. It contains the methods that implement
// an iterator for scanning through the linked list forming a "set" between a parent ("primary")
// and multiple child ("foreign") instances of a type.
//   @tparam T_foreign the Extended Data Type that is in the child ("foreign") position in the set
//   @tparam T_foreign_slot an integer indexing the list of references in the T_foreign type
template <typename T_foreign, int T_foreign_slot>
class gaia_set_iterator_t {
    T_foreign* m_edc_ptr;
public:
    gaia_set_iterator_t() {}
    gaia_set_iterator_t(T_foreign* edc_ptr) { m_edc_ptr = edc_ptr; }

    T_foreign* operator*() { return m_edc_ptr; }

    gaia_set_iterator_t& operator++();

    bool operator!=(const gaia_set_iterator_t&) const { return m_edc_ptr != nullptr; }
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
//   @tparam T_foreign_slot an integer indenifying the location of the pointer to the next child in the child reference list
template <typename T_primary, typename T_foreign, int T_parent_slot, int T_primary_slot, int T_foreign_slot>
class reference_chain_container_t {
    T_primary* m_outer;
public:
    gaia_set_iterator_t<T_foreign, T_foreign_slot> begin();

    gaia_set_iterator_t<T_foreign, T_foreign_slot> end() {return gaia_set_iterator_t<T_foreign, T_foreign_slot>(nullptr);}

    void set_outer(T_primary* outer) {m_outer = outer;}

    void insert(T_foreign* foreign_ptr);

    void erase(T_foreign* foreign_ptr);
};

/*@}*/
} // direct_access
/*@}*/
} // gaia


// Pick up our template implementation.  These still
// need to be in the header so that template specializations
// that are declared later will pick up the definitions.
#include "gaia_iterators.inc"
