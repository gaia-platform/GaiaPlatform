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

#include "gaia_internal/common/io_uring_error.hpp"
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
    FSYNC = 5,
};

// For simplicity all APIs in this file assume that the io_uring submission queue has enough space to write to.
// The caller should verify that enough space exists before queuing requests to the ring.
class io_uring_wrapper_t
{
private:
    // Size can only be a power of 2 and the max value is 4096.
    static constexpr size_t c_buffer_size = 32;

    static constexpr char c_setup_err_msg[] = "IOUring setup failed.";
    static constexpr char c_buffer_empty_err_msg[] = "IOUring submission queue out of space.";

    // IOUring instance. Each ring maintains a submission queue and a completion queue.
    std::unique_ptr<io_uring> ring;

    void prep_sqe(uint64_t data, u_char flags, io_uring_sqe* sqe);

    io_uring_sqe* get_sqe();

    // Keep track of all persistent log file_fds that need to be closed.
    std::vector<std::pair<int, size_t>> file_fds;

public:
    void close_all_files_in_batch();

    void append_file_to_batch(int fd, size_t file_size);

    void teardown();

    io_uring* get_ring();

    void append_pwritev(
        const struct iovec* iov,
        size_t iovcnt,
        int file_fd,
        uint64_t current_offset,
        uint64_t data,
        u_char flags);

    void append_fsync(
        int file_fd,
        uint32_t fsync_flags,
        uint64_t data,
        u_char flags);

    void close(int fd, uint64_t data, u_char flags);

    void open(size_t buffer_size = c_buffer_size);

    size_t submit(bool wait);

    size_t space_left();

    size_t count_unsubmitted_entries();

    size_t get_completion_count();

    /**
     * Returns an I/O completion, if one is readily available. Doesn’t wait.
     * Returns 0 with cqe filled in on success, -errno on failure.
     */
    int get_completion_event(struct io_uring_cqe** cqe);

    void mark_completion_seen(struct io_uring_cqe* cqe);

    io_uring_wrapper_t()
        : ring(nullptr){};

    ~io_uring_wrapper_t();

    // Decisions that belong to this batch.
    decision_list_t batch_decisions;
};
} // namespace db
} // namespace gaia
