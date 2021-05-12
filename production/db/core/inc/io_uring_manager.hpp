/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include <atomic>
#include <memory>
#include <string>

#include "sys/eventfd.h"

#include "gaia_internal/db/db_types.hpp"

#include "io_uring_wrapper.hpp"
#include "liburing.h"

namespace gaia
{
namespace db
{

// This class will be used by the persistence thread to perform async writes to disk.
// Context: The persistence thread exists on the server and will scan the txn table for any new txn updates and
// will write them to the log. The session threads will signal to the persistence thread that new writes are available via an eventfd on
// txn commit. Additionally, the session threads will wait on the persistence thread to write its updates to disk before returning
// the commit decision to the client. Signaling between threads will occur via eventfds.
class io_uring_manager_t
{
public:
    /**
     * Initializes io_uring ring buffers. 
     * Setting up each ring will create two shared memory queues.
    */
    io_uring_manager_t();

    /**
     * Tear down function for io_uring. Unmaps all setup shared ring buffers 
     * and closes the low-level io_uring file descriptors returned by the kernel on setup.
     */
    ~io_uring_manager_t();

    // Writes are batched and we maintain two buffers so that writes to a buffer
    // can still proceed when the other buffer is getting flushed to disk.

    // The buffer that is getting flushed to disk.
    static inline std::unique_ptr<io_uring_wrapper_t> in_flight_buffer{};

    // The buffer where new writes are added.
    static inline std::unique_ptr<io_uring_wrapper_t> in_progress_buffer{};

    // Signifies flush is in progress.
    static inline int flush_efd = 0;

    // Reserve slots in the buffer to be able to append additional operations in a batch before it gets submitted to the kernel.
    static constexpr size_t submit_batch_sqe_count = 3;
    static constexpr eventfd_t default_flush_efd_value = 1;

    static void handle_io_uring_error(int ret, std::string err_msg);

    // Swap in_flight and in_progress batches.
    static void swap_buffers();

    static void pwritev(
        const struct iovec* iov,
        size_t iovcnt,
        int file_fd,
        uint64_t current_offset,
        bool submit);

    static void handle_submit(int file_fd, bool validate);

    static void close_fd(int fd);

private:
    static void teardown();

    static void validate_completion_batch();
};

} // namespace db
} // namespace gaia
