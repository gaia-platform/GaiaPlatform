/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// This file contains encoders/decoders for Gaia objects to Key Value Objects (RocskDB slices)
// A slice structure contains a pointer to a byte array and its length.
// Note that all ints (excluding int8) are stored as big endian for normalization purposes.

#pragma once

#include <endian.h>

#include <vector>

#include "rocksdb/slice.h"

#include "gaia_se_object.hpp"
#include "persistent_store_error.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{

/**
 * String writer library containing a byte buffer and current length; used for serializing
 * gaia objects to rocksdb slices.
 */
class string_writer_t
{
private:
    std::vector<u_char> m_buffer;
    size_t m_position;

public:
    string_writer_t()
    {
        m_position = 0;
    }

    string_writer_t(const size_t len)
    {
        m_position = 0;
        m_buffer.resize(len);
    }

    void allocate(const size_t len)
    {
        if (m_buffer.size() < m_position + len)
        {
            m_buffer.resize(m_position + len);
        }
        m_position += len;
    }

    void write_uint64(const uint64_t value)
    {
        allocate(sizeof(uint64_t));
        // Convert to network order (big endian).
        u_int64_t result = htobe64(value);
        memcpy(m_buffer.data() + m_position - sizeof(uint64_t), &result, sizeof(result));
    }

    void write_uint32(const uint32_t value)
    {
        allocate(sizeof(uint32_t));
        // Convert to network order (big endian).
        u_int32_t result = htobe32(value);
        memcpy(m_buffer.data() + m_position - sizeof(uint32_t), &result, sizeof(result));
    }

    void write_uint16(const uint16_t value)
    {
        allocate(sizeof(uint16_t));
        // Convert to network order (big endian).
        u_int16_t result = htobe16(value);
        memcpy(m_buffer.data() + m_position - sizeof(uint16_t), &result, sizeof(result));
    }

    void write_uint8(const uint8_t value)
    {
        allocate(sizeof(uint8_t));
        *(m_buffer.data() + m_position - sizeof(uint8_t)) = value;
    }

    void write(const char* const payload, const size_t len)
    {
        allocate(len);
        memcpy(m_buffer.data() + m_position - len, payload, len);
    }

    // Method to obtain slice from buffer.
    rocksdb::Slice to_slice()
    {
        return rocksdb::Slice(reinterpret_cast<char*>(m_buffer.data()), m_position);
    }

    size_t get_current_position()
    {
        return m_position;
    }

    void clean()
    {
        m_position = 0;
    }
};

/**
 * String reader library used during deserialization of slices to gaia objects.
 */
class string_reader_t
{
private:
    // Current ptr to keep track of char's read in slice.
    const char* m_current;
    // Maintain remaining length for sanity purposes.
    size_t m_remaining;

public:
    string_reader_t(const rocksdb::Slice* const slice)
    {
        if (!slice)
        {
            // Ideally data should never be accessed if length is 0; hence set to nullptr.
            m_remaining = 0;
            m_current = nullptr;
        }
        else
        {
            m_current = slice->data();
            m_remaining = slice->size();
        }
    }

    void read_uint64(uint64_t* out)
    {
        const uint8_t* casted_res
            = reinterpret_cast<const uint8_t*>(read(sizeof(uint64_t)));

        if (casted_res)
        {
            uint64_t temp;
            memcpy(&temp, casted_res, sizeof(uint64_t));
            // Convert to little endian.
            *out = be64toh(temp);
        }
        else
        {
            throw persistent_store_error("Unable to deserialize uint64 value.");
        }
    }

    void read_uint32(uint32_t* out)
    {
        const uint8_t* casted_res
            = reinterpret_cast<const uint8_t*>(read(sizeof(uint32_t)));

        if (casted_res)
        {
            uint32_t temp;
            memcpy(&temp, casted_res, sizeof(uint32_t));
            // Convert to little endian.
            *out = be32toh(temp);
        }
        else
        {
            throw persistent_store_error("Unable to deserialize uint32 value.");
        }
    }

    void read_uint16(uint16_t* out)
    {
        const uint8_t* casted_res
            = reinterpret_cast<const uint8_t*>(read(sizeof(uint16_t)));

        if (casted_res)
        {
            uint16_t temp;
            memcpy(&temp, casted_res, sizeof(uint16_t));
            // Convert to little endian.
            *out = be16toh(temp);
        }
        else
        {
            throw persistent_store_error("Unable to deserialize uint16 value.");
        }
    }

    void read_byte(uint8_t* out)
    {
        const uint8_t* casted_res
            = reinterpret_cast<const uint8_t*>(read(sizeof(uint8_t)));

        if (casted_res)
        {
            *out = *casted_res;
        }
        else
        {
            throw persistent_store_error("Unable to deserialize uint8 value.");
        }
    }

    size_t get_remaining_len_in_bytes()
    {
        return m_remaining;
    }

    const char* read(const size_t size)
    {
        // Sanity check
        if (size > m_remaining)
        {
            return nullptr;
        }

        const char* result = m_current;
        m_remaining -= size;
        m_current += size;

        return result;
    }
};

void encode_object(
    const gaia_se_object_t* gaia_object,
    string_writer_t* key,
    string_writer_t* value);

gaia_type_t decode_object(
    const rocksdb::Slice& key,
    const rocksdb::Slice& value);

gaia_id_t decode_key(const rocksdb::Slice& key);

} // namespace db
} // namespace gaia
