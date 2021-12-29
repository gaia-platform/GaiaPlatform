/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace gaia
{
namespace common
{
namespace hash
{

// Replacement of `std::rotl` before C++ 20 from the post below:
// https://blog.regehr.org/archives/1063
uint32_t rotl32(uint32_t x, uint32_t n);

// Compute murmur3 32 bit hash for the key. Adapted from the public domain
// murmur3 hash implementation at:
// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
//
// We made the following changes to the original implementation.
// - Use the key length as the seed.
// - Use `std::memcpy` for block read.
// - Return `uint32_t` directly to avoid block write.
// - Change C-style cast to C++ cast and switch to `auto` type.
// - Code format change in various places to adhering to Gaia coding guideline.
//
// Warning: murmur3 is not a cryptographic hash function and should not be used
// in places where security is a concern.
uint32_t murmur3_32(const void* key, int len);

} // namespace hash
} // namespace common
} // namespace gaia
