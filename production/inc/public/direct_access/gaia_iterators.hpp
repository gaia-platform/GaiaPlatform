/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

namespace gaia
{

/**
 * \addtogroup Gaia
 * @{
 */

namespace direct_access
{

/**
 * \addtogroup Direct
 * @{
 *
 * Implementation of Extended Data Classes. This provides a direct access API
 * for CRUD operations on the database, plus navigation between objects through
 * linked lists called "sets".
 */

// C++17 compliant way when std::iterator is deprecated.
//
// A gaia_iterator_t contains the methods that satisfy an iterator interface.
// Only used from gaia_container_t template, which defines the begin(), where() and end().
//
// @tparam T_class the Extended Data Class
template <typename T_class>
class gaia_iterator_t
{
    T_class m_obj;
    std::function<bool(const T_class&)> m_filter_fn;

public:
    using difference_type = std::ptrdiff_t;
    using value_type = T_class;
    using pointer = T_class*;
    using reference = T_class&;
    using iterator_category = std::forward_iterator_tag;

    explicit gaia_iterator_t(gaia_id_t id);
    gaia_iterator_t(gaia_id_t id, std::function<bool(const T_class&)> filter_function);
    gaia_iterator_t() = default;

    gaia_iterator_t<T_class>& operator++();

    gaia_iterator_t<T_class> operator++(int);

    bool operator==(const gaia_iterator_t& rhs) const;

    bool operator!=(const gaia_iterator_t& rhs) const;

    reference operator*();

    pointer operator->();
};

// A gaia_container_t is all objects of the same Extended Data Class in the database.
//
// @tparam T_container the type identifier of Extended Data Class
// @tparam T_class the class of the Extended Data Class
template <gaia_type_t T_container, typename T_class>
struct gaia_container_t
{
    // This constructor will be used by the where() method to create a filtered container.
    gaia_container_t(std::function<bool(const T_class&)> filter_function)
        : m_filter_fn(filter_function)
    {
    }
    gaia_container_t() = default;

    gaia_iterator_t<T_class> begin();
    static gaia_container_t<T_container, T_class> where(std::function<bool(const T_class&)>);
    static gaia_iterator_t<T_class> end();

private:
    std::function<bool(const T_class&)> m_filter_fn;
};

// A gaia_set_iterator_t is only used from reference_chain_container_t. It
// contains the methods that implement an iterator for scanning through the
// linked list forming a "set" between a parent and multiple child instances of
// a class.
//
// @tparam T_child the Extended Data Class that is in the child position in the set
// @tparam T_next_slot index into the child's reference list of the next child in the set
template <typename T_child, size_t T_next_slot>
class gaia_set_iterator_t
{
    T_child m_child_obj;
    std::function<bool(const T_child&)> m_filter_fn;

public:
    using difference_type = std::ptrdiff_t;
    using value_type = T_child;
    using pointer = T_child*;
    using reference = T_child&;
    using iterator_category = std::forward_iterator_tag;

    explicit gaia_set_iterator_t(gaia_id_t id);
    gaia_set_iterator_t(gaia_id_t id, std::function<bool(const T_child&)> filter_function);
    gaia_set_iterator_t() = default;

    reference operator*();

    pointer operator->();

    gaia_set_iterator_t<T_child, T_next_slot>& operator++();

    gaia_set_iterator_t<T_child, T_next_slot> operator++(int);

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
// the number of slots required to be stored in an EDC instance in order to
// hold the pointers. One reference slot is needed for each set owned by this
// EDC, and two slots are needed for each set of which this EDC is a member.
// Constants have been generated to identify the particular slot assigned for
// its role in a set.
//
// @tparam T_parent the Extended Data Class that is in the parent position in the set
// @tparam T_child the Extended Data Class that is in the child position in the set
// @tparam T_parent_slot index into the child's reference list of the parent pointer
// @tparam T_child_slot index into the parent's reference list of the first child in the set
// @tparam T_next_slot index into the child's reference list of the next child in the set
template <typename T_parent, typename T_child, size_t T_parent_slot, size_t T_child_slot, size_t T_next_slot>
class reference_chain_container_t
{
    gaia_id_t m_parent_id = 0;
    std::function<bool(const T_child&)> m_filter_fn;

public:
    // This constructor will be used by the where() method to create a filtered container.
    explicit reference_chain_container_t(gaia_id_t parent, std::function<bool(const T_child&)> filter_function)
        : m_parent_id(parent), m_filter_fn(filter_function)
    {
    }
    reference_chain_container_t() = default;

    // EDC returns copies of reference_chain_container_t, hence a copy-constructor is necessary.
    // Even though the compiler would generate it, this is a reminder to improve the copy constructor
    // if the class grows in complexity.
    reference_chain_container_t(const reference_chain_container_t&) = default;
    reference_chain_container_t& operator=(const reference_chain_container_t&) = default;

    gaia_set_iterator_t<T_child, T_next_slot> begin();

    reference_chain_container_t<T_parent, T_child, T_parent_slot, T_child_slot, T_next_slot> where(std::function<bool(const T_child&)>);

    gaia_set_iterator_t<T_child, T_next_slot> end();

    void set_outer(gaia_id_t parent_id)
    {
        m_parent_id = parent_id;
    }

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
