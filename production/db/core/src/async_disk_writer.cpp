/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "async_disk_writer.hpp"

#include <liburing.h>

#include <climits>

#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>

#include <sys/eventfd.h>

#include "gaia_internal/common/fd_helpers.hpp"
#include "gaia_internal/common/retail_assert.hpp"

#include "async_write_batch.hpp"
#include "persistence_types.hpp"
#include "txn_metadata.hpp"

using namespace gaia::db;
using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace persistence
{

async_disk_writer_t::async_disk_writer_t(int validate_flush_efd, int signal_checkpoint_efd)
{
    ASSERT_PRECONDITION(validate_flush_efd >= 0, "Invalid validate flush eventfd");
    ASSERT_PRECONDITION(signal_checkpoint_efd >= 0, "Invalid signal checkpoint eventfd");

    m_validate_flush_efd = validate_flush_efd;
    m_signal_checkpoint_efd = signal_checkpoint_efd;

    // Used to block new writes to disk when a batch is already getting flushed.
    s_flush_efd = eventfd(1, 0);
    if (s_flush_efd == -1)
    {
        const char* reason = ::explain_eventfd(1, 0);
        throw_system_error(reason);
    }
}

void async_disk_writer_t::open(size_t batch_size)
{
    if (!m_in_progress_batch)
    {
        m_in_progress_batch = std::make_unique<async_write_batch_t>();
    }
    if (!m_in_flight_batch)
    {
        m_in_flight_batch = std::make_unique<async_write_batch_t>();
    }

    m_in_progress_batch->setup(batch_size);
    m_in_flight_batch->setup(batch_size);
}

async_disk_writer_t::~async_disk_writer_t()
{
    teardown();
}

void async_disk_writer_t::teardown()
{
    close_fd(s_flush_efd);
}

void async_disk_writer_t::throw_error(std::string err_msg, int err, uint64_t user_data)
{
    std::stringstream ss;
    ss << err_msg << "\n with operation return code = " << err;
    if (user_data != 0)
    {
        ss << "\n with cqe data = " << user_data;
    }
    throw_system_error(ss.str(), err);
}

void async_disk_writer_t::map_commit_ts_to_session_decision_efd(gaia_txn_id_t commit_ts, int session_decision_eventfd)
{
    m_ts_to_session_decision_eventfd_map.insert(std::pair(commit_ts, session_decision_eventfd));
}

void async_disk_writer_t::add_decisions_to_batch(decision_list_t& decisions)
{
    for (const auto& decision : decisions)
    {
        m_in_progress_batch->add_decision_to_batch(decision);
    }
}

void async_disk_writer_t::perform_post_completion_maintenance()
{
    // Validate IO completion events belonging to the in_flight batch.
    auto expected_completion_event_count = m_in_flight_batch->get_completion_count();
    for (size_t i = 0; i < expected_completion_event_count; i++)
    {
        m_in_flight_batch->validate_next_completion_event();
    }

    auto max_file_seq_to_close = m_in_flight_batch->get_max_file_seq_to_close();

    // Post validation, close all files in batch.
    m_in_flight_batch->close_all_files_in_batch();

    // Signal to checkpointing thread the upper bound of files that it can process.
    if (max_file_seq_to_close > 0)
    {
        signal_eventfd(m_signal_checkpoint_efd, max_file_seq_to_close);
    }

    const decision_list_t& decisions = m_in_flight_batch->get_decision_batch_entries();
    for (auto decision : decisions)
    {
        // Set durability flags for txn.
        transactions::txn_metadata_t::set_txn_durable(decision.commit_ts);

        // Unblock session thread.
        auto itr = m_ts_to_session_decision_eventfd_map.find(decision.commit_ts);
        ASSERT_INVARIANT(itr != m_ts_to_session_decision_eventfd_map.end(), "Unable to find session durability eventfd from committing txn's commit_ts");
        signal_eventfd_single_thread(itr->second);
        m_ts_to_session_decision_eventfd_map.erase(itr);
    }

    m_in_flight_batch->clear_decision_batch();
}

void async_disk_writer_t::submit_and_swap_in_progress_batch(int file_fd, bool should_wait_for_completion)
{
    eventfd_t event_counter;

    // Block on any pending disk flushes.
    eventfd_read(s_flush_efd, &event_counter);

    // Perform any maintenance on the in_flight batch.
    perform_post_completion_maintenance();
    finish_and_submit_batch(file_fd, should_wait_for_completion);
}

void async_disk_writer_t::finish_and_submit_batch(int file_fd, bool should_wait_for_completion)
{
    size_t submission_entries_needed = c_submit_batch_sqe_count;

    size_t in_progress_size = m_in_progress_batch->get_unsubmitted_entries_count();
    if (in_progress_size == 0)
    {
        swap_batches();

        // Nothing to submit; reset the flush efd that got burnt in submit_and_swap_in_progress_batch() function.
        signal_eventfd_single_thread(s_flush_efd);

        // Reset metadata buffer.
        m_metadata_buffer.clear();
    }
    in_progress_size += submission_entries_needed;

    // Append fdatasync operation.
    m_in_progress_batch->add_fdatasync_op_to_batch(file_fd, get_enum_value(uring_op_t::fdatasync), IOSQE_IO_LINK);

    // Signal eventfd's as part of batch.
    m_in_progress_batch->add_pwritev_op_to_batch(static_cast<void*>(&c_default_iov), 1, s_flush_efd, 0, get_enum_value(uring_op_t::pwritev_eventfd_flush), IOSQE_IO_LINK);
    m_in_progress_batch->add_pwritev_op_to_batch(static_cast<void*>(&c_default_iov), 1, m_validate_flush_efd, 0, get_enum_value(uring_op_t::pwritev_eventfd_validate), IOSQE_IO_DRAIN);

    swap_batches();
    auto flushed_batch_size = m_in_flight_batch->get_unsubmitted_entries_count();
    ASSERT_INVARIANT(in_progress_size == flushed_batch_size, "Failed to swap in-flight batch with in-progress batch.");
    size_t submission_count = 0;

    submission_count = m_in_flight_batch->submit_operation_batch(should_wait_for_completion);
    if (should_wait_for_completion)
    {
        perform_post_completion_maintenance();
    }

    ASSERT_INVARIANT(flushed_batch_size == submission_count, "Batch submission failure.");
    ASSERT_INVARIANT(m_in_flight_batch->get_unsubmitted_entries_count() == 0, "batch size after submission should be 0.");
}

void async_disk_writer_t::perform_file_close_operations(int file_fd, file_sequence_t log_seq)
{
    submit_if_full(file_fd, c_single_submission_entry_count);

    // Call fdatasync on the file before closing it.
    m_in_progress_batch->add_fdatasync_op_to_batch(file_fd, get_enum_value(uring_op_t::fdatasync), IOSQE_IO_DRAIN);

    // Append file fd to batch so that file can be closed post batch validation.
    m_in_progress_batch->append_file_to_batch(file_fd, log_seq);
}

unsigned char* async_disk_writer_t::copy_into_metadata_buffer(void* source, size_t size, int file_fd)
{
    auto current_ptr = m_metadata_buffer.get_current_ptr();
    ASSERT_PRECONDITION(current_ptr, "Invalid metadata buffer pointer.");
    ASSERT_PRECONDITION(size > 0, "Size to copy into metadata buffer must be greater than 0.");
    ASSERT_PRECONDITION(source, "Source pointer must be valid.");

    // Call submit synchronously once the metadata buffer is full.
    if (!m_metadata_buffer.has_enough_space(size))
    {
        bool should_wait_for_completion = true;
        submit_and_swap_in_progress_batch(file_fd, should_wait_for_completion);
        m_metadata_buffer.clear();
    }

    m_metadata_buffer.allocate(size);
    memcpy(current_ptr, source, size);

    // Return old pointer.
    return current_ptr;
}

size_t async_disk_writer_t::get_total_pwritev_size_in_bytes(void* iov_array, size_t count)
{
    auto ptr = static_cast<const iovec*>(iov_array);

    size_t size_in_bytes = 0;
    for (size_t i = 0; i < count; i++)
    {
        size_in_bytes += ptr->iov_len;
        ptr++;
    }
    return size_in_bytes;
}

void async_disk_writer_t::enqueue_pwritev_requests(
    std::vector<iovec>& writes_to_submit,
    int file_fd,
    size_t current_log_file_offset,
    uring_op_t type)
{
    ASSERT_PRECONDITION(!writes_to_submit.empty(), "Cannot enqueue 0 writes to the submission queue!");

    size_t required_size_bytes = sizeof(iovec) * writes_to_submit.size();
    unsigned char* current_helper_ptr = copy_into_metadata_buffer(writes_to_submit.data(), required_size_bytes, file_fd);

    // Each pwritev call can accomodate IOV_MAX entries at most.
    size_t num_pwrites = writes_to_submit.size() / IOV_MAX + 1;

    ASSERT_PRECONDITION(num_pwrites > 0, "Cannot enqueue 0 writes to the submission queue!");

    size_t remaining_size_bytes = required_size_bytes;
    size_t remaining_count = writes_to_submit.size();

    for (size_t i = 0; i < num_pwrites; i++)
    {
        unsigned char* ptr_to_write = current_helper_ptr;

        if (remaining_count <= IOV_MAX)
        {
            enqueue_pwritev_request(ptr_to_write, remaining_count, file_fd, current_log_file_offset, type);
            break;
        }
        else
        {
            ASSERT_PRECONDITION(remaining_count >= IOV_MAX, "Expected iov count greater than IOV_MAX.");
            ASSERT_PRECONDITION(remaining_size_bytes >= c_max_iovec_array_size_bytes, "Expected total pwrite size greater than max pwrite call size.");
            enqueue_pwritev_request(ptr_to_write, IOV_MAX, file_fd, current_log_file_offset, type);
            current_log_file_offset += get_total_pwritev_size_in_bytes(current_helper_ptr, IOV_MAX);
            remaining_size_bytes -= c_max_iovec_array_size_bytes;
            current_helper_ptr += c_max_iovec_array_size_bytes;
            remaining_count -= IOV_MAX;
        }
    }
}

void async_disk_writer_t::submit_if_full(int file_fd, size_t required_entries)
{
    required_entries = c_submit_batch_sqe_count + required_entries;
    bool should_submit = m_in_progress_batch.get()->get_unused_submission_entries_count() <= required_entries;
    if (should_submit)
    {
        submit_and_swap_in_progress_batch(file_fd);
    }
}

void async_disk_writer_t::enqueue_pwritev_request(
    void* iovec_array,
    size_t iovec_array_count,
    int file_fd,
    uint64_t current_offset,
    uring_op_t type)
{
    submit_if_full(file_fd, c_single_submission_entry_count);
    m_in_progress_batch->add_pwritev_op_to_batch(iovec_array, iovec_array_count, file_fd, current_offset, get_enum_value(type), 0);
}

void async_disk_writer_t::swap_batches()
{
    std::swap(m_in_flight_batch, m_in_progress_batch);
}

} // namespace persistence
} // namespace db
} // namespace gaia
