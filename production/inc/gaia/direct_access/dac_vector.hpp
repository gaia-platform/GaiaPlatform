/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <type_traits>

#include <flatbuffers/flatbuffers.h>

#include "gaia/common.hpp"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

namespace gaia
{
/**
 * @addtogroup gaia
 * @{
 */
namespace direct_access
{
/**
 * @addtogroup direct_access
 * @{
 *
 * Provides an API for accessing array fields in Direct Access objects.
 */

// Forward declaring so `dac_vector_const_iterator_t` can use it.
template <typename T_type>
class dac_vector_t;

/**
 * A read-only iterator for traversing elements in a `dac_vector_t`.
 *
 * @tparam T_type The type of `dac_vector_t` elements that the iterator traverses.
 */
template <typename T_type>
class dac_vector_const_iterator_t
{
public:
    using difference_type = std::ptrdiff_t;
    using value_type = T_type;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = std::random_access_iterator_tag;

    /**
     * Constructs an iterator that does not point to any data.
     */
    dac_vector_const_iterator_t() = default;

    /**
     * Constructs an iterator pointing to a DAC vector, starting at a given index.
     * @param dac_vector The DAC vector to iterate over.
     * @param index The vector index to start iterating from.
     */
    explicit dac_vector_const_iterator_t(const dac_vector_t<T_type>& dac_vector, uint32_t index);

    // Comparing iterators from different containers is undefined behavior.

    bool operator==(const dac_vector_const_iterator_t& rhs) const;
    bool operator!=(const dac_vector_const_iterator_t& rhs) const;

    reference operator*() const;
    pointer operator->() const;

    dac_vector_const_iterator_t<T_type>& operator++();
    dac_vector_const_iterator_t<T_type> operator++(int);
    dac_vector_const_iterator_t<T_type>& operator--();
    dac_vector_const_iterator_t<T_type> operator--(int);

    dac_vector_const_iterator_t<T_type> operator+(difference_type rhs) const;
    dac_vector_const_iterator_t<T_type> operator-(difference_type rhs) const;
    difference_type operator-(const dac_vector_const_iterator_t<T_type>& rhs) const;

    dac_vector_const_iterator_t<T_type>& operator+=(difference_type rhs);
    dac_vector_const_iterator_t<T_type>& operator-=(difference_type rhs);

    reference operator[](difference_type rhs) const;

    bool operator<(const dac_vector_const_iterator_t<T_type>& rhs) const;
    bool operator>(const dac_vector_const_iterator_t<T_type>& rhs) const;
    bool operator<=(const dac_vector_const_iterator_t<T_type>& rhs) const;
    bool operator>=(const dac_vector_const_iterator_t<T_type>& rhs) const;

private:
    const T_type* m_iterator_data;
    uint32_t m_index;
};

template <typename T_type>
dac_vector_const_iterator_t<T_type> operator+(
    typename dac_vector_const_iterator_t<T_type>::difference_type lhs,
    const dac_vector_const_iterator_t<T_type>& rhs);

/**
 * A read-only reverse iterator for traversing elements in a `dac_vector_t`.
 *
 * @tparam T_type The type of `dac_vector_t` elements that the iterator traverses.
 */
template <typename T_type>
class dac_vector_const_reverse_iterator_t
{
public:
    using difference_type = std::ptrdiff_t;
    using value_type = T_type;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = std::random_access_iterator_tag;

    /**
     * Constructs a reverse iterator that does not point to any data.
     */
    dac_vector_const_reverse_iterator_t() = default;

    /**
     * Constructs a revere iterator pointing to a DAC vector, starting at a given index.
     * @param dac_vector The DAC vector to iterate over.
     * @param index The vector index to start iterating from.
     */
    explicit dac_vector_const_reverse_iterator_t(const dac_vector_t<T_type>& dac_vector, uint32_t index);

    // Comparing iterators from different containers is undefined behavior.

    bool operator==(const dac_vector_const_reverse_iterator_t& rhs) const;
    bool operator!=(const dac_vector_const_reverse_iterator_t& rhs) const;

    reference operator*() const;
    pointer operator->() const;

    dac_vector_const_reverse_iterator_t<T_type>& operator++();
    dac_vector_const_reverse_iterator_t<T_type> operator++(int);
    dac_vector_const_reverse_iterator_t<T_type>& operator--();
    dac_vector_const_reverse_iterator_t<T_type> operator--(int);

    dac_vector_const_reverse_iterator_t<T_type> operator+(difference_type rhs) const;
    dac_vector_const_reverse_iterator_t<T_type> operator-(difference_type rhs) const;
    difference_type operator-(const dac_vector_const_reverse_iterator_t<T_type>& rhs) const;

    dac_vector_const_reverse_iterator_t<T_type>& operator+=(difference_type rhs);
    dac_vector_const_reverse_iterator_t<T_type>& operator-=(difference_type rhs);

    reference operator[](difference_type rhs) const;

    bool operator<(const dac_vector_const_reverse_iterator_t<T_type>& rhs) const;
    bool operator>(const dac_vector_const_reverse_iterator_t<T_type>& rhs) const;
    bool operator<=(const dac_vector_const_reverse_iterator_t<T_type>& rhs) const;
    bool operator>=(const dac_vector_const_reverse_iterator_t<T_type>& rhs) const;

private:
    const T_type* m_iterator_data;
    uint32_t m_index;
};

template <typename T_type>
dac_vector_const_reverse_iterator_t<T_type> operator+(
    typename dac_vector_const_reverse_iterator_t<T_type>::difference_type lhs,
    const dac_vector_const_reverse_iterator_t<T_type>& rhs);

/**
 * The base class of array fields.
 *
 * @tparam T_type The type of the array's elements.
 */
template <typename T_type>
class dac_vector_t
{
public:
    using const_iterator = dac_vector_const_iterator_t<T_type>;
    using const_reverse_iterator = dac_vector_const_reverse_iterator_t<T_type>;

    dac_vector_t() = delete;

    /**
     * @return A pointer to the contiguous array of data stored by the vector.
     */
    const T_type* data() const;

    /**
     * @return The number of elements in the vector.
     */
    uint32_t size() const;

    /**
     * The read-only member access operator.
     * Unlike typical `operator[]` implementations, it returns a copy of the element instead of a reference.
     *
     * @param i The index to access. Out-of-bounds indices cause undefined behavior.
     * @return A copy of the element at given index.
     */
    T_type operator[](uint32_t i) const;

    /**
     * @return An iterator pointing to the first element of the vector.
     */
    const_iterator begin() const;

    /**
     * @return A "past-the-end" iterator used to check if there are no more elements.
     * It is not the last element and should not be dereferenced.
     */
    const_iterator end() const;

    /**
     * @return A reverse iterator pointing to the last element of the vector.
     */
    const_reverse_iterator rbegin() const;

    /**
     * @return A "past-the-start" reverse iterator used to check if there are no more elements.
     * It is not the first element and should not be dereferenced.
     */
    const_reverse_iterator rend() const;

    /**
     * @return A `std::vector` containing a copy of all elements in the Direct Access vector.
     */
    std::vector<T_type> to_vector() const;

private:
    /**
     * `dac_object_t` needs to be a friend so it can call the private vector constructor.
     */
    template <gaia::common::gaia_type_t::value_type gaia_type, typename T_gaia, typename T_fb, typename T_obj>
    friend struct dac_object_t;

    explicit dac_vector_t(const flatbuffers::Vector<T_type>* vector_ptr);

    const flatbuffers::Vector<T_type>* m_vector;
};

// Pick up our template implementation.  These still
// need to be in the header so that template specializations
// that are declared later will pick up the definitions.
#include "dac_array.inc"

/*@}*/
} // namespace direct_access
/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
