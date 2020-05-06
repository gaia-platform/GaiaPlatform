/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// This file contains encoders/decoders for Gaia objects to Key Value Objects (RocskDB slices)
// A slice structure contains a pointer to a byte array and its length.
// Note that all ints (excluding int8) are stored as big endian for normalization purposes.

#pragma once

#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "storage_engine.hpp"
#include "vector"

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
        // Convert to network order (big endian)
        u_int64_t result = htobe64(value);
        memcpy(m_buffer.data() + m_position - sizeof(uint64_t), &result, sizeof(result));
    }

    void write_uint32(const uint32_t value) {
        allocate(sizeof(uint32_t));
        // Convert to network order (big endian)
        u_int32_t result = htobe32(value);
        memcpy(m_buffer.data() + m_position - sizeof(uint32_t), &result, sizeof(result));
    }

    void write_uint8(const u_int8_t value) {
        allocate(sizeof(u_int8_t));
        *(m_buffer.data() + m_position - sizeof(u_int8_t)) = value;
    }

    void write(const char* const payload, const size_t len) {
        allocate(len);
        memcpy(m_buffer.data() + m_position - len, payload, len);
    }

    // Method to obtain slice from buffer.
    rocksdb::Slice to_slice() {
        return rocksdb::Slice(reinterpret_cast<char *>(m_buffer.data()), m_position);
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
    // current ptr to keep track of char's read in slice
    const char* m_current;
    // maintain remaining length for sanity purposes
    u_int m_remaining;

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

    bool read_uint64(u_int64_t* out) {
        const u_char* casted_res = 
        reinterpret_cast<const u_char *>(read(sizeof(u_int64_t)));

        if (casted_res) {
            u_int64_t temp;
            memcpy(&temp, casted_res, sizeof(u_int64_t));
            // Convert to little endian.
            *out = be64toh(temp);
            return true; // Value read successfully. 
        }

        return false; // Error
    }

    bool read_uint32(u_int32_t* out) {
        const u_char* casted_res = 
        reinterpret_cast<const u_char *>(read(sizeof(u_int32_t)));

        if (casted_res) {
            uint32_t temp;
            memcpy(&temp, casted_res, sizeof(u_int32_t));
            // Convert to little endian.
            *out = be32toh(temp);
            return true; // Value read successfully.
        }

        return false; // Error
    }

    bool read_byte(u_char* out) {
        const u_char* casted_res = 
        reinterpret_cast<const u_char *>(read(sizeof(u_char)));

        if (casted_res) {
            *out = *casted_res;
            return true; // Value read successfully. 
        }

        return false; // Error
    }

    u_int get_remaining_len_in_bytes () {
        return m_remaining;
    }

    const char* read(const u_int size) {
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
    private:
    static void encode_object (
        gaia_id_t id,
        gaia_type_t type,
        u_int32_t size,
        const char* payload,
        bool is_edge,
        const gaia_id_t* first,
        const gaia_id_t* second,
        string_writer* key,
        string_writer* value
    );

    public:
    static const char* decode_object ( 
        const rocksdb::Slice& key,
        const rocksdb::Slice& value, 
        bool is_edge,
        gaia_id_t* id,
        gaia_type_t* type,
        u_int32_t* size,
        gaia_id_t* first,
        gaia_id_t* second
    );

    static gaia_ptr<gaia_se_node> decode_node (const rocksdb::Slice& key, const rocksdb::Slice& value);

    static gaia_ptr<gaia_se_edge> decode_edge(const rocksdb::Slice& key, const rocksdb::Slice& value);
                                   
    static bool is_rdb_object_edge(const rocksdb::Slice& value);

    static void encode_node(const gaia::db::gaia_se_node* payload, string_writer* key, string_writer* value);
    static void encode_edge(const gaia::db::gaia_se_edge* payload, string_writer* key, string_writer* value);

    static std::string get_error_message(const rocksdb::Status& status);

};

}

}
