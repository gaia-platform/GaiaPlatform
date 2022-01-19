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
#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{
namespace persistence
{

enum class recovery_mode_t : uint8_t
{
    not_set = 0x0,

    // Does not tolerate any IO failure when reading a log file; any
    // IO error is treated as unrecoverable.
    // This mode is used when checkpointing log writes to RocksDB.
    checkpoint = 0x1,

    // Stop recovery on first IO error. Database will always start and will try to recover as much
    // committed data from the log as possible.
    // Updates are logged one batch as a time; Persistent batch IO is validated
    // first before marking any txn in the batch as durable (and returning a commit decision to the user);
    // Thus ignore any txn after the last seen decision timestamp before encountering IO error.
    finish_on_first_error = 0x2,
};

enum class record_type_t : uint8_t
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

typedef size_t file_offset_t;

// The record size is constrained by the size of the log file.
// We'd never need more than 32 bits for the record size.
typedef uint32_t record_size_t;

// https://stackoverflow.com/questions/2321676/data-length-vs-crc-length
// "From the wikipedia article: "maximal total blocklength is equal to 2**r − 1". That's in bits.
// So CRC-32 would have max message size 2^33-1 bits or about 2^30 bytes = 1GB
typedef uint32_t crc32_t;

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

    char padding[3];
};

struct read_record_t
{
    struct record_header_t header;
    unsigned char payload[];
};

struct record_iterator_t
{
    unsigned char* cursor;
    unsigned char* end;
    unsigned char* stop_at;
    unsigned char* begin;
    void* mapped;
    size_t map_size;
    int file_fd;
    recovery_mode_t recovery_mode;
    bool halt_recovery;
};

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
