/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// This file contains encoders/decoders for Gaia objects to Key Value Objects (RocksDB slices)
// A slice structure contains a pointer to a byte array and its length.
// Note that all ints (excluding int8) are stored as big-endian for normalization purposes.

#pragma once

#include <vector>

#include "rocksdb/slice.h"

#include "gaia_internal/common/persistent_store_error.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_object.hpp"

/**
 * This file assumes big-endian architectures aren't supported which is why all encoding & decoding
 * to and from a specific byte ordering is avoided.
 */
namespace gaia
{
namespace db
{
namespace persistence
{

/**
 * String writer library containing a byte buffer and current length; used for serializing
 * gaia objects to rocksdb slices.
 */
class string_writer_t
{
private:
    std::vector<char> m_buffer;

public:
    string_writer_t() = default;

    string_writer_t(size_t len)
    {
        m_buffer.reserve(len);
    }

    inline void write_uint64(uint64_t value)
    {
        write(reinterpret_cast<const char* const>(&value), sizeof(value));
    }

    inline void write_uint32(uint32_t value)
    {
        write(reinterpret_cast<const char* const>(&value), sizeof(value));
    }

    inline void write_uint16(uint16_t value)
    {
        write(reinterpret_cast<const char* const>(&value), sizeof(value));
    }

    inline void write_uint8(uint8_t value)
    {
        write(reinterpret_cast<const char* const>(&value), sizeof(value));
    }

    inline void write(const char* const payload, size_t len)
    {
        m_buffer.reserve(m_buffer.size() + len);
        std::copy(payload, payload + len, std::back_inserter(m_buffer));
    }

    // Method to obtain slice from buffer.
    inline rocksdb::Slice to_slice()
    {
        return rocksdb::Slice(const_cast<const char*>(m_buffer.data()), m_buffer.size());
    }

    inline size_t get_current_position()
    {
        return m_buffer.size();
    }

    inline void clear()
    {
        // NB: This will not deallocate the vector's internal buffer.
        m_buffer.clear();
    }
};

/**
 * String reader library used during deserialization of slices to gaia objects.
 */
class string_reader_t
{
private:
    // Starting position of data pointer in slice.
    const char* const m_starting_byte;
    // Current ptr to keep track of char's read in slice.
    const char* m_current_byte;
    // Total size of slice in bytes.
    const size_t m_size;

public:
    string_reader_t() = delete;

    string_reader_t(const rocksdb::Slice& slice)
        : m_starting_byte(slice.data()), m_current_byte(slice.data()), m_size(slice.size())
    {
    }

    inline void read_uint64(uint64_t& out)
    {
        const char* value_ptr = read(sizeof(uint64_t));
        uint64_t value;
        memcpy(&value, value_ptr, sizeof(uint64_t));
        out = value;
    }

    inline void read_uint32(uint32_t& out)
    {
        const char* value_ptr = read(sizeof(uint32_t));
        uint32_t value;
        memcpy(&value, value_ptr, sizeof(uint32_t));
        out = value;
    }

    inline void read_uint16(uint16_t& out)
    {
        const char* value_ptr = read(sizeof(uint16_t));
        uint16_t value;
        memcpy(&value, value_ptr, sizeof(uint16_t));
        out = value;
    }

    inline void read_byte(uint8_t& out)
    {
        const char* value_ptr = read(sizeof(uint8_t));
        out = *(reinterpret_cast<const uint8_t*>(value_ptr));
    }

    inline size_t get_remaining_len_in_bytes()
    {
        // We check that m_current_byte is within bounds in read().
        return m_size - (m_current_byte - m_starting_byte);
    }

    inline const char* read(const size_t size)
    {
        ASSERT_PRECONDITION(get_remaining_len_in_bytes() >= size, "Not enough bytes remaining to read!");

        // We shouldn't return an out-of-bounds pointer even for zero-length reads.
        if (get_remaining_len_in_bytes() == 0)
        {
            return nullptr;
        }

        // We need to allow the current pointer to point to the position just
        // after the last slice entry, as long as we never read from it.
        const char* result = m_current_byte;
        m_current_byte += size;
        size_t current_pos = m_current_byte - m_starting_byte;

        // We need to allow the current pointer to point to the position just
        // after the last slice entry, as long as we never read from it.
        ASSERT_INVARIANT(current_pos <= m_size, "Current pointer has overrun slice array bounds!");
        return result;
    }
};

void encode_object(
    const db_object_t* gaia_object,
    string_writer_t& key,
    string_writer_t& value);

void encode_checkpointed_object(
    const db_recovered_object_t* gaia_object,
    string_writer_t& key,
    string_writer_t& value);

db_object_t* decode_object(
    const rocksdb::Slice& key,
    const rocksdb::Slice& value);

} // namespace persistence
} // namespace db
} // namespace gaia
