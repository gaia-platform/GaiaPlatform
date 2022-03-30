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
