/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "bitmap.hpp"

#include <iostream>
#include <sstream>

#include "gaia_internal/common/retail_assert.hpp"

using namespace std;

using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace memory_manager
{

void apply_mask_to_word(
    uint64_t& word, uint64_t mask, bool set)
{
    // See: https://graphics.stanford.edu/~seander/bithacks.html#ConditionalSetOrClearBitsWithoutBranching
    word ^= (-set ^ word) & mask;
}

void apply_mask_to_word(
    std::atomic<uint64_t>& word, uint64_t mask, bool set)
{
    uint64_t word_copy = word;
    apply_mask_to_word(word_copy, mask, set);
    word = word_copy;
}

bool try_apply_mask_to_word(
    std::atomic<uint64_t>& word, uint64_t mask, bool set)
{
    // We read the word once, because other threads may be updating it.
    uint64_t old_word = word;
    uint64_t new_word = old_word;

    apply_mask_to_word(new_word, mask, set);

    return word.compare_exchange_strong(old_word, new_word);
}

void safe_apply_mask_to_word(
    std::atomic<uint64_t>& word, uint64_t mask, bool set)
{
    while (!try_apply_mask_to_word(word, mask, set))
    {
        // Someone else made an update; retry after reading updated word value.
    }
}

void validate_bitmap_parameters(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size)
{
    ASSERT_PRECONDITION(bitmap != nullptr, "validate_bitmap_parameters() was called with a null bitmap!");
    ASSERT_PRECONDITION(bitmap_word_size > 0, "validate_bitmap_parameters() was called with an empty bitmap!");
}

void validate_bit_index(
    size_t bitmap_word_size, size_t bit_index)
{
    ASSERT_PRECONDITION(bit_index < bitmap_word_size * c_uint64_bit_count, "validate_bit_index() was called with arguments that exceed the range of the input bitmap!");
}

void find_bit_word_and_mask(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t bit_index, std::atomic<uint64_t>*& word, uint64_t& mask)
{
    validate_bitmap_parameters(bitmap, bitmap_word_size);
    validate_bit_index(bitmap_word_size, bit_index);

    size_t word_index = bit_index / c_uint64_bit_count;
    size_t bit_index_within_word = bit_index % c_uint64_bit_count;

    word = &bitmap[word_index];
    mask = 1ULL << bit_index_within_word;
}

bool is_bit_set(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t bit_index)
{
    std::atomic<uint64_t>* word = nullptr;
    uint64_t mask;
    find_bit_word_and_mask(bitmap, bitmap_word_size, bit_index, word, mask);
    return ((word->load() & mask) != 0);
}

void set_bit_value(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t bit_index, bool value)
{
    std::atomic<uint64_t>* word = nullptr;
    uint64_t mask;
    find_bit_word_and_mask(bitmap, bitmap_word_size, bit_index, word, mask);
    apply_mask_to_word(*word, mask, value);
}

bool try_set_bit_value(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t bit_index, bool value)
{
    std::atomic<uint64_t>* word = nullptr;
    uint64_t mask;
    find_bit_word_and_mask(bitmap, bitmap_word_size, bit_index, word, mask);
    return try_apply_mask_to_word(*word, mask, value);
}

void safe_set_bit_range_value(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t start_bit_index, size_t bit_count, bool value)
{
    validate_bitmap_parameters(bitmap, bitmap_word_size);
    validate_bit_index(bitmap_word_size, start_bit_index);

    ASSERT_PRECONDITION(bit_count > 0, "safe_set_bit_range_value() was called with a 0 bit count!");
    ASSERT_PRECONDITION(
        start_bit_index + bit_count - 1 >= start_bit_index,
        "safe_set_bit_range_value() was called with arguments that cause an integer overflow!");

    validate_bit_index(bitmap_word_size, start_bit_index + bit_count - 1);

    size_t start_word_index = start_bit_index / c_uint64_bit_count;
    size_t start_bit_index_within_word = start_bit_index % c_uint64_bit_count;

    size_t end_bit_index = start_bit_index + bit_count - 1;
    size_t end_word_index = end_bit_index / c_uint64_bit_count;
    size_t end_bit_index_within_word = end_bit_index % c_uint64_bit_count;

    // We can have three scenarios: bits fall in one word or across several words.
    if (start_word_index == end_word_index)
    {
        uint64_t mask = (bit_count == c_uint64_bit_count)
            ? c_all_set_word
            : ((1ULL << bit_count) - 1) << start_bit_index_within_word;
        safe_apply_mask_to_word(bitmap[start_word_index], mask, value);
    }
    else
    {
        // Handle the start word.
        size_t count_bits_in_first_word = c_uint64_bit_count - start_bit_index_within_word;
        uint64_t start_word_mask = (count_bits_in_first_word == c_uint64_bit_count)
            ? c_all_set_word
            : ((1ULL << count_bits_in_first_word) - 1) << start_bit_index_within_word;
        safe_apply_mask_to_word(bitmap[start_word_index], start_word_mask, value);

        // Handle any words for which we have to set all bits.
        if (end_word_index - start_word_index > 1)
        {
            for (size_t word_index = start_word_index + 1; word_index < end_word_index; ++word_index)
            {
                bitmap[word_index] = value ? c_all_set_word : 0;
            }
        }

        // Handle the end word.
        size_t count_bits_in_last_word = end_bit_index_within_word + 1;
        uint64_t end_word_mask = (count_bits_in_last_word == c_uint64_bit_count)
            ? c_all_set_word
            : ((1ULL << count_bits_in_last_word) - 1);
        safe_apply_mask_to_word(bitmap[end_word_index], end_word_mask, value);
    }
}

size_t count_set_bits(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t end_limit_bit_index)
{
    validate_bitmap_parameters(bitmap, bitmap_word_size);

    // If no limit bit index was provided, set the limit to the last bit index in the bitmap.
    if (end_limit_bit_index == c_max_bit_index)
    {
        end_limit_bit_index = bitmap_word_size * c_uint64_bit_count - 1;
    }
    else
    {
        validate_bit_index(bitmap_word_size, end_limit_bit_index);
    }

    size_t end_word_index = end_limit_bit_index / c_uint64_bit_count;
    size_t end_bit_index_within_word = end_limit_bit_index % c_uint64_bit_count;

    size_t bit_count = 0;

    for (size_t word_index = 0; word_index <= end_word_index; ++word_index)
    {
        uint64_t word = bitmap[word_index];

        // If we're processing the last word and we're not supposed to process it in its entirety,
        // then first mask out the bits that we are supposed to ignore before doing the counting.
        if (word_index == end_word_index && end_bit_index_within_word != c_uint64_bit_count - 1)
        {
            uint64_t mask = (1ULL << (end_bit_index_within_word + 1)) - 1;
            word &= mask;
        }

        // __builtin_popcountll counts the set bits.
        bit_count += __builtin_popcountll(word);
    }

    return bit_count;
}

size_t find_first_unset_bit(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t end_limit_bit_index)
{
    validate_bitmap_parameters(bitmap, bitmap_word_size);

    // If no limit bit index was provided, set the limit to the last bit index in the bitmap.
    if (end_limit_bit_index == c_max_bit_index)
    {
        end_limit_bit_index = bitmap_word_size * c_uint64_bit_count - 1;
    }
    else
    {
        validate_bit_index(bitmap_word_size, end_limit_bit_index);
    }

    size_t end_word_index = end_limit_bit_index / c_uint64_bit_count;
    size_t end_bit_index_within_word = end_limit_bit_index % c_uint64_bit_count;

    for (size_t word_index = 0; word_index <= end_word_index; ++word_index)
    {
        uint64_t word = bitmap[word_index];

        // If we're processing the last word and we're not supposed to process it in its entirety,
        // then first mask out the bits that we are supposed to ignore before doing any check.
        if (word_index == end_word_index && end_bit_index_within_word != c_uint64_bit_count - 1)
        {
            uint64_t mask = (1ULL << (end_bit_index_within_word + 1)) - 1;
            // Because we're looking out for unset bits,
            // the masking is done by setting the irrelevant bits to 1.
            word |= ~mask;
        }

        // __builtin_ffsll finds the first set bit and returns its (index + 1),
        // or returns 0 if no bits are set.
        // Because we look for the first unset bit, we'll negate our word before passing it to the builtin
        // and we'll subtract 1 from the result, which works great given that our c_max_bit_index value
        // is set to -1.
        size_t unset_bit_index = __builtin_ffsll(~word) - 1;
        if (unset_bit_index != c_max_bit_index)
        {
            return (word_index * c_uint64_bit_count) + unset_bit_index;
        }
    }

    return c_max_bit_index;
}

size_t find_first_bitstring(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, uint64_t bitstring,
    size_t width, size_t end_limit_bit_index)
{
    validate_bitmap_parameters(bitmap, bitmap_word_size);

    // If no limit bit index was provided, set the limit to the last bit index in the bitmap.
    if (end_limit_bit_index == c_max_bit_index)
    {
        end_limit_bit_index = bitmap_word_size * c_uint64_bit_count - 1;
    }
    else
    {
        validate_bit_index(bitmap_word_size, end_limit_bit_index);
    }

    // The bitstring width must divide the word length, i.e. it must be a power of 2 <= 64.
    ASSERT_PRECONDITION((width <= c_uint64_bit_count) && (c_uint64_bit_count % width == 0), "Bitstring width must divide word size!");

    // The bitstring value must fit in the bitstring width.
    ASSERT_PRECONDITION(bitstring < (1ULL << width), "Bitstring value must fit in bitstring width!");

    size_t end_word_index = end_limit_bit_index / c_uint64_bit_count;
    size_t end_bit_index_within_word = end_limit_bit_index % c_uint64_bit_count;

    // The bitstring width must divide the number of available bits in the final word.
    ASSERT_PRECONDITION((end_bit_index_within_word + 1) % width == 0, "Bitstring width must divide available bits in final word!");

    for (size_t word_index = 0; word_index <= end_word_index; ++word_index)
    {
        uint64_t word = bitmap[word_index];

        // We can assume that the bitstring is always aligned on natural
        // boundaries, i.e., on the bitstring width, and that it never spans
        // words.

        // Construct a mask with the bitstring's width and shift it over all possible aligned positions within the word.
        uint64_t mask = (1ULL << width) - 1;
        size_t position_count = c_uint64_bit_count / width;

        // Only scan up to the final bit.
        if (word_index == end_word_index)
        {
            position_count = (end_bit_index_within_word + 1) / width;
        }

        // Scan for a match at each aligned position of the bitstring within the current word.
        for (size_t position = 0; position < position_count; ++position)
        {
            // Shift the bitstring mask to the current position.
            size_t position_shift = width * position;
            uint64_t position_mask = mask << position_shift;

            // Mask out the bitstring to match at this position and shift it to its final position.
            uint64_t bitstring_to_match = (word & position_mask) >> position_shift;
            if (bitstring_to_match == bitstring)
            {
                size_t match_bit_index = (word_index * c_uint64_bit_count) + (position * width);
                return match_bit_index;
            }
        }
    }

    return c_max_bit_index;
}

uint64_t get_bitstring_at_index(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t bit_index, size_t width)
{
    validate_bitmap_parameters(bitmap, bitmap_word_size);
    validate_bit_index(bitmap_word_size, bit_index);

    // The bitstring width must divide the word length, i.e. it must be a power of 2 <= 64.
    ASSERT_PRECONDITION((width <= c_uint64_bit_count) && (c_uint64_bit_count % width == 0), "Bitstring width must divide word size!");

    // The bit index must be a multiple of the bitstring width.
    ASSERT_PRECONDITION((bit_index % width == 0), "Bit index must be a multiple of bitstring width!");

    size_t word_index = bit_index / c_uint64_bit_count;
    size_t bit_index_within_word = bit_index % c_uint64_bit_count;
    uint64_t word = bitmap[word_index];
    uint64_t mask = ((1ULL << width) - 1) << bit_index_within_word;
    uint64_t bitstring = (word & mask) >> bit_index_within_word;

    return bitstring;
}

void set_bitstring_in_word(
    uint64_t& word, size_t bit_index, uint64_t bitstring, size_t width)
{
    ASSERT_PRECONDITION(bitstring < (1ULL << width), "Bitstring value must fit into bitstring width!");
    ASSERT_PRECONDITION(bit_index + width <= c_uint64_bit_count, "Bitstring must fit into word!");
    ASSERT_PRECONDITION((bit_index % width == 0), "Bit index must be a multiple of bitstring width!");

    uint64_t word_copy = word;
    // Clear all bits in the bitstring's range.
    uint64_t mask = ((1ULL << width) - 1) << bit_index;
    word_copy &= ~mask;
    // Copy the bitstring into its range.
    word_copy |= (bitstring << bit_index);
    word = word_copy;
}

bool try_set_bitstring_in_word(
    std::atomic<uint64_t>& word, size_t bit_index, uint64_t bitstring, size_t width)
{
    // We read the word once, because other threads may be updating it.
    uint64_t old_word = word;
    uint64_t new_word = old_word;

    set_bitstring_in_word(new_word, bit_index, bitstring, width);

    return word.compare_exchange_strong(old_word, new_word);
}

void safe_set_bitstring_in_word(
    std::atomic<uint64_t>& word, size_t bit_index, uint64_t bitstring, size_t width)
{
    while (!try_set_bitstring_in_word(word, bit_index, bitstring, width))
    {
        // Someone else made an update; retry after reading updated word value.
    }
}

// This function is threadsafe; we could also have a non-threadsafe equivalent if needed.
void set_bitstring_at_index(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size, size_t bit_index, uint64_t bitstring, size_t width)
{
    validate_bitmap_parameters(bitmap, bitmap_word_size);
    validate_bit_index(bitmap_word_size, bit_index);

    // The bitstring width must divide the word length, i.e. it must be a power of 2 <= 64.
    ASSERT_PRECONDITION((width <= c_uint64_bit_count) && (c_uint64_bit_count % width == 0), "Bitstring width must divide word size!");

    // The bit index must be a multiple of the bitstring width.
    ASSERT_PRECONDITION((bit_index % width == 0), "Bit index must be a multiple of bitstring width!");

    size_t word_index = bit_index / c_uint64_bit_count;
    size_t bit_index_within_word = bit_index % c_uint64_bit_count;

    safe_set_bitstring_in_word(bitmap[word_index], bit_index_within_word, bitstring, width);
}

void print_bitmap(
    std::atomic<uint64_t>* bitmap, size_t bitmap_word_size)
{
    validate_bitmap_parameters(bitmap, bitmap_word_size);

    cout << "\nBitmap value:" << endl;

    for (size_t word_index = 0; word_index < bitmap_word_size; ++word_index)
    {
        uint64_t word = bitmap[word_index];

        // Print bits in word from MSB to LSB (for compatibility with usual
        // binary representation of word).
        // NB: we need a signed integer for the loop index, or the loop will
        // become infinite (because an unsigned integer will overflow on
        // decrement past zero instead of becoming negative).
        for (ssize_t bit_index = c_uint64_bit_count - 1; bit_index >= 0; --bit_index)
        {
            uint64_t bit_value = (word >> bit_index) & 1;

            if (((bit_index + 1) % (sizeof(uint16_t) * CHAR_BIT)) == 0)
            {
                cout << endl;
            }

            cout << bit_value << " ";
        }

        cout << endl;
    }
}

} // namespace memory_manager
} // namespace db
} // namespace gaia
