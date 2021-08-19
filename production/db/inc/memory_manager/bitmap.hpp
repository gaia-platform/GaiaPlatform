/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stddef.h>

#include <climits>
#include <cstdint>

#include <atomic>

namespace gaia
{
namespace db
{
namespace memory_manager
{

constexpr size_t c_uint64_bit_count = CHAR_BIT * sizeof(uint64_t);
constexpr uint64_t c_all_set_word = -1;
constexpr size_t c_max_bit_index = -1;

// Checks the value of a bit.
bool is_bit_set(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t bit_index);

// Sets the value of a bit.
// This method should only be used if we're the only ones updating a bitmap,
// otherwise it will lead to missing updates.
void set_bit_value(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t bit_index, bool value);

// Attempts to set the value of a bit.
// Will fail if another thread is updating the word in which the bit lies.
// Caller could check bit value again to decide whether to retry.
bool try_set_bit_value(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t bit_index, bool value);

// Sets the value of a range of bits.
// This method is safe in the sense that it will not cause a loss of updates to other bits than the ones we set.
void safe_set_bit_range_value(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t start_bit_index, size_t bit_count, bool value);

// Count the number of set bits within a bitmap.
// end_limit_bit_index can limit the counting to a prefix of a large bitmap.
// If end_limit_bit_index is not set, we'll count the entire bitmap.
size_t count_set_bits(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t end_limit_bit_index = c_max_bit_index);

// Find the first unset bit in a bitmap.
// end_limit_bit_index can limit the search to a prefix of a large bitmap.
// If end_limit_bit_index is not set, we'll search the entire bitmap.
size_t find_first_unset_bit(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t end_limit_bit_index = c_max_bit_index);

// Bitarray terminology.
//
// A bitarray is an array of bitmaps of fixed size which we refer to as "elements".
//
// A bitarray can be used instead of a set of bitmaps,
// to permit the atomic modification of several bits at the same position at the same time.
//
// We can reference elements of a bitarray using the index of the elements (element_index)
// or the bit index of the first bit of the element (bit_index).

// Find the first occurrence of a bitarray element
// and return the element index of this occurrence.
// end_limit_element_index can limit the search to a prefix of a large bitarray. If
// end_limit_element_index is not set, we'll search the entire bitarray.
size_t find_first_element(
    std::atomic<uint64_t>* bitarray, size_t bitarray_word_size,
    size_t element_width, uint64_t element_value,
    size_t end_limit_element_index = c_max_bit_index);

// Read a bitarray element with given width at the given element index.
uint64_t get_element_at_index(
    std::atomic<uint64_t>* bitarray, size_t bitarray_word_size,
    size_t element_width, size_t element_index);

// Atomically (using CAS) set a bitarray element with given width at the given bitarray index.
void set_element_at_index(
    std::atomic<uint64_t>* bitarray, size_t bitarray_word_size,
    size_t element_width, size_t element_index, uint64_t element_value);

// Atomically (using CAS) set a bitarray element with given width at the given bitarray index.
// This operation will fail if the current element value is not `expected_element_value`.
bool conditional_set_element_at_index(
    std::atomic<uint64_t>* bitarray, size_t bitarray_word_size,
    size_t element_width, size_t element_index,
    uint64_t expected_element_value, uint64_t desired_element_value);

// Print a bitmap to console, for testing and debugging.
void print_bitmap(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, bool msb_first = false);

} // namespace memory_manager
} // namespace db
} // namespace gaia
