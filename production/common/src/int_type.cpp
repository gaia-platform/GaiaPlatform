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

// Explicit instantiations of the int_type_t template.
// These are all the instantiations that we support
// and the only ones that we should need.
template class int_type_t<uint64_t>;

static_assert(
    sizeof(int_type_t<uint64_t>) == sizeof(uint64_t),
    "The int_type_t<uint64_t> instantiation has a different size than its underlying uint64_t type!");

template class int_type_t<uint32_t>;

static_assert(
    sizeof(int_type_t<uint32_t>) == sizeof(uint32_t),
    "The int_type_t<uint32_t> instantiation has a different size than its underlying uint32_t type!");

template class int_type_t<uint16_t>;

static_assert(
    sizeof(int_type_t<uint16_t>) == sizeof(uint16_t),
    "The int_type_t<uint16_t> instantiation has a different size than its underlying uint16_t type!");

template class int_type_t<uint8_t>;

static_assert(
    sizeof(int_type_t<uint8_t>) == sizeof(uint8_t),
    "The int_type_t<uint8_t> instantiation has a different size than its underlying uint8_t type!");

template class int_type_t<int64_t>;

static_assert(
    sizeof(int_type_t<int64_t>) == sizeof(int64_t),
    "The int_type_t<int64_t> instantiation has a different size than its underlying int64_t type!");

template class int_type_t<int32_t>;

static_assert(
    sizeof(int_type_t<int32_t>) == sizeof(int32_t),
    "The int_type_t<int32_t> instantiation has a different size than its underlying int32_t type!");

template class int_type_t<int16_t>;

static_assert(
    sizeof(int_type_t<int16_t>) == sizeof(int16_t),
    "The int_type_t<int16_t> instantiation has a different size than its underlying int16_t type!");

template class int_type_t<int8_t>;

static_assert(
    sizeof(int_type_t<int8_t>) == sizeof(int8_t),
    "The int_type_t<int8_t> instantiation has a different size than its underlying int8_t type!");

} // namespace common
} // namespace gaia
