/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "async_disk_writer.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>

#include <sys/eventfd.h>

#include "gaia_internal/common/fd_helpers.hpp"
#include "gaia_internal/common/retail_assert.hpp"

#include "async_write_batch.hpp"
#include "liburing.h"
#include "persistence_types.hpp"

using namespace gaia::db;
using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace persistence
{

async_disk_writer_t::async_disk_writer_t(int validate_flush_efd0, int signal_checkpoint_eventfd0)
{
    m_in_progress_batch = std::make_unique<async_write_batch_t>();
    m_in_flight_batch = std::make_unique<async_write_batch_t>();
    ASSERT_INVARIANT(validate_flush_efd0 >= 0, "Invalid validate flush eventfd");

    s_validate_flush_efd = validate_flush_efd0;
    s_signal_checkpoint_efd = signal_checkpoint_eventfd0;

    // Used to block new writes to disk when a batch is already getting flushed.
    s_flush_efd = eventfd(1, 0);
    if (s_flush_efd == -1)
    {
        teardown();
    }
}

void async_disk_writer_t::open(size_t batch_size)
{
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

uint64_t get_enum_value(uring_op_t op)
{
    return static_cast<std::underlying_type<uring_op_t>::type>(op);
}

void async_disk_writer_t::throw_error(std::string err_msg, int err, uint64_t user_data)
{
    teardown();
    std::stringstream ss;
    ss << err_msg << "; with operation return code = " << err;
    if (user_data != 0)
    {
        ss << "; with cqe data = " << user_data;
    }
    throw_system_error(ss.str(), err);
}

void async_disk_writer_t::map_commit_ts_to_session_decision_efd(gaia_txn_id_t commit_ts, int session_decision_fd)
{
    m_ts_to_session_decision_fd_map.insert(std::pair(commit_ts, session_decision_fd));
}

void async_disk_writer_t::add_decisions_to_batch(decision_list_t& decisions)
{
    for (const auto& decision : decisions)
    {
        m_in_progress_batch->add_decision_to_batch(decision);
    }
}

void async_disk_writer_t::register_txn_durable_fn(std::function<void(gaia_txn_id_t)> txn_durable_fn)
{
    m_txn_durable_fn = txn_durable_fn;
}

void async_disk_writer_t::perform_post_completion_maintenance()
{
    // Validate IO completion events belonging to the in_flight batch.
    auto size_to_validate = m_in_flight_batch->get_completion_count();
    for (size_t i = 0; i < size_to_validate; i++)
    {
        m_in_flight_batch->validate_next_completion_event();
    }

    auto max_file_seq_to_close = m_in_flight_batch->get_max_file_seq_to_close();

    // Post validation, close all files in batch.
    m_in_flight_batch->close_all_files_in_batch();

    // Signal to checkpointing thread the upper bound of files that it can process.
    if (max_file_seq_to_close > 0)
    {
        signal_eventfd(s_signal_checkpoint_efd, max_file_seq_to_close);
    }

    const decision_list_t& decisions = m_in_flight_batch->get_decision_batch_entries();
    for (auto entry : decisions)
    {
        // Set durability flags for txn.
        m_txn_durable_fn(entry.commit_ts);

        // Unblock session thread.
        auto itr = m_ts_to_session_decision_fd_map.find(entry.commit_ts);
        ASSERT_INVARIANT(itr != m_ts_to_session_decision_fd_map.end(), "Unable to find fd of session to wakeup post log write.");
        signal_eventfd(itr->second, 1);
        m_ts_to_session_decision_fd_map.erase(itr);
    }

    m_in_flight_batch->clear_decision_batch();
}

size_t async_disk_writer_t::submit_and_swap_in_progress_batch(int file_fd, bool sync_submit)
{
    eventfd_t event_counter;

    // Block on any pending disk flushes.
    eventfd_read(s_flush_efd, &event_counter);

    // Perform any mantainance on the in_flight batch.
    perform_post_completion_maintenance();
    return finish_and_submit_batch(file_fd, sync_submit);
}

size_t async_disk_writer_t::finish_and_submit_batch(int file_fd, bool sync_submit)
{
    size_t submission_entries_needed = c_submit_batch_sqe_count;

    size_t in_prog_size = m_in_progress_batch->get_unsubmitted_entries_count();
    if (in_prog_size == 0)
    {
        swap_batches();

        // Nothing to submit; reset the flush efd that got burnt in handle_submit() function.
        signal_eventfd(s_flush_efd, 1);

        // Reset metadata buffer.
        m_metadata_buffer.clear();
        return 0;
    }
    in_prog_size += submission_entries_needed;

    // Append fdatasync operation.
    m_in_progress_batch->add_fdatasync_op_to_batch(file_fd, get_enum_value(uring_op_t::fdatasync), IOSQE_IO_LINK);

    // Signal eventfd's as part of batch.
    m_in_progress_batch->add_pwritev_op_to_batch(&c_default_iov, 1, s_flush_efd, 0, get_enum_value(uring_op_t::pwritev_eventfd_flush), IOSQE_IO_LINK);
    m_in_progress_batch->add_pwritev_op_to_batch(&c_default_iov, 1, s_validate_flush_efd, 0, get_enum_value(uring_op_t::pwritev_eventfd_validate), IOSQE_IO_DRAIN);

    swap_batches();
    auto flushed_batch_size = m_in_flight_batch->get_unsubmitted_entries_count();
    ASSERT_INVARIANT(in_prog_size == flushed_batch_size, "ptr swap failed.");
    size_t submission_count = 0;

    if (sync_submit)
    {
        submission_count = m_in_flight_batch->submit_operation_batch(sync_submit);
        perform_post_completion_maintenance();
    }
    else
    {
        // Issue async submit.
        bool wait_for_io = false;
        submission_count = m_in_flight_batch->submit_operation_batch(wait_for_io);
    }

    if (flushed_batch_size != submission_count)
    {
        throw_error("IOUring submission failure.", -1);
    }

    if (m_in_flight_batch->get_unsubmitted_entries_count() != 0)
    {
        throw_error("IOUring batch size after submission should be 0.", -1);
    }
    return submission_entries_needed;
}

size_t async_disk_writer_t::perform_file_close_operations(int file_fd, file_sequence_t log_seq)
{
    size_t submission_entries_needed = 1;
    submit_if_full(file_fd, submission_entries_needed);

    // Call fdatasync on the file before closing it.
    m_in_progress_batch->add_fdatasync_op_to_batch(file_fd, get_enum_value(uring_op_t::fdatasync), IOSQE_IO_DRAIN);

    // Append file fd to batch so that file can be closed post batch validation.
    m_in_progress_batch->append_file_to_batch(file_fd, log_seq);
    return submission_entries_needed;
}

unsigned char* async_disk_writer_t::copy_into_metadata_buffer(void* source, size_t size, int file_fd)
{
    auto current_ptr = m_metadata_buffer.get_current_ptr();
    ASSERT_PRECONDITION(current_ptr, "Invalid metadata_buffer ptr");
    ASSERT_PRECONDITION(size > 0, "Expected size to copy into metadata buffer must be greater than 0");
    ASSERT_PRECONDITION(source, "Source ptr must be valid.");

    // Call submit synchronously once the metadata buffer is full.
    if (!m_metadata_buffer.has_enough_space(size))
    {
        bool sync_submit = true;
        submit_and_swap_in_progress_batch(file_fd, sync_submit);
        m_metadata_buffer.clear();

        ASSERT_INVARIANT(m_metadata_buffer.current_ptr == m_metadata_buffer.start, "Should receive a new metadata buffer.");
    }

    m_metadata_buffer.allocate(size);
    memcpy(current_ptr, source, size);

    // Return old pointer.
    return current_ptr;
}

size_t async_disk_writer_t::get_total_pwritev_size_in_bytes(const iovec* iov_array, size_t count)
{
    auto ptr = iov_array;

    size_t to_return = 0;
    for (size_t i = 1; i <= count; i++)
    {
        to_return += ptr->iov_len;
        ptr++;
    }
    return to_return;
}

void async_disk_writer_t::enqueue_pwritev_requests(
    std::vector<iovec>& writes_to_submit,
    int file_fd,
    size_t current_log_file_offset,
    uring_op_t type)
{
    ASSERT_PRECONDITION(!writes_to_submit.empty(), "At least enque one write to the io_uring queue");

    auto required_size = sizeof(iovec) * writes_to_submit.size();
    auto current_helper_ptr = copy_into_metadata_buffer(writes_to_submit.data(), required_size, file_fd);

    // Each pwritev call can accomodate __IOV_MAX entries at most.
    auto num_pwrites = writes_to_submit.size() / __IOV_MAX + 1;

    ASSERT_PRECONDITION(num_pwrites > 0, "At least enque one write to the io_uring queue");

    auto remaining_size = required_size;
    auto remaining_count = writes_to_submit.size();

    for (size_t i = 1; i <= num_pwrites; i++)
    {
        auto ptr_to_write = current_helper_ptr;

        if (remaining_count <= __IOV_MAX)
        {
            enqueue_pwritev_request(reinterpret_cast<const iovec*>(ptr_to_write), remaining_count, file_fd, current_log_file_offset, type);
            break;
        }
        else
        {
            enqueue_pwritev_request(reinterpret_cast<const iovec*>(ptr_to_write), __IOV_MAX, file_fd, current_log_file_offset, type);
            current_log_file_offset += get_total_pwritev_size_in_bytes(reinterpret_cast<const iovec*>(current_helper_ptr), __IOV_MAX);
            remaining_size -= __IOV_MAX * sizeof(iovec);
            current_helper_ptr += __IOV_MAX * sizeof(iovec);
            remaining_count -= __IOV_MAX;
        }
    }
}

bool async_disk_writer_t::submit_if_full(int file_fd, size_t required_entries)
{
    bool to_submit = m_in_progress_batch.get()->get_unused_submission_entries_count() - c_submit_batch_sqe_count - required_entries <= 0;
    if (to_submit)
    {
        submit_and_swap_in_progress_batch(file_fd);
    }
    return to_submit;
}

size_t async_disk_writer_t::enqueue_pwritev_request(
    const struct iovec* iovec_array,
    size_t iovcnt,
    int file_fd,
    uint64_t current_offset,
    uring_op_t type)
{
    size_t submission_entries_needed = 1;
    submit_if_full(file_fd, submission_entries_needed);
    m_in_progress_batch->add_pwritev_op_to_batch(iovec_array, iovcnt, file_fd, current_offset, get_enum_value(type), 0);
    return submission_entries_needed;
}

void async_disk_writer_t::swap_batches()
{
    m_in_flight_batch.swap(m_in_progress_batch);
}

} // namespace persistence
} // namespace db
} // namespace gaia
