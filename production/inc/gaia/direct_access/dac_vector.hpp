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
namespace direct_access
{

/**
 * @brief A vector class for enabling operation with vector fields.
 */
template <typename T_type>
class dac_vector_t
{
public:
    dac_vector_t() = delete;

    const T_type* data() const
    {
        return m_vector->data();
    }

    uint32_t size() const
    {
        return m_vector->size();
    }

    // Normally the operator "[]" should return a reference or const reference
    // to the array element. Given we only support arrays of basic types and the
    // vector class is always read-only, it should be safe to return the T_type
    // value directly here without any performance or functionality loss.
    T_type operator[](uint32_t i) const
    {
        return (*m_vector)[i];
    }

    std::vector<T_type> to_vector() const
    {
        return std::vector<T_type>(data(), data() + size());
    }

private:
    // Make dac_object_t a friend so it can call the private vector constructor.
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
