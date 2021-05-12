/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include <atomic>
#include <string>

#include "gaia_internal/common/io_uring_error.hpp"

#include "liburing.h"

namespace gaia
{
namespace db
{

// For simplicity all APIs in this file assume that the io_uring submission queue has enough space to write to.
// The caller should verify that enough space exists before queuing requests to the ring.
class io_uring_wrapper_t
{
public:
    static void teardown();

    io_uring_wrapper_t();

    ~io_uring_wrapper_t();

    static io_uring* get_ring();

    static void pwritev(
        const struct iovec* iov,
        size_t iovcnt,
        int file_fd,
        uint64_t current_offset,
        void* data,
        u_char flags);

    void write(
        const void* buf,
        size_t len,
        int file_fd,
        uint64_t current_offset,
        void* data,
        u_char flags);

    static void fsync(
        int file_fd,
        uint32_t fsync_flags,
        void* data,
        u_char flags);

    static void close(int fd);

    static void fallocate(int fd, int mode, size_t offset, size_t size);

    static size_t submit(bool wait);

    static size_t space_left();

    static size_t count_unsubmitted_entries();

    static size_t get_completion_count();

    /**
     * Returns an I/O completion, if one is readily available. Doesn’t wait.
     * Returns 0 with cqe filled in on success, -errno on failure.
     */
    static int get_completion_event(struct io_uring_cqe** cqe);

    static void mark_completion_seen(struct io_uring_cqe* cqe);

private:
    // Size can only be a power of 2 and the max value is 4096.
    static constexpr size_t c_buffer_size = 64;

    static constexpr char c_setup_err_msg[] = "IOUring setup failed.";
    static constexpr char c_buffer_empty_err_msg[] = "IOUring submission queue out of space.";

    // ring will internally maintain a submission queue and a completion queue which exist in shared memory.
    static struct io_uring* ring;

    static void prep_sqe(void* data, u_char flags, io_uring_sqe* sqe);
};
} // namespace db
} // namespace gaia
