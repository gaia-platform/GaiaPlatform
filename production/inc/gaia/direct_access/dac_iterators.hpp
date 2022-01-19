/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <functional>
#include <utility>

#include "gaia/common.hpp"
#include "gaia/direct_access/dac_base.hpp"
#include "gaia/exceptions.hpp"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

namespace gaia
{
/**
 * \addtogroup gaia
 * @{
 */
namespace direct_access
{
/**
 * \addtogroup direct_access
 * @{
 *
 * Implementation of Direct Access Classes. This provides a direct access API
 * for CRUD operations on the database, plus navigation between objects through
 * linked lists called "sets".
 */

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class>
class dac_container_t;

// C++17 compliant way when std::iterator is deprecated.
//
// An dac_iterator_t contains the methods that satisfy an iterator interface.
// Only used from dac_container_t template, which defines the begin(), where() and end().
//
// @tparam T_class the Direct Access Class
template <typename T_class>
class dac_iterator_t : protected dac_db_t
{
    template <gaia::common::gaia_type_t::value_type container_type_id, typename T_whatever_class>
    friend class dac_container_t;

public:
    using difference_type = std::ptrdiff_t;
    using value_type = T_class;
    using pointer = T_class*;
    using reference = T_class&;
    using iterator_category = std::forward_iterator_tag;

    dac_iterator_t() = default;

    dac_iterator_t<T_class>& operator++();
    dac_iterator_t<T_class> operator++(int);
    bool operator==(const dac_iterator_t& rhs) const;
    bool operator!=(const dac_iterator_t& rhs) const;
    reference operator*();
    pointer operator->();

protected:
    explicit dac_iterator_t(std::shared_ptr<dac_base_iterator_state_t> iterator_state);
    explicit dac_iterator_t(
        std::shared_ptr<dac_base_iterator_state_t> iterator_state,
        std::function<bool(const T_class&)> filter_function);
    explicit dac_iterator_t(gaia::common::gaia_id_t id);

protected:
    std::shared_ptr<dac_base_iterator_state_t> m_iterator_state;
    T_class m_obj;
    std::function<bool(const T_class&)> m_filter_fn;
};

// A dac_container_t is all objects of the same Direct Access Class in the database.
//
// @tparam container_type_id the type identifier of Direct Access Class
// @tparam T_class the class of the Direct Access Class
template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class>
class dac_container_t : protected dac_db_t
{
public:
    // This constructor will be used by the where() method to create a filtered container.
    explicit dac_container_t(std::function<bool(const T_class&)> filter_function)
        : m_filter_fn(filter_function){};

    dac_container_t() = default;

    dac_iterator_t<T_class> begin() const;
    dac_iterator_t<T_class> end() const;

    size_t size() const;

    static dac_container_t<container_type_id, T_class> where(std::function<bool(const T_class&)>);

private:
    std::function<bool(const T_class&)> m_filter_fn;
};

// A dac_set_iterator_t is only used from reference_chain_container_t. It
// contains the methods that implement an iterator for scanning through the
// linked list forming a "set" between a parent and multiple child instances of
// a class.
//
// @tparam T_child the Direct Access Class that is in the child position in the set
template <typename T_child>
class dac_set_iterator_t : protected dac_db_t
{
public:
    using difference_type = std::ptrdiff_t;
    using value_type = T_child;
    using pointer = T_child*;
    using reference = T_child&;
    using iterator_category = std::forward_iterator_tag;

    explicit dac_set_iterator_t(gaia::common::gaia_id_t id, common::reference_offset_t next_offset);
    explicit dac_set_iterator_t(gaia::common::gaia_id_t id, std::function<bool(const T_child&)> filter_function, common::reference_offset_t next_offset);
    dac_set_iterator_t() = default;

    reference operator*();
    pointer operator->();
    dac_set_iterator_t<T_child>& operator++();
    dac_set_iterator_t<T_child> operator++(int);
    bool operator==(const dac_set_iterator_t& rhs) const;
    bool operator!=(const dac_set_iterator_t& rhs) const;

private:
    T_child m_child_obj;
    std::function<bool(const T_child&)> m_filter_fn;
    common::reference_offset_t m_next_offset;
};

template <typename T_child>
class value_linked_reference_container_t : protected dac_db_t
{
public:
    explicit value_linked_reference_container_t(
        gaia::common::gaia_id_t anchor_id,
        common::reference_offset_t anchor_offset,
        std::function<bool(const T_child&)> filter_function)
        : m_anchor_id(anchor_id), m_anchor_offset(anchor_offset), m_filter_fn(filter_function)
    {
    }

    explicit value_linked_reference_container_t(
        gaia::common::gaia_id_t anchor_id,
        common::reference_offset_t anchor_offset)
        : m_anchor_id(anchor_id), m_anchor_offset(anchor_offset)
    {
    }

    value_linked_reference_container_t(const value_linked_reference_container_t&) = default;
    value_linked_reference_container_t& operator=(const value_linked_reference_container_t&) = default;

    dac_set_iterator_t<T_child> begin() const;
    dac_set_iterator_t<T_child> end() const;

    size_t size() const;

    value_linked_reference_container_t<T_child> where(std::function<bool(const T_child&)> filter_function) const;

private:
    gaia::common::gaia_id_t m_anchor_id{gaia::common::c_invalid_gaia_id};
    common::reference_offset_t m_anchor_offset;
    std::function<bool(const T_child&)> m_filter_fn{};
};

template <typename T_child>
class reference_container_t : protected dac_db_t
{
public:
    explicit reference_container_t(
        gaia::common::gaia_id_t parent,
        std::function<bool(const T_child&)> filter_function,
        common::reference_offset_t child_offset,
        common::reference_offset_t next_offset)
        : m_parent_id(parent)
        , m_filter_fn(filter_function)
        , m_child_offset(child_offset)
        , m_next_offset(next_offset){};

    explicit reference_container_t(
        gaia::common::gaia_id_t parent,
        common::reference_offset_t child_offset,
        common::reference_offset_t next_offset)
        : m_parent_id(parent)
        , m_child_offset(child_offset)
        , m_next_offset(next_offset){};

    reference_container_t(const reference_container_t&) = default;
    reference_container_t& operator=(const reference_container_t&) = default;

    dac_set_iterator_t<T_child> begin() const;
    dac_set_iterator_t<T_child> end() const;

    size_t size() const;

    void insert(gaia::common::gaia_id_t child_id);
    void insert(const T_child& child_edc);
    bool connect(gaia::common::gaia_id_t child_id);
    bool connect(const T_child& child_edc);

    dac_set_iterator_t<T_child> erase(dac_set_iterator_t<T_child> position);
    void remove(gaia::common::gaia_id_t child_id);
    void remove(const T_child& child_edc);
    bool disconnect(gaia::common::gaia_id_t child_id);
    bool disconnect(const T_child& child_edc);
    void clear();

    reference_container_t<T_child> where(std::function<bool(const T_child&)>) const;

private:
    gaia::common::gaia_id_t m_parent_id{gaia::common::c_invalid_gaia_id};
    std::function<bool(const T_child&)> m_filter_fn{};
    common::reference_offset_t m_child_offset;
    common::reference_offset_t m_next_offset;

    inline dac_set_iterator_t<T_child> end_dac_set_iterator() const
    {
        return dac_set_iterator_t<T_child>(gaia::common::c_invalid_gaia_id, common::c_invalid_reference_offset);
    }
};

// Pick up our template implementation. These still need to be in the header so
// that template specializations that are declared later will pick up the
// definitions.
#include "dac_iterators.inc"

/*@}*/
} // namespace direct_access
/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
