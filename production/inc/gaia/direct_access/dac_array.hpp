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
namespace direct_access
{

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

// A pimpl style wrapper class that encapsulates flatbuffers::Vector.
template <typename T_type>
class dac_vector_t
{
public:
    dac_vector_t() = delete;

    const T_type* data() const;
    uint32_t size() const;

    // Normally the operator "[]" should return a reference or const reference
    // to the array element. Given we only support arrays of basic types and the
    // vector class is always read-only, it should be safe to return the T_type
    // value directly here without any performance or functionality loss.
    T_type operator[](uint32_t i) const;

    dac_vector_iterator_t<T_type> begin() const;
    dac_vector_iterator_t<T_type> end() const;

    std::vector<T_type> to_vector() const;

private:
    // Make the dac_object_t a friend so it can call the private vector constructor.
    template <gaia::common::gaia_type_t::value_type gaia_type, typename T_gaia, typename T_fb, typename T_obj>
    friend struct dac_object_t;

    explicit dac_vector_t(const flatbuffers::Vector<T_type>* vector_ptr);

    const flatbuffers::Vector<T_type>* m_vector;
};

#include "dac_array.inc"

} // namespace direct_access
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
