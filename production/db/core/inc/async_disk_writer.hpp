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

#include "async_write_batch.hpp"

namespace gaia
{
namespace db
{

/**
 * This class is used by the server to perform asynchronous log writes to disk.
 * Internally manages two instances of async_write_batch_t; the in_flight batch and the 
 * in_progress batch.
 */
class async_disk_writer_t
{
public:
    async_disk_writer_t(int validate_flushed_batch_efd, int signal_checkpoint_eventfd);

    ~async_disk_writer_t();

    void open(size_t buffer_size = 32);

    /**
     * Wrapper over throw_system_error()
     */
    void throw_io_uring_error(std::string err_msg, int err, uint64_t cqe_data = 0);

    /**
     * Empties the contents of the in_progress batch into the in_flight batch; this API is usually
     * called when the in_progress batch has become full and we want to submit IO requests in the batch
     * to the kernel.
     */
    void swap_batches();

    /**
     * Create a pwritev() request and enqueue it to the in_progress batch.
     */
    size_t pwritev(
        const struct iovec* iov,
        size_t iovcnt,
        int file_fd,
        uint64_t current_offset,
        uring_op_t type);

    /**
     * Create one or more pwritev() requests and enqueues them to the in_progress batch.
     */
    void construct_pwritev(
        std::vector<iovec>& writes_to_submit,
        int file_fd,
        size_t current_offset,
        uring_op_t type);

    /**
     * Calculate the total write size from an array of iovec.
     */
    size_t calculate_total_pwritev_size(const iovec* start, size_t count);

    /**
     * Append fdatasync to the in_progress_batch, empty the in_progress batch into the in_flight batch 
     * and submit the IO requests in the in_flight batch to the kernel. Note that this API
     * will block on any other IO batches to finish disk flush before proceeding.
     */
    size_t handle_submit(int file_fd, bool validate = false);

    /**
     * Append fdatasync to the in_progress_batch and update batch with file fd so that the file 
     * can be closed once the kernel has processed it.
     */
    size_t handle_file_close(int fd, uint64_t log_seq);

    /**
     * Copy any temporary writes (which don't exist in gaia shared memory) into the metadata buffer.
     */
    uint8_t* copy_into_metadata_buffer(void* source, size_t size, int file_fd);

    /**
     * Perform maintenance actions on in_flight batch after all of its IO entries have been processed.
     */
    void perform_post_completion_maintenance();

    void add_decisions_to_batch(decision_list_t& decisions);

    /**
     * For each commit ts, keep track of the eventfd which the session thread blocks on. Once the txn
     * has been made durable, this eventfd is written to so that the session thread can make progress and
     * return commit decision to the client.
     */
    void map_commit_ts_to_session_unblock_fd(gaia_txn_id_t commit_ts, int session_unblock_fd);

    /**
     * Register the function to make txn durable.
     */
    void register_txn_durable_fn(std::function<void(gaia_txn_id_t)> txn_durable_fn);

private:
    // Reserve slots in the in_progress batch to be able to append additional operations to it (before it gets submitted to the kernel)
    static constexpr size_t c_submit_batch_sqe_count = 3;
    static constexpr eventfd_t c_default_flush_efd_value = 1;
    static constexpr iovec c_default_iov = {(void*)&c_default_flush_efd_value, sizeof(eventfd_t)};

    // Event fd to signal that a batch flush has completed.
    // Used to block new writes to disk when a batch is already getting flushed.
    static inline int s_flush_efd = -1;

    // Event fd to signal that the IO results belonging to a batch are ready to be validated.
    static inline int s_validate_flush_efd = -1;

    // Event fd to signal that a file is ready to be checkpointed.
    static inline int s_signal_checkpoint_efd = -1;

    // Keep track of session threads to unblock.
    std::unordered_map<gaia_txn_id_t, int> m_ts_to_session_unblock_fd_map;

    // Function to mark txn durable.
    std::function<void(gaia_txn_id_t)> m_txn_durable_fn{};

    // Writes are batched and we maintain two buffers so that writes to a buffer
    // can still proceed when the other buffer is getting flushed to disk.
    // The buffer that is getting flushed to disk.
    std::unique_ptr<async_write_batch_t> m_in_flight_batch;

    // The buffer where new writes are added.
    std::unique_ptr<async_write_batch_t> m_in_progress_batch;

    metadata_buffer_t m_metadata_buffer;

    void teardown();
    bool submit_if_full(int file_fd, size_t required_size);
    size_t finish_and_submit_batch(int file_fd, bool wait);
};

} // namespace db
} // namespace gaia
