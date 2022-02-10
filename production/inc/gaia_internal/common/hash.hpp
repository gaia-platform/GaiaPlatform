/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>

#include <vector>

namespace gaia
{
namespace common
{
namespace hash
{

/*
 * Compute murmur3 32 bit hash for the key.
 *
 * Adapted from the public domain murmur3 hash implementation at:
 * https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
 *
 * We made the following changes to the original implementation.
 * - Use the key length as the seed.
 * - Use `std::memcpy` for block read.
 * - Return `uint32_t` directly to avoid block write.
 * - Change C-style cast to C++ cast and switch to `auto` type.
 * - Code format change in various places to adhering to Gaia coding guideline.
 *
 * Warning: murmur3 is not a cryptographic hash function and should not be used
 * in places where security is a concern.
 */
uint32_t murmur3_32(const void* key, size_t len);

constexpr size_t c_long_hash_value_length = 16;

/*
 * Compute murmur3 128 bit hash for the key.
 */
void murmur3_128(const void* key, const size_t len, void* out);

/*
 * Create a composite mash from 1 or more previous 128-bit hashes.
 *
 * Note that if only one hash has been included in this composite, it
 * will not be hashed again, but will be returned in its origical value.
 */
class multi_segment_hash
{
public:
    // Add a hash to the composite resulting from a key.
    void hash_add(const void* key, size_t len);

    // Add a hash value to the composite.
    void hash_include(const uint8_t* hash_in);

    // Return the hash of all included hashes.
    void hash_calc(uint8_t* hash_out);

private:
    std::vector<uint8_t> m_hashes;
};

} // namespace hash
} // namespace common
} // namespace gaia
