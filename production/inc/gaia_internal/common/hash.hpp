/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <cstring>

#include <bit>

// Adapted from the public domain murmur3 hash implementation at:
// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
uint32_t murmurhash3_32(const void* key, int len)
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
        k1 = std::__rotl(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        h1 = std::__rotl(h1, 13);
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
        k1 = std::__rotl(k1, 15);
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
