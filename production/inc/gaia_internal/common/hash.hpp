/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

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
uint32_t murmur3_32(const void* key, size_t len);

constexpr size_t c_bytes_per_long_hash = 16;

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
    /**
     * Add a hash to the composite resulting from a key.
     * Scalar values are encoded in such a way that the hash values are most diverse.
     * An issue with the murmur3 algorithm is that trailing 0's don't affect the
     * hash value. The encoding rules implemented here will reduce the odds of that.
     * Strings require no encoding.
     */
    void hash_add(const char* key);
    // Floating point requires no encoding.
    void hash_add(float key);
    void hash_add(double key);
    // Booleans become 0xdd for true and 0x99 for false.
    void hash_add(bool key);
    // All other scalar values are inverted.
    template <typename T>
    void hash_add(T key)
    {
        uint8_t hash[c_bytes_per_long_hash];
        // We are inverting the integral values because low values like 0 or 1 can
        // create a hash that is the same regardless of the length, 1, 2, 4 or 8 bytes.
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
    uint8_t m_hash[c_bytes_per_long_hash];

    // A printable form of the hash for human consumption.
    char m_hash_string[(c_bytes_per_long_hash * 2) + 1];
};

} // namespace hash
} // namespace common
} // namespace gaia
