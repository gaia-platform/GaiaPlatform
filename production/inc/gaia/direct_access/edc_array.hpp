/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "flatbuffers/flatbuffers.h"

namespace gaia
{

namespace direct_access
{

// The vector type for direct access.
//
// A pimpl style wrapper class to encapsulate flatbuffers::Vector
// implementation.
template <typename T_type>
class vector
{
public:
    explicit vector(const flatbuffers::Vector<T_type>* vector_ptr)
        : m_ptr(vector_ptr){};

    explicit operator const flatbuffers::Vector<T_type>*() const
    {
        return m_ptr;
    };

    T_type* data()
    {
        return m_ptr->data();
    };

    uint32_t size()
    {
        return m_ptr->size();
    };

    auto operator[](uint32_t i) const
    {
        return (*m_ptr)[i];
    }

    auto begin()
    {
        return m_ptr->begin();
    };

    auto end()
    {
        return m_ptr->end();
    };

private:
    const flatbuffers::Vector<T_type>* m_ptr;
};

} // namespace direct_access
} // namespace gaia
