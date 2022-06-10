////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <stddef.h>

#include <climits>
#include <cstdint>

#include <atomic>
#include <bitset>
#include <iostream>
#include <sstream>

#include "gaia/common.hpp"

#include "gaia_internal/common/debug_assert.hpp"

namespace gaia
{
namespace common
{
namespace bitmap
{

static inline constexpr uint64_t c_all_set_word = -1;
static inline constexpr size_t c_max_bit_index = -1;

// Returns the index of the first set bit in a word. The word must be nonzero.
static inline size_t find_first_set_bit_in_word(uint64_t word);

// Returns the index of the last set bit in a word. The word must be nonzero.
static inline size_t find_last_set_bit_in_word(uint64_t word);

// Checks the value of a bit.
static inline bool is_bit_set(
    std::atomic<uint64_t>* bitmap, size_t bitmap_size_in_words, size_t bit_index);

// Sets the value of a bit.
// This method should only be used if we're the only ones updating a bitmap,
// otherwise it will lead to missing updates.
static inline void set_bit_value(
    std::atomic<uint64_t>* bitmap, size_t bitmap_size_in_words, size_t bit_index, bool value);

// Sets the value of a bit.
// This method is safe in the presence of concurrent updates.
static inline void safe_set_bit_value(
    std::atomic<uint64_t>* bitmap, size_t bitmap_size_in_words, size_t bit_index, bool value);

// Attempts to set the value of a bit.
// Will fail if another thread is updating the word in which the bit lies.
// Caller could check bit value again to decide whether to retry.
static inline bool try_set_bit_value(
    std::atomic<uint64_t>* bitmap, size_t bitmap_size_in_words, size_t bit_index, bool value, bool fail_if_already_set = false);

// Sets the value of a range of bits.
// This method is safe in the sense that it will not cause a loss of updates to other bits than the ones we set.
static inline void safe_set_bit_range_value(
    std::atomic<uint64_t>* bitmap, size_t bitmap_size_in_words, size_t start_bit_index, size_t bit_count, bool value);

// Count the number of set bits within a bitmap.
// end_limit_bit_index can limit the counting to a prefix of a large bitmap.
// If end_limit_bit_index (exclusive) is not set, we'll count the entire bitmap.
static inline size_t count_set_bits(
    std::atomic<uint64_t>* bitmap, size_t bitmap_size_in_words, size_t end_limit_exclusive_bit_index = c_max_bit_index);

// Find the first unset bit in a bitmap.
// end_limit_bit_index can limit the search to a prefix of a large bitmap.
// If end_limit_bit_index (exclusive) is not set, we'll search the entire bitmap.
static inline size_t find_first_unset_bit(
    std::atomic<uint64_t>* bitmap, size_t bitmap_size_in_words, size_t end_limit_exclusive_bit_index = c_max_bit_index);

// Find the last set bit in a bitmap.
static inline size_t find_last_set_bit(std::atomic<uint64_t>* bitmap, size_t bitmap_size_in_words);

// Print a bitmap to console, for testing and debugging.
static inline void print_bitmap(
    std::atomic<uint64_t>* bitmap, size_t bitmap_size_in_words, bool msb_first = false);

#include "bitmap.inc"

} // namespace bitmap
} // namespace common
} // namespace gaia
