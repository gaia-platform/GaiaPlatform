/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// This file contains encoders/decoders for Gaia objects to Key Value Objects (RocskDB slices)
// A slice structure contains a pointer to a byte array and its length.
// Note that all ints (excluding int8) are stored as big endian for normalization purposes.

#pragma once

#include <vector>

#include "gaia_common.hpp"
#include "rocksdb/slice.h"

using namespace gaia::common;

namespace gaia {
namespace db {

/**
 * String writer library containing a byte buffer and current length; used for serializing
 * gaia objects to rocksdb slices.
 */
class string_writer {
   private:
    std::vector<u_char> m_buffer;
    size_t m_position;

   public:
    string_writer() {
        m_position = 0;
    }

    string_writer(const size_t len) {
        m_position = 0;
        m_buffer.resize(len);
    }

    void allocate(const size_t len) {
        if (m_buffer.size() < m_position + len) {
            m_buffer.resize(m_position + len);
        }
        m_position += len;
    }

    void write_uint64(const uint64_t value) {
        allocate(sizeof(uint64_t));
        // Convert to network order (big endian).
        u_int64_t result = htobe64(value);
        memcpy(m_buffer.data() + m_position - sizeof(uint64_t), &result, sizeof(result));
    }

    void write_uint32(const uint32_t value) {
        allocate(sizeof(uint32_t));
        // Convert to network order (big endian).
        u_int32_t result = htobe32(value);
        memcpy(m_buffer.data() + m_position - sizeof(uint32_t), &result, sizeof(result));
    }

    void write_uint8(const uint8_t value) {
        allocate(sizeof(uint8_t));
        *(m_buffer.data() + m_position - sizeof(uint8_t)) = value;
    }

    void write(const char* const payload, const size_t len) {
        allocate(len);
        memcpy(m_buffer.data() + m_position - len, payload, len);
    }

    // Method to obtain slice from buffer.
    rocksdb::Slice to_slice() {
        return rocksdb::Slice(reinterpret_cast<char*>(m_buffer.data()), m_position);
    }

    size_t get_current_position() {
        return m_position;
    }

    void clean() {
        m_position = 0;
    }
};

/**
 * String reader library used during deserialization of slices to gaia objects.
 */
class string_reader {
   private:
    // Current ptr to keep track of char's read in slice.
    const char* m_current;
    // Maintain remaining length for sanity purposes.
    size_t m_remaining;

   public:
    string_reader(const rocksdb::Slice* const slice) {
        if (!slice) {
            // Ideally data should never be accessed if length is 0; hence set to nullptr.
            m_remaining = 0;
            m_current = nullptr;
        } else {
            m_current = slice->data();
            m_remaining = slice->size();
        }
    }

    bool read_uint64(uint64_t* out) {
        const uint8_t* casted_res
            = reinterpret_cast<const uint8_t*>(read(sizeof(uint64_t)));

        if (casted_res) {
            uint64_t temp;
            memcpy(&temp, casted_res, sizeof(uint64_t));
            // Convert to little endian.
            *out = be64toh(temp);
            return true;  // Value read successfully.
        }

        return false;  // Error
    }

    bool read_uint32(uint32_t* out) {
        const uint8_t* casted_res
            = reinterpret_cast<const uint8_t*>(read(sizeof(uint32_t)));

        if (casted_res) {
            uint32_t temp;
            memcpy(&temp, casted_res, sizeof(uint32_t));
            // Convert to little endian.
            *out = be32toh(temp);
            return true;  // Value read successfully.
        }

        return false;  // Error
    }

    bool read_byte(uint8_t* out) {
        const uint8_t* casted_res
            = reinterpret_cast<const uint8_t*>(read(sizeof(uint8_t)));

        if (casted_res) {
            *out = *casted_res;
            return true;  // Value read successfully.
        }

        return false;  // Error
    }

    size_t get_remaining_len_in_bytes() {
        return m_remaining;
    }

    const char* read(const size_t size) {
        // Sanity check
        if (size > m_remaining) {
            return nullptr;
        }

        const char* result = m_current;
        m_remaining -= size;
        m_current += size;

        return result;
    }
};

/**
 * Utility class for for encoding/decoding gaia objects.
 */
class rdb_object_converter_util {
   public:
    static void encode_object(
        const uint64_t id,
        uint64_t type,
        uint32_t reference_count,
        uint32_t size,
        const char* payload,
        string_writer* key,
        string_writer* value);
    static const char* decode_object(
        const rocksdb::Slice& key,
        const rocksdb::Slice& value,
        gaia_id_t* id,
        gaia_type_t* type,
        uint32_t* reference_count,
        uint32_t* size);
};

}  // namespace db
}  // namespace gaia
