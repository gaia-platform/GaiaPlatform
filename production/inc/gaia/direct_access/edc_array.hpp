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
struct edc_vector_t
{
    const T_type* data()
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

    auto begin() const
    {
        return m_ptr->begin();
    };

    auto end() const
    {
        return m_ptr->end();
    };

private:
    template <gaia::common::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
    friend struct edc_object_t;

    explicit edc_vector_t(const flatbuffers::Vector<T_type>* vector_ptr)
        : m_ptr(vector_ptr){};

    const flatbuffers::Vector<T_type>* m_ptr;
};

} // namespace direct_access
} // namespace gaia
