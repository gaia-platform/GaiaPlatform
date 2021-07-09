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
typedef uint32_t record_size_t;
typedef uint32_t crc32_t;
typedef size_t persistent_log_sequence_t;
typedef size_t persistent_log_file_offset_t;

enum class recovery_mode_t : uint8_t
{
    not_set = 0x0,

    // Does not tolerate any IO failure when reading a log file; any
    // IO error is treated as unrecoverable. Checkpointing does not tolerate any error.
    checkpoint = 0x1,

    // Stop recovery on first IO error. Database will always start and will try to recover as much
    // committed data from the log as possible.
    // Updates are logged one batch as a time; IO request results from a batch are validated
    // first before marking any txn in the batch as durable (and returning a commit decision to the user);
    // Thus ignore any txn after the last seen decision timestamp before encountering IO error.
    finish_on_first_error = 0x2,
};

enum class record_type_t : uint8_t
{
    not_set = 0x0,
    txn = 0x1,
    decision = 0x2,
    file_header = 0x3,
};

enum class decision_type_t : uint8_t
{
    not_set = 0,
    commit = 1,
    abort = 2,
};

struct decision_entry_t
{
    gaia_txn_id_t txn_commit_ts;
    decision_type_t decision;
};

struct record_header_t
{
    crc32_t crc; // 4 bytes
    record_size_t payload_size; // 4 bytes
    record_type_t record_type; // 1 byte
    gaia_txn_id_t txn_commit_ts; // 8 bytes

    // Stores a count value depending on the record type.
    // For txn record, this represents the deleted count.
    // For a decision record, this represents the number of decisions in the record's payload.
    uint32_t count; // 4 bytes

    //char padding[3];
};

struct read_record_t
{
    record_header_t header;
    uint8_t payload[];
};

struct record_iterator_t
{
    uint8_t* cursor;
    uint8_t* end;
    uint8_t* stop_at;
    uint8_t* begin;
    void* mapped;
    size_t map_size;
    int file_fd;
    recovery_mode_t recovery_mode;
    bool halt_recovery;
};

// The primary motivation of this buffer is to keep a hold of any additional information we want to write to the log
// apart from the shared memory objects.
// Custom information includes
// 1) deleted txn IDs
// 2) custom txn headers
// 3) Txn decisions
// 4) iovec entries to be supplied to the pwritev() call.
static constexpr uint64_t c_max_helper_buf_size = 16 * 1024 * 1024;

struct helper_buffer_t
{
    uint8_t* start;
    uint8_t* current_ptr;
    size_t remaining_size;

    helper_buffer_t()
    {
        gaia::common::map_fd_data(
            start,
            c_max_helper_buf_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
            -1,
            0);
        current_ptr = &start[0];
        remaining_size = c_max_helper_buf_size;
    }

    ~helper_buffer_t()
    {
        gaia::common::unmap_fd_data(start, c_max_helper_buf_size);
    }

    uint8_t* allocate(size_t size)
    {
        ASSERT_INVARIANT(has_enough_space(size), "IOUring buffer ran out of space.");
        current_ptr += size;
        remaining_size -= size;
        return current_ptr;
    }

    bool has_enough_space(size_t size)
    {
        return size < remaining_size;
    }

    uint8_t* get_current_ptr()
    {
        return current_ptr;
    }

    void clear()
    {
        current_ptr = start;
    }
};

// Pair of log file sequence number and file fd.
typedef std::pair<uint64_t, int> log_file_info_t;
typedef std::vector<decision_entry_t> decision_list_t;

} // namespace db
} // namespace gaia
