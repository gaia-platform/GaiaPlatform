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
     * Initializes io_uring buffers. 
     * Setting up each ring will create two shared memory queues.
    */
    io_uring_manager_t();

    /**
     * Tear down function for io_uring. Unmaps all setup shared ring buffers 
     * and closes the low-level io_uring file descriptors returned by the kernel on setup.
     */
    ~io_uring_manager_t();

    void open(size_t buffer_size = 64);

    // The buffer that is getting flushed to disk.
    std::unique_ptr<io_uring_wrapper_t> in_flight_buffer;

    // The buffer where new writes are added.
    std::unique_ptr<io_uring_wrapper_t> in_progress_buffer;

    // Signifies flush is in progress.
    static inline int flush_efd = 0;

    // Reserve slots in the buffer to be able to append additional operations in a batch before it gets submitted to the kernel.
    static constexpr size_t submit_batch_sqe_count = 3;
    static constexpr eventfd_t default_flush_efd_value = 1;

    void handle_io_uring_error(bool condition, std::string err_msg, int err);

    // Swap in_flight and in_progress batches.
    void swap_buffers();

    size_t pwritev(
        const struct iovec* iov,
        size_t iovcnt,
        int file_fd,
        uint64_t current_offset);

    void construct_pwritev(
        std::vector<iovec>& writes_to_submit,
        int file_fd,
        uint64_t current_offset);

    size_t handle_submit(int file_fd, bool validate = false);

    size_t handle_file_close(int fd);

    void* get_header_ptr(
        int file_fd,
        record_header_t& record_header);

    uint8_t* copy_into_helper_buffer(void* source, size_t size, int file_fd);

    int get_flush_efd();

    size_t get_submission_batch_size(bool in_progress = true);

    size_t get_completion_batch_size(bool in_progress = true);

    void validate_completion_batch();

private:
    void teardown();

    bool submit_if_full(int file_fd, size_t required_size);
};

} // namespace db
} // namespace gaia
