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

    // We use the Liburing API to perform asynchronous writes to disk.
    // Writes to disk are batched and we maintain two io_uring buffers
    // (represented by the following fd's) so that writes to an io_uring buffer can
    // still proceed when one buffer is getting flushed to disk.
    // static io_uring_wrapper_t buffer1;
    // static io_uring_wrapper_t buffer2;

    // Use unique ptr to track which buffer is in_flight buffer vs in_progress buffer.
    static inline std::unique_ptr<io_uring_wrapper_t> in_progress_buffer{};
    static inline std::unique_ptr<io_uring_wrapper_t> in_flight_buffer{};

    // Signifies flush is in progress and writes to a batch or additional flushing will block.
    static inline int flush_efd = 0;
    static inline bool is_flush_in_progress = false;

    // 3 additional SQE's required to submit a batch.
    static constexpr size_t submit_batch_sqe_count = 3;
    static constexpr eventfd_t default_flush_efd_value = 1;

    static void handle_io_uring_error(int ret, std::string err_msg);

    // Swap in_flight and in_progress batches.
    // Returns false if the swap is unsuccessful - which can occur when the other
    // batch is busy (getting flushed to disk)
    static bool swap_buffers();

    // From the point of view of atomicity,
    // all transaction writes will be queued to the same batch.
    // For this reason, we will induce a 'soft limit'
    // on the io_uring batch.
    // The submit call here will issue a non-blocking submit.
    // API assumes that the correct offset is provided.
    static void pwritev(
        const struct iovec* iov,
        size_t iovcnt,
        int file_fd,
        uint64_t current_offset,
        bool submit);

    static void write(
        const void* buf,
        size_t len,
        bool submit);

    static void handle_submit(int file_fd, bool validate);

    static void validate_in_flight_batch_if_ready();

private:
    static void teardown();

    static void validate_completion_batch();
};

} // namespace db
} // namespace gaia
