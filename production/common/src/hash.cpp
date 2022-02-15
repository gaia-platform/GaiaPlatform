/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/common/hash.hpp"

#include <cstdint>
#include <cstring>

#include "gaia_internal/common/retail_assert.hpp"

namespace gaia
{
namespace common
{
namespace hash
{

// Replacement of `std::rotl` before C++ 20 from the post below:
// https://blog.regehr.org/archives/1063
inline uint32_t rotl32(uint32_t x, uint32_t n)
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    ASSERT_PRECONDITION(n < 32, "Out of range rotation number.");
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    return (x << n) | (x >> (-n & 31));
}

// Compute murmur3 32 bit hash for the key.
uint32_t murmur3_32(const void* key, size_t len)
{
    auto data = static_cast<const uint8_t*>(key);
    const size_t nblocks = len / 4;

    uint32_t h1 = len;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    //----------
    // body

    auto blocks = reinterpret_cast<const uint32_t*>(data + nblocks * 4);

    for (size_t i = -nblocks; i; i++)
    {
        uint32_t k1;
        std::memcpy(&k1, (blocks + i), sizeof(k1));

        k1 *= c1;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 = rotl32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        h1 = rotl32(h1, 13);
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        h1 = h1 * 5 + 0xe6546b64;
    }

    //----------
    // tail

    auto tail = static_cast<const uint8_t*>(data + nblocks * 4);

    uint32_t k1 = 0;

    switch (len & 3)
    {
    case 3:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 ^= tail[2] << 16;
    case 2:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 ^= tail[1] << 8;
    case 1:
        k1 ^= tail[0];
        k1 *= c1;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 = rotl32(k1, 15);
        k1 *= c2;
        h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    h1 ^= h1 >> 16;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    h1 *= 0x85ebca6b;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    h1 ^= h1 >> 13;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    h1 *= 0xc2b2ae35;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    h1 ^= h1 >> 16;

    return h1;
}

#define BIG_CONSTANT(x) (x##LLU)

inline uint64_t rotl64(uint64_t x, int8_t r)
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    return (x << r) | (x >> (64 - r));
}

inline uint64_t fmix64(uint64_t k)
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xff51afd7ed558ccd);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    k ^= k >> 33;

    return k;
}

// Compute murmur3 128 bit hash for the key.
void murmur3_128(const void* key, const size_t len, void* out)
{
    auto data = static_cast<const uint8_t*>(key);

    const size_t nblocks = len / 16;

    uint64_t h1 = len;
    uint64_t h2 = len;

    const uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
    const uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);

    //----------
    // body

    auto blocks = reinterpret_cast<const uint64_t*>(data);

    for (size_t i = 0; i < nblocks; i++)
    {
        uint64_t k1;
        uint64_t k2;
        memcpy(&k1, (blocks + i * 2), sizeof(k1));
        memcpy(&k2, (blocks + i * 2 + 1), sizeof(k2));

        k1 *= c1;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 = rotl64(k1, 31); // rotl64(k1, 31);
        k1 *= c2;
        h1 ^= k1;

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        h1 = rotl64(h1, 27);
        h1 += h2;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k2 = rotl64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        h2 = rotl64(h2, 31);
        h2 += h1;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        h2 = h2 * 5 + 0x38495ab5;
    }

    //----------
    // tail

    auto tail = static_cast<const uint8_t*>(data + nblocks * 16);

    uint64_t k1 = 0;
    uint64_t k2 = 0;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    switch (len & 15)
    {
    case 15:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k2 ^= static_cast<uint64_t>(tail[14]) << 48;
    case 14:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k2 ^= static_cast<uint64_t>(tail[13]) << 40;
    case 13:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k2 ^= static_cast<uint64_t>(tail[12]) << 32;
    case 12:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k2 ^= static_cast<uint64_t>(tail[11]) << 24;
    case 11:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k2 ^= static_cast<uint64_t>(tail[10]) << 16;
    case 10:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k2 ^= static_cast<uint64_t>(tail[9]) << 8;
    case 9:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k2 ^= static_cast<uint64_t>(tail[8]) << 0;
        k2 *= c2;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k2 = rotl64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

    case 8:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 ^= static_cast<uint64_t>(tail[7]) << 56;
    case 7:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 ^= static_cast<uint64_t>(tail[6]) << 48;
    case 6:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 ^= static_cast<uint64_t>(tail[5]) << 40;
    case 5:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 ^= static_cast<uint64_t>(tail[4]) << 32;
    case 4:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 ^= static_cast<uint64_t>(tail[3]) << 24;
    case 3:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 ^= static_cast<uint64_t>(tail[2]) << 16;
    case 2:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 ^= static_cast<uint64_t>(tail[1]) << 8;
    case 1:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 ^= static_cast<uint64_t>(tail[0]) << 0;
        k1 *= c1;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        k1 = rotl64(k1, 31);
        k1 *= c2;
        h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len;
    h2 ^= len;

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;
    h2 += h1;

    static_cast<uint64_t*>(out)[0] = h1;
    static_cast<uint64_t*>(out)[1] = h2;
}

// Add a hash to the composite resulting from a cstring.
void multi_segment_hash::hash_add(const char* key)
{
    uint8_t hash[c_long_hash_value_length];
    murmur3_128(key, strlen(key), hash);
    hash_include(hash);
}

// Add a hash to the composite resulting from a float.
void multi_segment_hash::hash_add(float key)
{
    uint8_t hash[c_long_hash_value_length];
    murmur3_128(&key, sizeof(key), hash);
    hash_include(hash);
}

// Add a hash to the composite resulting from a double.
void multi_segment_hash::hash_add(double key)
{
    uint8_t hash[c_long_hash_value_length];
    murmur3_128(&key, sizeof(key), hash);
    hash_include(hash);
}

// Add a hash to the composite resulting from a boolean value.
void multi_segment_hash::hash_add(bool key)
{
    uint8_t hash[c_long_hash_value_length];

    // We want non-zero keys. This will ensure boolean's are hashed
    // with non-zero but different keys.
    uint8_t bool_true_encoding = 0xdd;
    uint8_t bool_false_encoding = 0x99;
    if (key)
    {
        murmur3_128(&bool_true_encoding, sizeof(uint8_t), hash);
    }
    else
    {
        murmur3_128(&bool_false_encoding, sizeof(uint8_t), hash);
    }
    hash_include(hash);
}

// Add a hash value to the composite.
void multi_segment_hash::hash_include(const uint8_t* hash_in)
{
    m_hashes.insert(m_hashes.end(), hash_in, hash_in + c_long_hash_value_length);
}

// Return the hash of all included hashes.
void multi_segment_hash::hash_calc(uint8_t* hash_out)
{
    hash_calc();
    memcpy(hash_out, m_hash, c_long_hash_value_length);
}

void multi_segment_hash::hash_calc()
{
    if (m_hashes.size() == 16)
    {
        std::memcpy(m_hash, m_hashes.data(), c_long_hash_value_length);
    }
    else
    {
        murmur3_128(m_hashes.data(), m_hashes.size(), m_hash);
    }
}

char* multi_segment_hash::to_string()
{
    for (size_t i = 0; i < c_long_hash_value_length; ++i)
    {
        sprintf(m_hash_string + i * 2, "%02x", m_hash[i]);
    }

    return m_hash_string;
}

} // namespace hash
} // namespace common
} // namespace gaia
