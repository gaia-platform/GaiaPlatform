/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include <atomic>
#include <ostream>

#include "gaia/common.hpp"

#include "gaia_internal/common/mmap_helpers.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include <gaia_internal/common/topological_sort.hpp>
#include "gaia_internal/db/db_types.hpp"

struct chunk_info_helper_t
{
    toposort::graph<chunk_offset_t>::node_ptr node_ptr;
    gaia_offset_t min_offset;
    gaia_offset_t max_offset; 
};

namespace gaia
{
namespace db
{
namespace persistence
{

enum class record_type_t : size_t
{
    not_set = 0x0,
    txn = 0x1,
    decision = 0x2,
};

enum class decision_type_t : uint8_t
{
    undecided = 0,
    commit = 1,
    abort = 2,
};

struct decision_entry_t
{
    gaia_txn_id_t commit_ts;
    decision_type_t decision;
};

// Represents start and end offsets of a set of contiguous objects.
// They can span chunks.
struct contiguous_offsets_t
{
    gaia_offset_t offset1;
    gaia_offset_t offset2;
};

// Pair of log file sequence number and file fd.
typedef std::vector<decision_entry_t> decision_list_t;

class file_sequence_t : public common::int_type_t<size_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr file_sequence_t()
        : common::int_type_t<size_t, 0>()
    {
    }

    constexpr file_sequence_t(size_t value)
        : common::int_type_t<size_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(file_sequence_t) == sizeof(file_sequence_t::value_type),
    "file_sequence_t has a different size than its underlying integer type!");

constexpr file_sequence_t c_invalid_file_sequence_number;

static constexpr uint64_t c_max_log_file_size_in_bytes = 4 * 1024 * 1024;

class file_offset_t : public common::int_type_t<size_t, 0>
{
public:
    // By default, we should initialize to 0.
    constexpr file_offset_t()
        : common::int_type_t<size_t, 0>()
    {
    }

    constexpr file_offset_t(size_t value)
        : common::int_type_t<size_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(file_offset_t) == sizeof(file_offset_t::value_type),
    "file_offset_t has a different size than its underlying integer type!");

// The record size is constrained by the size of the log file.
// We'd never need more than 32 bits for the record size.
class record_size_t : public common::int_type_t<uint32_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr record_size_t()
        : common::int_type_t<uint32_t, 0>()
    {
    }

    constexpr record_size_t(uint32_t value)
        : common::int_type_t<uint32_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(record_size_t) == sizeof(record_size_t::value_type),
    "record_size_t has a different size than its underlying integer type!");

constexpr record_size_t c_invalid_record_size;

// https://stackoverflow.com/questions/2321676/data-length-vs-crc-length
// "From the wikipedia article: "maximal total blocklength is equal to 2**r − 1". That's in bits.
// So CRC-32 would have max message size 2^33-1 bits or about 2^30 bytes = 1GB
class crc32_t : public common::int_type_t<uint32_t, 0>
{
public:
    // By default, we should initialize to 0.
    constexpr crc32_t()
        : common::int_type_t<uint32_t, 0>()
    {
    }

    constexpr crc32_t(uint32_t value)
        : common::int_type_t<uint32_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(crc32_t) == sizeof(crc32_t::value_type),
    "crc32_t has a different size than its underlying integer type!");

// This assertion ensures that the default type initialization
// matches the value of the invalid constant.
static_assert(
    c_invalid_file_sequence_number.value() == file_sequence_t::c_default_invalid_value,
    "Invalid c_invalid_file_sequence_number initialization!");

struct log_file_info_t
{
    file_sequence_t sequence;
    int file_fd;
};

struct record_header_t
{
    crc32_t crc;
    record_size_t payload_size;
    record_type_t record_type;
    gaia_txn_id_t txn_commit_ts;

    // Stores a count value depending on the record type.
    // For a txn record, this represents the count of deleted objects.
    // For a decision record, this represents the number of decisions in the record's payload.
    union
    {
        uint32_t deleted_object_count;
        uint32_t decision_count;
    };
};

static_assert(
    sizeof(record_header_t) % 8 == 0,
    "record_header_t should be a multiple of 8 bytes!");

// The primary motivation of this buffer is to keep a hold of any additional information we want to write to the log
// apart from the shared memory objects.
// Custom information includes
// 1) deleted object IDs in a txn.
// 2) custom txn headers
// 3) Txn decisions
// 4) iovec entries to be supplied to the pwritev() call.
static constexpr size_t c_max_metadata_buf_size_bytes = 16 * 1024 * 1024;
struct metadata_buffer_t
{
    unsigned char* start;
    unsigned char* current_ptr;
    size_t remaining_size;

    metadata_buffer_t()
    {
        gaia::common::map_fd_data(
            start,
            c_max_metadata_buf_size_bytes,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
            -1,
            0);
        current_ptr = start;
        remaining_size = c_max_metadata_buf_size_bytes;
    }

    ~metadata_buffer_t()
    {
        gaia::common::unmap_fd_data(start, c_max_metadata_buf_size_bytes);
    }

    unsigned char* allocate(size_t size)
    {
        ASSERT_INVARIANT(has_enough_space(size), "metadata buffer ran out of space.");
        current_ptr += size;
        remaining_size -= size;
        return current_ptr;
    }

    bool has_enough_space(size_t size)
    {
        return size <= remaining_size;
    }

    unsigned char* get_current_ptr()
    {
        return current_ptr;
    }

    void clear()
    {
        current_ptr = start;
    }
};

} // namespace persistence
} // namespace db
} // namespace gaia
