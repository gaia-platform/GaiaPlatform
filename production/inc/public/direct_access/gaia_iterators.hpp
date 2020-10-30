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
//
// @tparam T_class the Extended Data Class type
template <typename T_class>
class gaia_iterator_t {
    T_class m_obj;

public:
    using difference_type = std::ptrdiff_t;
    using value_type = T_class;
    using pointer = T_class*;
    using reference = T_class&;
    using iterator_category = std::forward_iterator_tag;

    gaia_iterator_t(gaia_id_t id);
    gaia_iterator_t() = default;

    gaia_iterator_t<T_class>& operator++();

    gaia_iterator_t<T_class> operator++(int);

    bool operator==(const gaia_iterator_t& rhs) const;

    bool operator!=(const gaia_iterator_t& rhs) const;

    reference operator*();

    pointer operator->();
};

// A gaia_container_t is defined within each EDC and used by programmers to scan
// through all instances of the type in the database.
//
// @tparam T_container the type identifier of Extended Data Class
// @tparam T_class the class of the Extended Data Class
template <gaia_type_t T_container, typename T_class>
struct gaia_container_t {
    static gaia_iterator_t<T_class> begin();
    static gaia_iterator_t<T_class> end();
};

// A gaia_set_iterator_t is only used from reference_chain_container_t. It
// contains the methods that implement an iterator for scanning through the
// linked list forming a "set" between a parent and multiple child instances of
// a type.
//
// @tparam T_child the Extended Data Type that is in the child position in the set
// @tparam T_child_slot an integer indexing the list of references in the T_child type
template <typename T_child, size_t T_child_slot>
class gaia_set_iterator_t {
    T_child m_child_obj;

public:
    using difference_type = std::ptrdiff_t;
    using value_type = T_child;
    using pointer = T_child*;
    using reference = T_child&;
    using iterator_category = std::forward_iterator_tag;

    gaia_set_iterator_t(gaia_id_t id);
    gaia_set_iterator_t() = default;

    reference operator*();

    pointer operator->();

    gaia_set_iterator_t<T_child, T_child_slot>& operator++();

    gaia_set_iterator_t<T_child, T_child_slot> operator++(int);

    bool operator==(const gaia_set_iterator_t& rhs) const;

    bool operator!=(const gaia_set_iterator_t& rhs) const;
};

// A reference_chain_container_t is defined within each EDC that is a parent in
// a "set" relationship. The relationship is represented by a chain of pointers.
// The parent points to the first child, and each child points to the next
// child, where a null pointer indicates the end of the chain. Each EDC contains
// a fixed number of reference slots of type gaia_id_t, to form the chains it is
// a member of. Any EDC may be parent of any number of sets, and a member of any
// number of sets. The catalog is aware of each set relationship and determines
// the number of slots required to be stored in an EDC instanced in order to
// hold the pointers. One reference slot is needed for each set owned by this
// EDC, and two slots are needed for each set of which this EDC is a member.
// Constants have been generated to identify the particular slot assigned for
// its role in a set.
//
// @tparam T_parent the Extended Data Type that is in the parent position in the set
// @tparam T_child the Extended Data Type that is in the child position in the set
// @tparam T_parent_slot an integer identifying the location of the parent pointer in the child reference list
// @tparam T_child_slot an integer identifying the location of the pointer to the (first) child in the parent reference list
// @tparam T_next_slot an integer identifying the location of the pointer to the next (child) in the child reference list
template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
class reference_chain_container_t {
    gaia_id_t m_parent_id;

  public:
    gaia_set_iterator_t<T_child, T_next_slot> begin();

    gaia_set_iterator_t<T_child, T_next_slot> end();

    void set_outer(gaia_id_t parent_id) { m_parent_id = parent_id; }

    void insert(gaia_id_t child_id);

    void insert(T_child& child_edc);

    void erase(gaia_id_t child_id);

    void erase(T_child& child_edc);
};

/*@}*/
} // namespace direct_access
/*@}*/
} // namespace gaia

// Pick up our template implementation. These still need to be in the header so
// that template specializations that are declared later will pick up the
// definitions.
#include "gaia_iterators.inc"
