/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// This file contains encoders/decoders for Gaia objects to Key Value Objects (RocskDB slices)
// A slice structure contains a pointer to a byte array and its length.
// Note that all ints (excluding int8) are stored as big endian for normalization purposes.

#pragma once

#include "rocksdb/slice.h"
#include "storage_engine.hpp"
#include "vector"

namespace gaia { 

namespace db {

/**
 * Enum to represent gaia object types. Currently, the only types are nodes and edges
 */
enum GaiaObjectType: u_char {
    node = 0x0,
    edge = 0x1
};

/**
 * String writer library containing a byte buffer and current length; used for serializing 
 * gaia objects to rocksdb slices
 */
class string_writer {
    private: 
    std::vector<u_char> buf;
    size_t pos;

    public:
    string_writer() {
        pos = 0;
    }

    string_writer(const size_t len) {
        pos = 0;
        buf.resize(len);
    }

    void alloc(const size_t len) {
        if (buf.size() < pos + len) {
            buf.resize(pos + len);
        }
        pos += len;
    }

    void write_uint64(const uint64_t value) {
        alloc(sizeof(uint64_t));
        //convert to network order (big endian)
        u_int64_t result = htobe64(value);
        memcpy(buf.data() + pos - sizeof(uint64_t), &result, sizeof(result));
    }

    void write_uint32(const uint32_t value) {
        alloc(sizeof(uint32_t));
        //convert to network order (big endian)
        u_int32_t result = htobe32(value);
        memcpy(buf.data() + pos - sizeof(uint32_t), &result, sizeof(result));
    }

    void write_byte(const u_char value) {
        alloc(sizeof(u_char));
        *(buf.data() + pos - sizeof(u_char)) = value;
    }

    void write(const char* const payload, const size_t len) {
        alloc(len);
        memcpy(buf.data() + pos - len, payload, len);
    }

    // Method to obtain slice from buffer
    rocksdb::Slice to_slice() {
        return rocksdb::Slice(reinterpret_cast<char *>(buf.data()), pos);
    }

    size_t get_current_pos() {
        return pos;
    }

    void clean() {
        pos = 0;
    }
};

/**
 * String reader library used during deserialization of slices to gaia objects
 */
class string_reader {
    private: 
    // current ptr to keep track of char's read in slice
    const char* curr_ptr;
    // maintain remaining length for sanity purposes
    u_int rem_len;

    public:
    string_reader(const rocksdb::Slice* const slice) {
        if (!slice) {
            // Ideally data should never be accessed if length is 0; hence set to nullptr
            rem_len = 0;
            curr_ptr = nullptr;
        } else {
            curr_ptr = slice->data();
            rem_len = slice->size();
        }
    }

    bool read_uint64(u_int64_t* out) {
        const u_char* casted_res = 
        reinterpret_cast<const u_char *>(read(sizeof(u_int64_t)));

        if (casted_res) {
            u_int64_t temp;
            memcpy(&temp, casted_res, sizeof(u_int64_t));
            // Convert to little endian
            *out = be64toh(temp);
            return true; // value read successfully 
        }

        return false; // error
    }

    bool read_uint32(u_int32_t* out) {
        const u_char* casted_res = 
        reinterpret_cast<const u_char *>(read(sizeof(u_int32_t)));

        if (casted_res) {
            uint32_t temp;
            memcpy(&temp, casted_res, sizeof(u_int32_t));
            // Convert to little endian
            *out = be32toh(temp);
            return true; // value read successfully 
        }

        return false; // error
    }

    bool read_byte(u_char* out) {
        const u_char* casted_res = 
        reinterpret_cast<const u_char *>(read(sizeof(u_char)));

        if (casted_res) {
            *out = *casted_res;
            return true; // value read successfully 
        }

        return false; // error
    }

    u_int get_remaining_len_in_bytes () {
        return rem_len;
    }

    const char* read(const u_int size) {
        //sanity check
        if (size > rem_len) {
            return nullptr;
        }

        const char* result = curr_ptr;
        rem_len -= size;
        curr_ptr += size;

        return result;
    }

};

/**
 * Utility class for for encoding/decoding gaia objects
 */
class rdb_object_converter_util {
    public:
    static void encode_node(const u_int64_t id, u_int64_t type, u_int32_t size, const char* payload,
                            string_writer* key, string_writer* value);
    static void encode_edge(const u_int64_t id, u_int64_t type, u_int32_t size, const char* payload,
                            const u_int64_t first, const u_int64_t second,
                            string_writer* k, string_writer* v);
    static const char* decode_node(const rocksdb::Slice& key, const rocksdb::Slice& value,
                     gaia_id_t* id, gaia_type_t* type, u_int32_t* size);
    static const char* decode_edge(const rocksdb::Slice& key, const rocksdb::Slice& value, 
                            gaia_id_t* id, gaia_type_t* type, u_int32_t* size,
                            gaia_id_t* first, gaia_id_t* second);
    static bool is_rdb_object_edge(const rocksdb::Slice& value);
};

}

}