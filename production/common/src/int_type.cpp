/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/int_type.hpp"

#include <cstdint>

namespace gaia
{
namespace common
{

template <typename T, T default_invalid_value, typename C>
int_type_t<T, default_invalid_value, C>::operator T() const
{
    return m_value;
}

template <typename T, T default_invalid_value, typename C>
T& int_type_t<T, default_invalid_value, C>::value_ref()
{
    return m_value;
}

template <typename T, T default_invalid_value, typename C>
int_type_t<T, default_invalid_value, C>& int_type_t<T, default_invalid_value, C>::operator+=(
    const int_type_t<T, default_invalid_value, C>& other)
{
    this->m_value += other.m_value;
    return *this;
}

template <typename T, T default_invalid_value, typename C>
int_type_t<T, default_invalid_value, C>& int_type_t<T, default_invalid_value, C>::operator-=(
    const int_type_t<T, default_invalid_value, C>& other)
{
    this->m_value -= other.m_value;
    return *this;
}

template <typename T, T default_invalid_value, typename C>
int_type_t<T, default_invalid_value, C>& int_type_t<T, default_invalid_value, C>::operator*=(
    const int_type_t<T, default_invalid_value, C>& other)
{
    this->m_value *= other.m_value;
    return *this;
}

template <typename T, T default_invalid_value, typename C>
int_type_t<T, default_invalid_value, C>& int_type_t<T, default_invalid_value, C>::operator/=(
    const int_type_t<T, default_invalid_value, C>& other)
{
    this->m_value /= other.m_value;
    return *this;
}

template <typename T, T default_invalid_value, typename C>
int_type_t<T, default_invalid_value, C>& int_type_t<T, default_invalid_value, C>::operator%=(
    const int_type_t<T, default_invalid_value, C>& other)
{
    this->m_value %= other.m_value;
    return *this;
}

template <typename T, T default_invalid_value, typename C>
int_type_t<T, default_invalid_value, C>& int_type_t<T, default_invalid_value, C>::operator++()
{
    this->m_value += 1;
    return *this;
}

template <typename T, T default_invalid_value, typename C>
int_type_t<T, default_invalid_value, C> int_type_t<T, default_invalid_value, C>::operator++(int)
{
    int_type_t pre_value = *this;
    this->m_value += 1;
    return pre_value;
}

template <typename T, T default_invalid_value, typename C>
int_type_t<T, default_invalid_value, C>& int_type_t<T, default_invalid_value, C>::operator--()
{
    this->m_value -= 1;
    return *this;
}

template <typename T, T default_invalid_value, typename C>
int_type_t<T, default_invalid_value, C> int_type_t<T, default_invalid_value, C>::operator--(int)
{
    int_type_t pre_value = *this;
    this->m_value -= 1;
    return pre_value;
}

// Explicit instantiations of the int_type_t template.
// These are all the instantiations that we support
// and the only ones that we should need.
template class int_type_t<uint64_t, 0>;
template class int_type_t<uint64_t, std::numeric_limits<uint64_t>::max()>;

static_assert(
    sizeof(int_type_t<uint64_t, 0>) == sizeof(uint64_t),
    "The int_type_t<uint64_t> instantiation has a different size than its underlying uint64_t type!");
static_assert(
    sizeof(int_type_t<uint64_t, std::numeric_limits<uint64_t>::max()>) == sizeof(uint64_t),
    "The int_type_t<uint64_t> instantiation has a different size than its underlying uint64_t type!");

template class int_type_t<uint32_t, 0>;
template class int_type_t<uint32_t, std::numeric_limits<uint32_t>::max()>;

static_assert(
    sizeof(int_type_t<uint32_t, 0>) == sizeof(uint32_t),
    "The int_type_t<uint32_t> instantiation has a different size than its underlying uint32_t type!");
static_assert(
    sizeof(int_type_t<uint32_t, std::numeric_limits<uint32_t>::max()>) == sizeof(uint32_t),
    "The int_type_t<uint32_t> instantiation has a different size than its underlying uint32_t type!");

template class int_type_t<uint16_t, 0>;
template class int_type_t<uint16_t, std::numeric_limits<uint16_t>::max()>;

static_assert(
    sizeof(int_type_t<uint16_t, 0>) == sizeof(uint16_t),
    "The int_type_t<uint16_t> instantiation has a different size than its underlying uint16_t type!");
static_assert(
    sizeof(int_type_t<uint16_t, std::numeric_limits<uint16_t>::max()>) == sizeof(uint16_t),
    "The int_type_t<uint16_t> instantiation has a different size than its underlying uint16_t type!");

template class int_type_t<uint8_t, 0>;
template class int_type_t<uint8_t, std::numeric_limits<uint8_t>::max()>;

static_assert(
    sizeof(int_type_t<uint8_t, 0>) == sizeof(uint8_t),
    "The int_type_t<uint8_t> instantiation has a different size than its underlying uint8_t type!");
static_assert(
    sizeof(int_type_t<uint8_t, std::numeric_limits<uint8_t>::max()>) == sizeof(uint8_t),
    "The int_type_t<uint8_t> instantiation has a different size than its underlying uint8_t type!");

template class int_type_t<int64_t, 0>;
template class int_type_t<int64_t, std::numeric_limits<int64_t>::max()>;

static_assert(
    sizeof(int_type_t<int64_t, 0>) == sizeof(int64_t),
    "The int_type_t<int64_t> instantiation has a different size than its underlying int64_t type!");
static_assert(
    sizeof(int_type_t<int64_t, std::numeric_limits<int64_t>::max()>) == sizeof(int64_t),
    "The int_type_t<int64_t> instantiation has a different size than its underlying int64_t type!");

template class int_type_t<int32_t, 0>;
template class int_type_t<int32_t, std::numeric_limits<int32_t>::max()>;

static_assert(
    sizeof(int_type_t<int32_t, 0>) == sizeof(int32_t),
    "The int_type_t<int32_t> instantiation has a different size than its underlying int32_t type!");
static_assert(
    sizeof(int_type_t<int32_t, std::numeric_limits<int32_t>::max()>) == sizeof(int32_t),
    "The int_type_t<int32_t> instantiation has a different size than its underlying int32_t type!");

template class int_type_t<int16_t, 0>;
template class int_type_t<int16_t, std::numeric_limits<int16_t>::max()>;

static_assert(
    sizeof(int_type_t<int16_t, 0>) == sizeof(int16_t),
    "The int_type_t<int16_t> instantiation has a different size than its underlying int16_t type!");
static_assert(
    sizeof(int_type_t<int16_t, std::numeric_limits<int16_t>::max()>) == sizeof(int16_t),
    "The int_type_t<int16_t> instantiation has a different size than its underlying int16_t type!");

template class int_type_t<int8_t, 0>;
template class int_type_t<int8_t, std::numeric_limits<int8_t>::max()>;

static_assert(
    sizeof(int_type_t<int8_t, 0>) == sizeof(int8_t),
    "The int_type_t<int8_t> instantiation has a different size than its underlying int8_t type!");
static_assert(
    sizeof(int_type_t<int8_t, std::numeric_limits<int8_t>::max()>) == sizeof(int8_t),
    "The int_type_t<int8_t> instantiation has a different size than its underlying int8_t type!");

} // namespace common
} // namespace gaia
