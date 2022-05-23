////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <cstdint>
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
uint32_t murmur3_32(const void* key, int len);

/*
 * Create a composite mash from 1 or more previous 128-bit hashes.
 *
 * Note that if only one hash has been included in this composite, it
 * will not be hashed again, but will be returned in its origical value.
 */
class multi_segment_hash
{
public:
    static constexpr int c_murmur3_128_hash_size_in_bytes = 16;

    // Boolean encoding. These values are arbitrary. They just need to be different.
    static constexpr uint8_t bool_true_encoding = 0xdd;
    static constexpr uint8_t bool_false_encoding = 0x99;

    /**
     * Compute murmur3 128 bit hash for the key.
     */
    void murmur3_128(const void* key, const int len, void* out);

    /**
     * Add a hash to the composite resulting from a key.
     * Scalar values are encoded in such a way that the hash values are most diverse.
     * An issue with the murmur3 algorithm is that trailing 0's don't affect the
     * hash value. The encoding rules implemented here will reduce the odds of that.
     * Strings and floating point require no encoding.
     * Booleans are changed to two different non-zero values.
     * All other scalars are inverted because of the frequency of 0.
     */
    void hash_add(const char* key);
    void hash_add(float key);
    void hash_add(double key);
    void hash_add(bool key);
    template <typename T>
    void hash_add(T key)
    {
        uint8_t hash[c_murmur3_128_hash_size_in_bytes];
        T use_key = ~key;
        murmur3_128(&use_key, sizeof(T), hash);
        hash_include(hash);
    }

    /**
     * Add a hash value to the composite.
     */
    void hash_include(const uint8_t* hash_in);

    /**
     * Calculate the hash of all included hashes, optionally return the value.
     */
    void hash_calc(uint8_t* hash_out);
    void hash_calc();

    /**
     * Return a pointer to the calculated hash value;
     */
    uint8_t* hash()
    {
        return m_hash;
    }

    /**
     * Return the calculated hash value as a char string.
     */
    char* to_string();

private:
    // All the hashes that will be used to compose this one hash.
    std::vector<uint8_t> m_hashes;

    // The final hash value.
    uint8_t m_hash[c_murmur3_128_hash_size_in_bytes];

    // A printable form of the hash for human consumption.
    char m_hash_string[(c_murmur3_128_hash_size_in_bytes * 2) + 1];
};

} // namespace hash
} // namespace common
} // namespace gaia
