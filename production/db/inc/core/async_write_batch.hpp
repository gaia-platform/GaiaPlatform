////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <cstddef>

#include <atomic>
#include <memory>
#include <string>
#include <unordered_set>

#include <liburing.h>

#include "gaia_internal/db/db_types.hpp"

#include "persistence_types.hpp"

namespace gaia
{
namespace db
{
namespace persistence
{

enum class uring_op_t : uint8_t
{
    nop = 0,
    pwritev_txn = 1,
    pwritev_decision = 2,
    pwritev_eventfd_flush = 3,
    pwritev_eventfd_validate = 4,
    fdatasync = 5,
};

/**
 * Library to perform async IO operations using io_uring(https://kernel.dk/io_uring.pdf) which is a Linux async I/O interface.
 * The liburing client library(https://unixism.net/loti/) is used to manage all io_uring system calls.
 * For simplicity all APIs in this file assume that the io_uring submission queue has enough space.
 * The caller should verify that enough space exists before enqueuing requests to the ring.
 */
class async_write_batch_t
{
public:
    async_write_batch_t() = default;

    ~async_write_batch_t();

    void close_all_files_in_batch();

    /**
     * Add file fd to the batch that should be closed once all of its pending writes have finished.
     */
    void append_file_to_batch(int fd, file_sequence_t log_seq);

    file_sequence_t get_max_file_seq_to_close();

    /**
     * https://man7.org/linux/man-pages/man2/pwritev.2.html
     */
    void add_pwritev_op_to_batch(
        void* iovec_array,
        size_t num_iovecs,
        int file_fd,
        uint64_t current_offset,
        uint64_t data,
        uint8_t flags);

    /**
     * https://man7.org/linux/man-pages/man2/fdatasync.2.html
     */
    void add_fdatasync_op_to_batch(
        int file_fd,
        uint64_t data,
        uint8_t flags);

    void setup(size_t buffer_size = c_buffer_size);

    size_t submit_operation_batch(bool wait);

    size_t get_unused_submission_entries_count();

    size_t get_unsubmitted_entries_count();

    size_t get_completion_count();

    void validate_next_completion_event();

    void mark_completion_seen(struct io_uring_cqe* cqe);

    void add_decision_to_batch(decision_entry_t decision);
    const decision_list_t& get_decision_batch_entries() const;
    size_t get_decision_batch_size();
    void clear_decision_batch();

private:
    // Size can only be a power of 2 and the max value is 4096.
    static constexpr size_t c_buffer_size = 32;

    static constexpr char c_setup_err_msg[] = "Failed to initialize io_uring instance (io_uring_queue_init returned error).";
    static constexpr char c_get_sqe_failure_err_msg[] = "Unable to get entry from submission queue.";

    // io_uring instance. Each ring maintains a submission queue and a completion queue.
    io_uring m_ring;

    // Keep track of all persistent log file_fds that need to be closed.
    std::vector<log_file_info_t> m_files_to_close;

    void prepare_submission_queue_entry(uint64_t data, u_char flags, io_uring_sqe* sqe);

    io_uring_sqe* get_submission_queue_entry();

    void teardown();

    // Decisions that belong to this batch.
    decision_list_t m_batch_decisions;
};

} // namespace persistence
} // namespace db
} // namespace gaia
