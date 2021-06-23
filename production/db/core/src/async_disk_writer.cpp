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

#include "io_uring_wrapper.hpp"
#include "liburing.h"
#include "persistence_types.hpp"

using namespace gaia::db;
using namespace gaia::common;

namespace gaia
{
namespace db
{

async_disk_writer_t::async_disk_writer_t(int validate_flush_efd0)
{
    in_progress_buffer = std::make_unique<io_uring_wrapper_t>();
    in_flight_buffer = std::make_unique<io_uring_wrapper_t>();
    ASSERT_INVARIANT(validate_flush_efd0 >= 0, "Invalid validate flush eventfd");

    validate_flush_efd = validate_flush_efd0;

    // Used to block new writes to disk when a batch is already getting flushed.
    flush_efd = eventfd(1, 0);
    if (flush_efd == -1)
    {
        teardown();
    }
}

void async_disk_writer_t::open(size_t buffer_size)
{
    in_progress_buffer->open(buffer_size);
    in_flight_buffer->open(buffer_size);
}

async_disk_writer_t::~async_disk_writer_t()
{
    teardown();
}

void async_disk_writer_t::teardown()
{
    close_fd(flush_efd);
}

uint64_t get_enum_value(uring_op_t op)
{
    return static_cast<std::underlying_type<uring_op_t>::type>(op);
}

void async_disk_writer_t::throw_io_uring_error(std::string err_msg, int err, uint64_t cqe_data)
{
    teardown();
    std::stringstream ss;
    ss << err_msg << "; with operation return code = " << err;
    if (cqe_data != 0)
    {
        ss << "; with cqe data = " << cqe_data;
    }
    throw io_uring_error(ss.str());
}

void async_disk_writer_t::map_commit_ts_to_session_unblock_fd(gaia_txn_id_t commit_ts, int session_unblock_fd)
{
    ts_to_session_unblock_fd_map.insert(std::pair(commit_ts, session_unblock_fd));
}

void async_disk_writer_t::add_decisions_to_batch(decision_list_t& decisions)
{
    for (const auto& decision : decisions)
    {
        in_progress_buffer->batch_decisions.push_back(decision);
    }
}

size_t async_disk_writer_t::get_submission_batch_size(bool in_progress)
{
    return in_progress ? in_progress_buffer->space_left() : in_flight_buffer->space_left();
}

size_t async_disk_writer_t::get_completion_batch_size(bool in_progress)
{
    return in_progress ? in_progress_buffer->get_completion_count() : in_flight_buffer->get_completion_count();
}

int async_disk_writer_t::get_flush_efd()
{
    return flush_efd;
}

void async_disk_writer_t::register_txn_durable_fn(std::function<void(gaia_txn_id_t)> txn_durable_fn)
{
    s_txn_durable_fn = txn_durable_fn;
}

// This function is only called post batch write completion.
void async_disk_writer_t::perform_post_completion_maintenence()
{
    std::cout << "calling batch validation" << std::endl;
    // Validate most recent async write batch.
    auto size_to_validate = in_flight_buffer->get_completion_count();
    for (size_t i = 0; i < size_to_validate; i++)
    {
        io_uring_cqe* cqe;
        auto ret = in_flight_buffer->get_completion_event(&cqe);

        if (ret != 0)
        {
            throw_io_uring_error("Expected completions to be ready post flush_fd write.", ret);
        }

        // Validate completion result.
        if (cqe->res < 0)
        {
            throw_io_uring_error("CQE completion failure from in_flight batch.", cqe->res, cqe->user_data);
        }

        // Mark completion as seen.
        in_flight_buffer->mark_completion_seen(cqe);
    }

    // Post validation, clear the helper buffer.
    in_flight_buffer->close_all_files_in_batch();

    // Set durability flags.
    std::cout << "validating batch decision size = " << in_flight_buffer->batch_decisions.size() << std::endl;
    for (auto entry : in_flight_buffer->batch_decisions)
    {
        s_txn_durable_fn(entry.first);
        auto itr = ts_to_session_unblock_fd_map.find(entry.first);
        ASSERT_INVARIANT(itr != ts_to_session_unblock_fd_map.end(), "Unable to find fd of session to wakeup post log write.");

        // Obtain session_unblock_efd to be able to signal it.
        // Signal to session threads so they can make progress.
        std::cout << " Write SESSION unblock efd = " << itr->second << std::endl;
        signal_eventfd(itr->second, 1);
        ts_to_session_unblock_fd_map.erase(itr);
    }

    in_flight_buffer->batch_decisions.clear();
}

size_t async_disk_writer_t::handle_submit(int file_fd, bool wait)
{
    eventfd_t event_counter;

    // The read call will block on a zero value of the event_counter.
    // The flush_efd's counter gets updated when flush of the in_flight batch completes.
    // The initial value of the event counter is set to 1 so we don't block the very first
    // batch flush.
    eventfd_read(flush_efd, &event_counter);

    // Flush of in-flight batch has completed.
    // Validate events from the previous in-flight batch if they exist.
    // The completion queue should have as many events as the flushed_batch_size. The method
    // throws an error otherwise.

    // TODO(mihir): Note that validation of the flushed batch completions needs to happen ASAP and not wait for the
    // next time handle_submit() is called (as is happening in this code path);
    // The session threads can't proceed until this validation of completions pertaining to the in-flight buffer
    /// has finished.
    // Option 1: Reserve a separate thread to do a blocking wait and validation of results.
    // Option 2: One of the blocked session threads can do this check.
    // Option 3: The persistence thread will wake up on in-flight flush completions via epoll_wait()
    // and validate completions.
    // Proposal: Option3
    perform_post_completion_maintenence();
    return finish_and_submit_batch(file_fd, wait);
}

size_t async_disk_writer_t::finish_and_submit_batch(int file_fd, bool wait)
{
    size_t submission_entries_needed = submit_batch_sqe_count;

    // Finish up batch before swapping & flushing it. We swap batches since writes can only
    // be made to the in-progress batch.
    // Usually submission queue entries (sqes) are used independently, meaning that the execution of one does not affect
    // the execution or ordering of subsequent sqe entries in the ring. io_uring supports draining the
    // submission side queue until all previous writes have finished. The queued writes will complete
    // in parallel and out of order, but the fsync operation will only begin after all prior writes in the
    // queue are done. This is accomplished by the IOSQE_IO_DRAIN flag.
    // Additionally we supply a IOSQE_IO_LINK flag, so that the next operation in the queue is dependent on
    // the fsync operation.
    size_t in_prog_size = in_progress_buffer->count_unsubmitted_entries();
    std::cout << "IN PROG SIZE DURING SUBMIT = " << in_prog_size << std::endl;
    if (in_prog_size == 0)
    {
        // Nothing to submit; reset the flush efd that got burnt.
        std::cout << "NOTHING TO SUBMIT - IN PROG DECISIONS = " << in_progress_buffer->batch_decisions.size() << std::endl;
        swap_buffers();
        signal_eventfd(flush_efd, 1);
        // signal_eventfd(validate_flush_efd, 1);

        // No more writes to submit; reset metadata buffer.
        metadata_buffer.clear();
        return 0;
    }
    in_prog_size += submission_entries_needed;

    in_progress_buffer->append_fsync(file_fd, IORING_FSYNC_DATASYNC, get_enum_value(uring_op_t::FSYNC), IOSQE_IO_LINK);
    in_progress_buffer->append_pwritev(&default_iov, 1, flush_efd, 0, get_enum_value(uring_op_t::PWRITEV_EVENTFD_FLUSH), IOSQE_IO_LINK);
    in_progress_buffer->append_pwritev(&default_iov, 1, validate_flush_efd, 0, get_enum_value(uring_op_t::PWRITEV_EVENTFD_VALIDATE), IOSQE_IO_DRAIN);

    swap_buffers();
    auto flushed_batch_size = in_flight_buffer->count_unsubmitted_entries();
    ASSERT_INVARIANT(in_prog_size == flushed_batch_size, "ptr swap failed.");
    size_t submission_count = 0;

    if (wait)
    {
        submission_count = in_flight_buffer->submit(wait);
        perform_post_completion_maintenence();
    }
    else
    {
        // Issue async submit.
        bool wait_for_io = false;
        submission_count = in_flight_buffer->submit(wait_for_io);
    }

    if (flushed_batch_size != submission_count)
    {
        throw_io_uring_error("IOUring submission failure.", -1);
    }

    if (in_flight_buffer->count_unsubmitted_entries() != 0)
    {
        throw_io_uring_error("IOUring batch size after submission should be 0.", -1);
    }
    return submission_entries_needed;
}

// Call fsync on the file before closing it.
// Close the fd separately.
size_t async_disk_writer_t::handle_file_close(int fd, size_t file_size)
{
    size_t submission_entries_needed = 1;
    submit_if_full(fd, submission_entries_needed);
    in_progress_buffer->append_fsync(fd, IORING_FSYNC_DATASYNC, get_enum_value(uring_op_t::FSYNC), IOSQE_IO_DRAIN);
    in_progress_buffer->append_file_to_batch(fd, file_size);
    return submission_entries_needed;
}

uint8_t* async_disk_writer_t::copy_into_metadata_buffer(void* source, size_t size, int file_fd)
{
    auto current_ptr = metadata_buffer.get_current_ptr();
    ASSERT_PRECONDITION(current_ptr, "Invalid metadata_buffer ptr");
    ASSERT_PRECONDITION(size > 0, "Expected size to copy into metadata buffer must be greater than 0");
    ASSERT_PRECONDITION(source, "Source ptr must be valid.");

    // Call submit synchronously once the metadata buffer is full.
    if (!metadata_buffer.has_enough_space(size))
    {
        std::cout << "helper buffer ran out of space." << std::endl;
        bool sync_submit = true;
        handle_submit(file_fd, sync_submit);
        metadata_buffer.clear();

        // Batches have been swapped. Ensure new helper buffer for batch is empty.
        ASSERT_INVARIANT(metadata_buffer.current_ptr == metadata_buffer.start, "Should receive a new metadata buffer.");
    }

    metadata_buffer.allocate(size);
    memcpy(current_ptr, source, size);

    // Return old pointer.
    return current_ptr;
}

size_t async_disk_writer_t::calculate_total_pwritev_size(const iovec* start, size_t count)
{
    auto ptr = start;

    size_t to_return = 0;
    std::cout << "IOVEC count = " << count << std::endl;
    for (size_t i = 1; i <= count; i++)
    {
        to_return += ptr->iov_len;
        ptr++;
        std::cout << "Iter = " << i << std::endl;
    }
    return to_return;
}

void async_disk_writer_t::construct_pwritev(
    std::vector<iovec>& writes_to_submit,
    int file_fd,
    persistent_log_file_offset_t current_log_file_offset,
    persistent_log_file_offset_t expected_final_offset,
    uring_op_t type)
{
    auto required_size = sizeof(iovec) * writes_to_submit.size();
    std::cout << "CONSTRUCT PWRITEV" << std::endl;
    auto current_helper_ptr = copy_into_metadata_buffer(writes_to_submit.data(), required_size, file_fd);

    // Construct pwrites.
    auto num_pwrites = writes_to_submit.size() / __IOV_MAX + 1;

    ASSERT_PRECONDITION(num_pwrites > 0, "At least enque one write to the io_uring queue");

    auto remaining_size = required_size;
    auto remaining_count = writes_to_submit.size();

    for (size_t i = 1; i <= num_pwrites; i++)
    {
        auto ptr_to_write = current_helper_ptr;

        if (remaining_count <= __IOV_MAX)
        {
            pwritev((const iovec*)ptr_to_write, remaining_count, file_fd, current_log_file_offset, type);
            current_log_file_offset += calculate_total_pwritev_size((const iovec*)current_helper_ptr, remaining_count);
            ASSERT_INVARIANT(i == num_pwrites, "");
            break;
        }
        else
        {
            pwritev((const iovec*)ptr_to_write, __IOV_MAX, file_fd, current_log_file_offset, type);
            current_log_file_offset += calculate_total_pwritev_size((const iovec*)current_helper_ptr, __IOV_MAX);
            remaining_size -= __IOV_MAX * sizeof(iovec);
            current_helper_ptr += __IOV_MAX * sizeof(iovec);
            remaining_count -= __IOV_MAX;
        }
    }

    ASSERT_POSTCONDITION(expected_final_offset == current_log_file_offset, "Offset mismatch.");
}

bool async_disk_writer_t::submit_if_full(int file_fd, size_t required_entries)
{
    bool to_submit = in_progress_buffer.get()->space_left() - submit_batch_sqe_count - required_entries <= 0;
    if (to_submit)
    {
        // This call will block if flushing the in_flight batch is still in progress.
        // This generates back pressure on clients as they will get blocked waiting on persistence
        // to make progress.
        handle_submit(file_fd);
    }
    return to_submit;
}

// API assumes that there is enough space remaining in file.
size_t async_disk_writer_t::pwritev(
    const struct iovec* iovecs,
    size_t iovcnt,
    int file_fd,
    uint64_t current_offset,
    uring_op_t type)
{
    size_t submission_entries_needed = 1;
    submit_if_full(file_fd, submission_entries_needed);
    in_progress_buffer->append_pwritev(iovecs, iovcnt, file_fd, current_offset, get_enum_value(type), 0);
    return submission_entries_needed;
}

void async_disk_writer_t::swap_buffers()
{
    in_flight_buffer.swap(in_progress_buffer);
}

} // namespace db
} // namespace gaia
