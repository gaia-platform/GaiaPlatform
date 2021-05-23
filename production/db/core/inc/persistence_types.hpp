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

// Persistence types.
typedef uint32_t record_size_t;
typedef uint32_t crc32_t;
typedef uint32_t wal_sequence_t;
typedef uint32_t wal_file_offset_t;

// We don't need 8 bytes to represent file offset as the physical size of the log file
// will have a much shorter upper bound.
typedef uint32_t log_offset_t;

enum class record_type_t : uint8_t
{
    not_set = 0x0,
    txn = 0x1,
    txn_first = 0x2,
    txn_middle = 0x3,
    txn_last = 0x4,
    decision = 0x5,
    file_header = 0x6,
};

enum class txn_decision_type_t : uint8_t
{
    not_set = 0x0,
    commit = 0x1,
    abort = 0x2,
    in_progress = 0x3,
};

struct txn_decision_record_t
{
    wal_file_offset_t offset;
    txn_decision_type_t decision;
};

// Generate one for the current file and for every file we close.
// The first 4 bytes in this file will represent the crc and the next byte will
// represent the system endianess.
// Keep re-writing the metadata file every time.
struct txn_index_metadata_t
{
    size_t file_size;
    wal_sequence_t index;
    size_t decision_size;
    txn_decision_record_t decisions[];
};

struct txn_index_entry_t
{
    wal_sequence_t index;
    wal_file_offset_t offset;
};

struct record_header_t
{
    crc32_t crc; // 4

    record_size_t payload_size; // 4
    record_type_t record_type; // 1

    // The wal_index and the offset uniquely represent the location of a txn's updates in the log.
    txn_index_entry_t entry; // 8

    gaia_txn_id_t txn_commit_ts; // 8

    // Deleted IDs in the txn record.
    size_t deleted_count; // 8

    // Location of a decision record that came before the current decision record. Add it in the payload
    // of the decision record.
    // txn_index_entry_t prev_entry;
};

struct read_record_t
{
    record_header_t header;
    uint8_t payload[];
};

struct record_iterator
{
    uint8_t* cursor;
    uint8_t* end;

    /*
	 * Stop returning records as soon as their first byte
	 * (including the header) is at or after `stop_at`.
	 *
	 * This is initially equal to `end`, but can be shifted
	 * earlier in the record stream.  Note that a record that
	 * starts before `stop_at` will end after that pointer.
	 */
    uint8_t* stop_at;

    uint8_t* begin;

    void* mapped;
    size_t map_size;

    int file_fd;
};

// The primary motivation of this buffer is to keep a hold of any additional information we want to write to the log
// apart from the shared memory objects.
// Custom information includes
// 1) deleted txn IDs
// 2) custom txn headers
// 3) Txn decisions
// 4) iovec entries to be supplied to the pwritev() call.
// The amount of space needed is constrained by the size of the I/O batch.
// Force flush the current I/O batch if this buffer gets full.
static constexpr size_t c_max_helper_buf_size = 1000 * 1024 * 1024;

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

} // namespace db
} // namespace gaia
