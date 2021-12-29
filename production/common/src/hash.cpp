/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/common/hash.hpp"

#include <cstring>

#include "gaia_internal/common/retail_assert.hpp"

namespace gaia
{
namespace common
{
namespace hash
{

uint32_t rotl32(uint32_t x, uint32_t n)
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    ASSERT_PRECONDITION(n < 32, "Out of range rotation number.");
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    return (x << n) | (x >> (-n & 31));
}

uint32_t murmur3_32(const void* key, int len)
{
    auto data = static_cast<const uint8_t*>(key);
    const int nblocks = len / 4;

    uint32_t h1 = len;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    //----------
    // body

    auto blocks = reinterpret_cast<const uint32_t*>(data + nblocks * 4);

    for (int i = -nblocks; i; i++)
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

} // namespace hash
} // namespace common
} // namespace gaia
