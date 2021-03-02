/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "flatbuffers/flatbuffers.h"

#include "gaia/direct_access/edc_object.hpp"

namespace gaia
{

namespace direct_access
{

// A pimpl style wrapper class that encapsulates flatbuffers::Vector.
template <typename T_type>
class edc_vector_t
{
public:
    const T_type* data()
    {
        return m_vector->data();
    };

    uint32_t size()
    {
        return m_vector->size();
    };

    // Normally operator [] should return a reference or const reference to the
    // element. Given we only support array of basic types and the vector is
    // read-only, it is safe to return the value directly.
    T_type operator[](uint32_t i) const
    {
        return (*m_vector)[i];
    }

private:
    // Make the edc_object_t a friend so it can call the private vector constructor.
    template <gaia::common::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
    friend struct edc_object_t;

    explicit edc_vector_t(const flatbuffers::Vector<T_type>* vector_ptr)
        : m_vector(vector_ptr){};

    const flatbuffers::Vector<T_type>* m_vector;
};

} // namespace direct_access
} // namespace gaia
