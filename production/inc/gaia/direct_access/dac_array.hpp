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
    dac_vector_iterator_t()
        : m_iterator_data(nullptr), m_index(0)
    {
    }

    explicit dac_vector_iterator_t(const T_class* iterator_data, uint32_t index)
        : m_iterator_data(iterator_data), m_index(index)
    {
    }

    dac_vector_iterator_t<T_class>& operator++()
    {
        ++m_index;
        return *this;
    }
    dac_vector_iterator_t<T_class> operator++(int)
    {
        dac_vector_iterator_t<T_class> temp = *this;
        ++*this;
        return temp;
    }
    bool operator==(const dac_vector_iterator_t& rhs) const
    {
        return m_iterator_data == rhs.m_iterator_data && m_index == rhs.m_index;
    }
    bool operator!=(const dac_vector_iterator_t& rhs) const
    {
        return m_iterator_data != rhs.m_iterator_data || m_index != rhs.m_index;
    }

    const T_class& operator*() const
    {
        return *(m_iterator_data + m_index);
    }

    const T_class* operator->() const
    {
        return m_iterator_data + m_index;
    }

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

    const T_type* data() const
    {
        if (is_null())
        {
            return nullptr;
        }
        return m_vector->data();
    }

    uint32_t size() const
    {
        if (is_null())
        {
            return 0;
        }

        return m_vector->size();
    }

    bool is_null() const
    {
        return m_vector == nullptr;
    }

    // Normally the operator "[]" should return a reference or const reference
    // to the array element. Given we only support arrays of basic types and the
    // vector class is always read-only, it should be safe to return the T_type
    // value directly here without any performance or functionality loss.
    T_type operator[](uint32_t i) const
    {
        return (*m_vector)[i];
    }

    dac_vector_iterator_t<T_type> begin() const
    {
        return dac_vector_iterator_t<T_type>(data(), 0);
    }

    dac_vector_iterator_t<T_type> end() const
    {
        return dac_vector_iterator_t<T_type>(data(), size());
    }

private:
    // Make the dac_object_t a friend so it can call the private vector constructor.
    template <gaia::common::gaia_type_t::value_type gaia_type, typename T_gaia, typename T_fb, typename T_obj>
    friend struct dac_object_t;

    explicit dac_vector_t(const flatbuffers::Vector<T_type>* vector_ptr)
        : m_vector(vector_ptr)
    {
        static_assert(std::is_arithmetic<T_type>::value, "dac_vector_t only supports basic types!");
    };

    const flatbuffers::Vector<T_type>* m_vector;
};

} // namespace direct_access
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
