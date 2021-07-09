/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "sys/eventfd.h"

#include "gaia_internal/db/db_types.hpp"

#include "io_uring_wrapper.hpp"

namespace gaia
{
namespace db
{

// This class will be used by the persistence thread to perform async writes to disk.
// Context: The persistence thread exists on the server and will scan the txn table for any new txn updates and
// will write them to the log. The session threads will signal to the persistence thread that new writes are available via an eventfd on
// txn commit. Additionally, the session threads will wait on the persistence thread to write its updates to disk before returning
// the commit decision to the client. Signaling between threads will occur via eventfds.
class async_disk_writer_t
{
public:
    /**
     * Initializes io_uring ring buffers. 
     * Setting up each ring will create two shared memory queues.
    */
    async_disk_writer_t(int validate_flushed_batch_efd, int signal_checkpoint_eventfd);

    /**
     * Tear down function for io_uring. Unmaps all setup shared ring buffers 
     * and closes the low-level io_uring file descriptors returned by the kernel on setup.
     */
    ~async_disk_writer_t();

    void open(size_t buffer_size = 64);

    void throw_io_uring_error(std::string err_msg, int err, uint64_t cqe_data = 0);

    // Swap in_flight and in_progress batches.
    void swap_buffers();

    size_t pwritev(
        const struct iovec* iov,
        size_t iovcnt,
        int file_fd,
        uint64_t current_offset,
        uring_op_t type);

    void construct_pwritev(
        std::vector<iovec>& writes_to_submit,
        int file_fd,
        persistent_log_file_offset_t current_offset,
        persistent_log_file_offset_t expected_final_offset,
        uring_op_t type);

    size_t calculate_total_pwritev_size(const iovec* start, size_t count);

    size_t handle_submit(int file_fd, bool validate = false);

    size_t handle_file_close(int fd, uint64_t log_seq);

    void* get_header_ptr(
        int file_fd,
        record_header_t& record_header);

    uint8_t* copy_into_metadata_buffer(void* source, size_t size, int file_fd);

    int get_flush_efd();

    size_t get_submission_batch_size(bool in_progress = true);

    size_t get_completion_batch_size(bool in_progress = true);

    void perform_post_completion_maintenence();

    void add_decisions_to_batch(decision_list_t& decisions);

    void map_commit_ts_to_session_unblock_fd(gaia_txn_id_t commit_ts, int session_unblock_fd);

    void register_txn_durable_fn(std::function<void(gaia_txn_id_t)> txn_durable_fn);

    bool batch_has_new_updates();

private:
    // Reserve slots in the buffer to be able to append additional operations in a batch before it gets submitted to the kernel.
    static constexpr size_t submit_batch_sqe_count = 3;
    static constexpr eventfd_t default_flush_efd_value = 1;
    static constexpr iovec default_iov = {(void*)&default_flush_efd_value, sizeof(eventfd_t)};

    static inline int flush_efd = -1;
    static inline int validate_flush_efd = -1;
    static inline int signal_checkpoint_efd = -1;

    std::unordered_map<gaia_txn_id_t, int> ts_to_session_unblock_fd_map;

    std::function<void(gaia_txn_id_t)> s_txn_durable_fn{};

    // Writes are batched and we maintain two buffers so that writes to a buffer
    // can still proceed when the other buffer is getting flushed to disk.
    // The buffer that is getting flushed to disk.
    std::unique_ptr<io_uring_wrapper_t> in_flight_buffer;
    // The buffer where new writes are added.
    std::unique_ptr<io_uring_wrapper_t> in_progress_buffer;

    void teardown();

    bool submit_if_full(int file_fd, size_t required_size);

    size_t finish_and_submit_batch(int file_fd, bool wait);

    // To store metadata.
    helper_buffer_t metadata_buffer;
};

} // namespace db
} // namespace gaia
