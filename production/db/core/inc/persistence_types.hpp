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

// The primary motivation of this buffer is to keep a hold of any additional information we want to write to the log
// apart from the shared memory objects.
// Custom information includes
// 1) deleted txn IDs
// 2) custom txn headers
// 3) Txn decisions
// 4) iovec entries to be supplied to the pwritev() call.
static constexpr uint64_t c_max_metadata_buf_size = 16 * 1024 * 1024;
struct metadata_buffer_t
{
    uint8_t* start;
    uint8_t* current_ptr;
    size_t remaining_size;

    metadata_buffer_t()
    {
        gaia::common::map_fd_data(
            start,
            c_max_metadata_buf_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
            -1,
            0);
        current_ptr = &start[0];
        remaining_size = c_max_metadata_buf_size;
    }

    ~metadata_buffer_t()
    {
        gaia::common::unmap_fd_data(start, c_max_metadata_buf_size);
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

} // namespace persistence
} // namespace db
} // namespace gaia
