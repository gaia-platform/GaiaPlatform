/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <type_traits>

#include <flatbuffers/flatbuffers.h>

#include "gaia/direct_access/dac_object.hpp"

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

template <typename T_class>
class dac_vector_iterator_t
{
public:
    dac_vector_iterator_t();

    explicit dac_vector_iterator_t(const T_class* iterator_data, uint32_t index);

    dac_vector_iterator_t<T_class>& operator++();
    dac_vector_iterator_t<T_class> operator++(int);

    bool operator==(const dac_vector_iterator_t& rhs) const;
    bool operator!=(const dac_vector_iterator_t& rhs) const;

    const T_class& operator*() const;
    const T_class* operator->() const;

protected:
    const T_class* m_iterator_data;
    uint32_t m_index;
};

/**
 * The base class of array fields. It encapsulates `flatbuffers::Vector` for storage.
 *
 * @tparam T_type the type of the array's elements.
 */
template <typename T_type>
class dac_vector_t
{
public:
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
    dac_vector_iterator_t<T_type> begin() const;

    /**
     * @return A "past-the-end" iterator used to check if there are no more elements.
     * It is not the last element and should not be dereferenced.
     */
    dac_vector_iterator_t<T_type> end() const;

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
