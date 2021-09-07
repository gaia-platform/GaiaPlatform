/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <climits>
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
namespace persistence
{

/**
 * This class is used by the server to perform asynchronous log writes to disk.
 * Internally manages two instances of async_write_batch_t; the in_flight batch and the 
 * in_progress batch.
 */
class async_disk_writer_t
{
public:
    async_disk_writer_t(int validate_flushed_batch_eventfd, int signal_checkpoint_eventfd);

    ~async_disk_writer_t();

    void open(size_t batch_size = c_async_batch_size);

    /**
     * Wrapper over throw_system_error()
     */
    void throw_error(std::string err_msg, int err, uint64_t user_data = 0);

    /**
     * Empties the contents of the in_progress batch into the in_flight batch; this API is usually
     * called when the in_progress batch has become full and we want to submit IO requests in the batch
     * to the kernel.
     */
    void swap_batches();

    /**
     * Create a single pwritev() request and enqueue it to the in_progress batch.
     * This API is used when iov_count is guaranteed to be lesser than or equal to IOV_MAX.
     */
    void enqueue_pwritev_request(
        void* iovec_array,
        size_t iov_count,
        int file_fd,
        uint64_t current_offset,
        uring_op_t type);

    /**
     * Create one or more pwritev() requests and enqueues them to the in_progress batch.
     * The size of writes_to_submit can be larger than IOV_MAX, internally this API uses
     * enqueue_pwritev_request()
     */
    void enqueue_pwritev_requests(
        std::vector<iovec>& writes_to_submit,
        int file_fd,
        size_t current_offset,
        uring_op_t type);

    /**
     * Calculate the total write size of an iovec array in bytes.
     */
    size_t get_total_pwritev_size_in_bytes(void* iovec_array, size_t count);

    /**
     * Append fdatasync to the in_progress_batch, empty the in_progress batch into the in_flight batch 
     * and submit the IO requests in the in_flight batch to the kernel. Note that this API
     * will block on any other IO batches to finish disk flush before proceeding.
     */
    void submit_and_swap_in_progress_batch(int file_fd, bool should_wait_for_completion = false);

    /**
     * Append fdatasync to the in_progress_batch and update batch with file fd so that the file 
     * can be closed once the kernel has processed it.
     */
    void perform_file_close_operations(int file_fd, file_sequence_t log_seq);

    /**
     * Copy any temporary writes (which don't exist in gaia shared memory) into the metadata buffer.
     */
    unsigned char* copy_into_metadata_buffer(const void* source, size_t size, int file_fd);

    /**
     * Perform maintenance actions on in_flight batch after all of its IO entries have been processed.
     */
    void perform_post_completion_maintenance();

    void add_decisions_to_batch(const decision_list_t& decisions);

    /**
     * For each commit ts, keep track of the eventfd which the session thread blocks on. Once the txn
     * has been made durable, this eventfd is written to so that the session thread can make progress and
     * return commit decision to the client.
     */
    void map_commit_ts_to_session_decision_efd(gaia_txn_id_t commit_ts, int session_decision_eventfd);

    /**
     * Used for testing purposes.
     */
    int get_flush_eventfd()
    {
        return s_flush_eventfd;
    }

private:
    // Reserve slots in the in_progress batch to be able to append additional operations to it (before it gets submitted to the kernel)
    static constexpr size_t c_submit_batch_sqe_count = 3;
    static constexpr size_t c_single_submission_entry_count = 1;
    static constexpr size_t c_async_batch_size = 32;
    static constexpr size_t c_max_iovec_array_size_bytes = IOV_MAX * sizeof(iovec);
    static inline eventfd_t c_default_flush_eventfd_value = 1;
    static inline iovec c_default_iov = {static_cast<void*>(&c_default_flush_eventfd_value), sizeof(eventfd_t)};

    // eventfd to signal that a batch flush has completed.
    // Used to block new writes to disk when a batch is already getting flushed.
    static inline int s_flush_eventfd = -1;

    // eventfd to signal that the IO results belonging to a batch are ready to be validated.
    int m_validate_flush_eventfd = -1;

    // eventfd to signal that a file is ready to be checkpointed.
    int m_signal_checkpoint_eventfd = -1;

    // Keep track of session threads to unblock.
    std::unordered_map<gaia_txn_id_t, int> m_ts_to_session_decision_eventfd_map;

    // Writes are batched and we maintain two buffers so that writes to a buffer
    // can still proceed when the other buffer is getting flushed to disk.

    // The buffer that is getting flushed to disk.
    std::unique_ptr<async_write_batch_t> m_in_flight_batch;

    // The buffer where new writes are added.
    std::unique_ptr<async_write_batch_t> m_in_progress_batch;

    // The buffer to hold any additional information that needs to be written to the log
    // apart from the shared memory objects.
    metadata_buffer_t m_metadata_buffer;

private:
    void teardown();
    void submit_if_full(int file_fd, size_t required_size);
    void finish_and_submit_batch(int file_fd, bool wait);
};

} // namespace persistence
} // namespace db
} // namespace gaia
