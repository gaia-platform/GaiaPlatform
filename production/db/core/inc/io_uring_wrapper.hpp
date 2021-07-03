/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include <atomic>
#include <memory>
#include <string>
#include <unordered_set>

#include "gaia_internal/db/db_types.hpp"

#include "liburing.h"
#include "persistence_types.hpp"

namespace gaia
{
namespace db
{

enum class uring_op_t : uint64_t
{
    NOT_SET = 0,
    PWRITEV_TXN = 1,
    PWRITEV_DECISION = 2,
    PWRITEV_EVENTFD_FLUSH = 3,
    PWRITEV_EVENTFD_VALIDATE = 4,
    FDATASYNC = 5,
};

/**
 * Library to perform async IO operations using io_uring which is a Linux async I/O interface.
 * The liburing client library is used to manage all io_uring system calls.
 * For simplicity all APIs in this file assume that the io_uring submission queue has enough space.
 * The caller should verify that enough space exists before queuing requests to the ring.
 */
class io_uring_wrapper_t
{
public:
    io_uring_wrapper_t()
        : m_ring(){};

    ~io_uring_wrapper_t();

    void close_all_files_in_batch();

    void append_file_to_batch(int fd);

    void add_pwritev_op_to_batch(
        const iovec* iovecs,
        size_t num_iovecs,
        int file_fd,
        uint64_t current_offset,
        uint64_t data,
        u_char flags);

    void add_fdatasync_op_to_batch(
        int file_fd,
        uint64_t data,
        u_char flags);

    void open(size_t buffer_size = c_buffer_size);

    size_t submit_operation_batch(bool wait);

    size_t get_unused_submission_entries_count();

    size_t get_unsubmitted_entries_count();

    size_t get_completion_count();

    /**
     * Returns an I/O completion, if one is readily available. Doesn’t wait.
     * Returns 0 with cqe filled in on success, -errno on failure.
     */
    int get_completion_event(struct io_uring_cqe** cqe);

    void mark_completion_seen(struct io_uring_cqe* cqe);

    // Decisions that belong to this batch.
    decision_list_t batch_decisions;

private:
    // Size can only be a power of 2 and the max value is 4096.
    static constexpr size_t c_buffer_size = 32;

    static constexpr char c_setup_err_msg[] = "io_uring_queue_init failed.";
    static constexpr char c_buffer_empty_err_msg[] = "io_uring submission queue out of space.";

    // io_uring instance. Each ring maintains a submission queue and a completion queue.
    std::unique_ptr<io_uring> m_ring;

    void prep_sqe(uint64_t data, u_char flags, io_uring_sqe* sqe);

    io_uring_sqe* get_sqe();

    void teardown();

    // Keep track of all persistent log file_fds that need to be closed.
    std::vector<int> m_file_fds;
};
} // namespace db
} // namespace gaia
