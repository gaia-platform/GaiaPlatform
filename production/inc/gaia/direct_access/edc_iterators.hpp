/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/direct_access/edc_base.hpp"

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

template <gaia::common::gaia_type_t container_type_id, typename T_class>
class edc_container_t;

// C++17 compliant way when std::iterator is deprecated.
//
// An edc_iterator_t contains the methods that satisfy an iterator interface.
// Only used from edc_container_t template, which defines the begin(), where() and end().
//
// @tparam T_class the Extended Data Class
template <typename T_class>
class edc_iterator_t : protected edc_db_t
{
    template <gaia::common::gaia_type_t container_type_id, typename T_whatever_class>
    friend class edc_container_t;

public:
    using difference_type = std::ptrdiff_t;
    using value_type = T_class;
    using pointer = T_class*;
    using reference = T_class&;
    using iterator_category = std::forward_iterator_tag;

    edc_iterator_t() = default;

    edc_iterator_t<T_class>& operator++();
    edc_iterator_t<T_class> operator++(int);
    bool operator==(const edc_iterator_t& rhs) const;
    bool operator!=(const edc_iterator_t& rhs) const;
    reference operator*();
    pointer operator->();

protected:
    explicit edc_iterator_t(std::shared_ptr<edc_base_iterator_state_t> iterator_state);
    explicit edc_iterator_t(
        std::shared_ptr<edc_base_iterator_state_t> iterator_state,
        std::function<bool(const T_class&)> filter_function);
    explicit edc_iterator_t(gaia::common::gaia_id_t id);

protected:
    std::shared_ptr<edc_base_iterator_state_t> m_iterator_state;
    T_class m_obj;
    std::function<bool(const T_class&)> m_filter_fn;
};

// A edc_container_t is all objects of the same Extended Data Class in the database.
//
// @tparam container_type_id the type identifier of Extended Data Class
// @tparam T_class the class of the Extended Data Class
template <gaia::common::gaia_type_t container_type_id, typename T_class>
class edc_container_t : protected edc_db_t
{
public:
    // This constructor will be used by the where() method to create a filtered container.
    explicit edc_container_t(std::function<bool(const T_class&)> filter_function)
        : m_filter_fn(filter_function){};

    edc_container_t() = default;

    edc_iterator_t<T_class> begin() const;
    edc_iterator_t<T_class> end() const;

    size_t size() const;

    static edc_container_t<container_type_id, T_class> where(std::function<bool(const T_class&)>);

private:
    std::function<bool(const T_class&)> m_filter_fn;
};

// A edc_set_iterator_t is only used from reference_chain_container_t. It
// contains the methods that implement an iterator for scanning through the
// linked list forming a "set" between a parent and multiple child instances of
// a class.
//
// @tparam T_child the Extended Data Class that is in the child position in the set
template <typename T_child>
class edc_set_iterator_t : protected edc_db_t
{
public:
    using difference_type = std::ptrdiff_t;
    using value_type = T_child;
    using pointer = T_child*;
    using reference = T_child&;
    using iterator_category = std::forward_iterator_tag;

    explicit edc_set_iterator_t(gaia::common::gaia_id_t id, size_t next_offset);
    explicit edc_set_iterator_t(gaia::common::gaia_id_t id, std::function<bool(const T_child&)> filter_function, size_t next_offset);
    edc_set_iterator_t() = default;

    reference operator*();
    pointer operator->();
    edc_set_iterator_t<T_child>& operator++();
    edc_set_iterator_t<T_child> operator++(int);
    bool operator==(const edc_set_iterator_t& rhs) const;
    bool operator!=(const edc_set_iterator_t& rhs) const;

private:
    T_child m_child_obj;
    std::function<bool(const T_child&)> m_filter_fn;
    size_t m_next_offset;
};

// A reference_chain_container_t is defined within each EDC that is a parent in
// a "set" relationship. The relationship is represented by a chain of pointers.
// The parent points to the first child, and each child points to the next
// child, where a null pointer indicates the end of the chain. Each EDC contains
// a fixed number of reference slots of type gaia::common::gaia_id_t, to form the chains it is
// a member of. Any EDC may be parent of any number of sets, and a member of any
// number of sets. The catalog is aware of each set relationship and determines
// the number of slots required to be stored in an EDC instance in order to
// hold the pointers. One reference slot is needed for each set owned by this
// EDC, and two slots are needed for each set of which this EDC is a member.
// Constants have been generated to identify the particular slot assigned for
// its role in a set.
//
// @tparam T_child the Extended Data Class that is in the child position in the set
template <typename T_child>
class reference_chain_container_t : protected edc_db_t
{
public:
    // This constructor will be used by the where() method to create a filtered container.
    explicit reference_chain_container_t(gaia::common::gaia_id_t parent, std::function<bool(const T_child&)> filter_function, size_t child_offset, size_t next_offset)
        : m_parent_id(parent)
        , m_filter_fn(filter_function)
        , m_child_offset(child_offset)
        , m_next_offset(next_offset){};

    explicit reference_chain_container_t(gaia::common::gaia_id_t parent, size_t child_offset, size_t next_offset)
        : m_parent_id(parent)
        , m_child_offset(child_offset)
        , m_next_offset(next_offset){};

    // reference_chain_container_t is copied from the EDC list methods.
    reference_chain_container_t(const reference_chain_container_t&) = default;
    reference_chain_container_t& operator=(const reference_chain_container_t&) = default;

    edc_set_iterator_t<T_child> begin() const;
    edc_set_iterator_t<T_child> end() const;

    size_t size() const;

    void insert(gaia::common::gaia_id_t child_id);
    void insert(T_child& child_edc);

    edc_set_iterator_t<T_child> erase(edc_set_iterator_t<T_child> position);
    void remove(gaia::common::gaia_id_t child_id);
    void remove(T_child& child_edc);
    void clear();

    reference_chain_container_t<T_child> where(std::function<bool(const T_child&)>) const;

private:
    gaia::common::gaia_id_t m_parent_id{gaia::common::c_invalid_gaia_id};
    std::function<bool(const T_child&)> m_filter_fn{};
    size_t m_child_offset;
    size_t m_next_offset;
};

/*@}*/
} // namespace direct_access
/*@}*/
} // namespace gaia

// Pick up our template implementation. These still need to be in the header so
// that template specializations that are declared later will pick up the
// definitions.
#include "edc_iterators.inc"
