/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <climits>
#include <cstdint>

#include <atomic>

namespace gaia
{
namespace db
{
namespace memory_manager
{

constexpr uint64_t c_uint64_bit_count = CHAR_BIT * sizeof(uint64_t);
constexpr uint64_t c_all_set_word = -1;
constexpr uint64_t c_max_bit_index = -1;

// Checks the value of a bit.
bool is_bit_set(
    std::atomic<uint64_t>* bitmap, uint64_t bitmap_size, uint64_t bit_index);

// Sets the value of a bit.
// This method should only be used if we're the only ones updating a bitmap,
// otherwise it will lead to missing updates.
void set_bit_value(
    std::atomic<uint64_t>* bitmap, uint64_t bitmap_size, uint64_t bit_index, bool value);

// Attempts to set the value of a bit.
// Will fail if another thread is updating the word in which the bit lies.
// Caller could check bit value again to decide whether to retry.
bool try_set_bit_value(
    std::atomic<uint64_t>* bitmap, uint64_t bitmap_size, uint64_t bit_index, bool value);

// Sets the value of a range of bits.
// This method should only be used if we're the only ones updating a bitmap,
// otherwise it will lead to missing updates.
void set_bit_range_value(
    std::atomic<uint64_t>* bitmap, uint64_t bitmap_size, uint64_t start_bit_index, uint64_t bit_count, bool value);

// Count the number of set bits within a bitmap.
// end_limit_bit_index can limit the counting to a prefix of a large bitmap.
// If end_limit_bit_index is not set, we'll count the entire bitmap.
uint64_t count_set_bits(
    std::atomic<uint64_t>* bitmap, uint64_t bitmap_size, uint64_t end_limit_bit_index = c_max_bit_index);

// Find the first unset bit in a bitmap.
// end_limit_bit_index can limit the search to a prefix of a large bitmap.
// If end_limit_bit_index is not set, we'll search the entire bitmap.
uint64_t find_first_unset_bit(
    std::atomic<uint64_t>* bitmap, uint64_t bitmap_size, uint64_t end_limit_bit_index = c_max_bit_index);

// Print a bitmap to console, for testing and debugging.
void print_bitmap(
    std::atomic<uint64_t>* bitmap, uint64_t bitmap_size);

} // namespace memory_manager
} // namespace db
} // namespace gaia
