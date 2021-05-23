/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "io_uring_manager.hpp"

#include <iostream>
#include <memory>

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

io_uring_manager_t::io_uring_manager_t()
{
    in_progress_buffer = std::make_unique<io_uring_wrapper_t>();
    in_flight_buffer = std::make_unique<io_uring_wrapper_t>();

    flush_efd = eventfd(1, 0);
    if (flush_efd == -1)
    {
        teardown();
    }
}

void io_uring_manager_t::open(size_t buffer_size)
{
    in_progress_buffer->open(buffer_size);
    in_flight_buffer->open(buffer_size);
}

io_uring_manager_t::~io_uring_manager_t()
{
    teardown();
}

void io_uring_manager_t::teardown()
{
    close_fd(flush_efd);
}

uint64_t get_enum_value(uring_op_t op)
{
    return static_cast<std::underlying_type<uring_op_t>::type>(op);
}

void io_uring_manager_t::handle_io_uring_error(bool condition, std::string err_msg, int err)
{
    if (!condition)
    {
        teardown();
        std::stringstream ss;
        ss << err_msg << "; with operation return code=" << err << "; with errno=" << errno;
        throw io_uring_error(ss.str());
    }
}

size_t io_uring_manager_t::get_submission_batch_size(bool in_progress)
{
    return in_progress ? in_progress_buffer->space_left() : in_flight_buffer->space_left();
}

size_t io_uring_manager_t::get_completion_batch_size(bool in_progress)
{
    return in_progress ? in_progress_buffer->get_completion_count() : in_flight_buffer->get_completion_count();
}

int io_uring_manager_t::get_flush_efd()
{
    return flush_efd;
}

// This function is only called post batch write completion.
void io_uring_manager_t::validate_completion_batch()
{
    auto max_size = in_flight_buffer->get_completion_count();
    for (size_t i = 0; i < max_size; i++)
    {
        io_uring_cqe* cqe;
        auto ret = in_flight_buffer->get_completion_event(&cqe);
        handle_io_uring_error(ret == 0, "Expected completions to be ready post flush_fd write.", ret);
        std::cout << "user data = " << cqe->user_data << std::endl;
        std::cout << "user result = " << cqe->res << std::endl;
        std::cout << "str error = " << strerror(-cqe->res) << std::endl;
        // Validate completion result.
        // Todo (mihir): Set user data for submission entries so they can be used to create meaningful error messages.
        handle_io_uring_error(cqe->res >= 0, "CQE completion failure from in_flight batch.", cqe->res);

        // Mark completion as seen.
        in_flight_buffer->mark_completion_seen(cqe);
    }

    // Post validation, clear the helper buffer.
    in_flight_buffer->buffer.clear();
    in_flight_buffer->close_all_files_in_batch();
}

size_t io_uring_manager_t::handle_submit(int file_fd, bool wait)
{
    size_t submission_entries_needed = 2;
    eventfd_t event_counter;

    // The read call will block on a zero value of the event_counter.
    // The flush_efd's counter gets updated when flush of the in_flight batch completes.
    // The initial value of the event counter is set to 1 so we don't block the very first
    // batch flush.
    eventfd_read(flush_efd, &event_counter); // Should this be non-blocking? Do we want to keep validating in-flight-batch?

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
    validate_completion_batch();

    // Finish up batch before swapping & flushing it. We swap batches since writes can only
    // be made to the in-progress batch.
    // Usually submission queue entries (sqes) are used independently, meaning that the execution of one does not affect
    // the execution or ordering of subsequent sqe entries in the ring. io_uring supports draining the
    // submission side queue until all previous writes have finished. The queued writes will complete
    // in parallel and out of order, but the fsync operation will only begin after all prior writes in the
    // queue are done. This is accomplished by the IOSQE_IO_DRAIN flag.
    // Additionally we supply a IOSQE_IO_LINK flag, so that the next operation in the queue is dependent on
    // the fsync operation.
    size_t in_prog_size = in_progress_buffer.get()->count_unsubmitted_entries() + 2;

    iovec iov;
    iov.iov_base = (void*)&default_flush_efd_value;
    iov.iov_len = sizeof(eventfd_t);

    auto ptr_to_use = (iovec*)copy_into_helper_buffer((void*)&iov, sizeof(iovec), file_fd);

    in_progress_buffer->fsync(file_fd, IORING_FSYNC_DATASYNC, get_enum_value(uring_op_t::FSYNC), IOSQE_IO_DRAIN | IOSQE_IO_LINK);
    in_progress_buffer->pwritev(ptr_to_use, 1, flush_efd, 0, get_enum_value(uring_op_t::PWRITEV), 0);

    swap_buffers();
    auto flushed_batch_size = in_flight_buffer->count_unsubmitted_entries();
    ASSERT_INVARIANT(in_prog_size == flushed_batch_size, "ptr swap failed.");
    size_t submission_count = 0;

    if (wait)
    {
        submission_count = in_flight_buffer->submit(wait);
        validate_completion_batch();
    }
    else
    {
        // Issue async submit.
        bool wait_for_io = false;
        submission_count = in_flight_buffer->submit(wait_for_io);
    }
    handle_io_uring_error(flushed_batch_size == submission_count, "IOUring submission failure.", 0);
    handle_io_uring_error(in_flight_buffer->count_unsubmitted_entries() == 0, "IOUring batch size after submission should be 0.", 0);
    return submission_entries_needed;
}

// Call fsync on the file before closing it.
// Close the fd separately.
size_t io_uring_manager_t::handle_file_close(int fd)
{
    size_t submission_entries_needed = 1;
    submit_if_full(fd, submission_entries_needed);
    in_progress_buffer->fsync(fd, IORING_FSYNC_DATASYNC, get_enum_value(uring_op_t::FSYNC), IOSQE_IO_DRAIN);
    in_progress_buffer->append_file_to_batch(fd);
    return submission_entries_needed;
}

uint8_t* io_uring_manager_t::copy_into_helper_buffer(void* source, size_t size, int file_fd)
{
    auto current_ptr = in_progress_buffer->buffer.get_current_ptr();

    // We don't expect helper buffer to run out of space, but if it does then
    // submit it.
    if (!in_progress_buffer->buffer.has_enough_space(size))
    {
        handle_submit(file_fd);

        // Batches have been swapped. Ensure new helper buffer for batch is empty.
        ASSERT_INVARIANT(in_progress_buffer->buffer.current_ptr == in_progress_buffer->buffer.start, "IOUring buffers should have been swapped.");
    }

    in_progress_buffer->buffer.allocate(size);
    memcpy(current_ptr, source, size);

    // Return old pointer.
    return current_ptr;
}

size_t calculate_total_pwritev_size(const iovec* start, size_t count)
{
    auto ptr = start;

    size_t to_return = 0;
    for (size_t i = 0; i < count; i++)
    {
        to_return += ptr->iov_len;
        ptr++;
    }
    return to_return;
}

void io_uring_manager_t::construct_pwritev(
    std::vector<iovec>& writes_to_submit,
    int file_fd,
    uint64_t current_log_file_offset)
{
    auto required_size = sizeof(iovec) * writes_to_submit.size();
    auto current_helper_ptr = copy_into_helper_buffer(writes_to_submit.data(), required_size, file_fd);

    // Construct pwrites.
    auto num_pwrites = writes_to_submit.size() / __IOV_MAX + 1;

    ASSERT_INVARIANT(num_pwrites > 0, "At least enque one write to the io_uring queue");

    auto remaining_size = required_size;
    auto remaining_count = writes_to_submit.size();

    for (size_t i = 1; i <= num_pwrites; i++)
    {
        auto ptr_to_write = current_helper_ptr;

        if (remaining_size <= __IOV_MAX * sizeof(iovec))
        {
            pwritev((const iovec*)ptr_to_write, remaining_size, file_fd, current_log_file_offset);
            current_log_file_offset += calculate_total_pwritev_size((const iovec*)current_helper_ptr, remaining_count);
            current_helper_ptr += remaining_size;
            ASSERT_INVARIANT(i == num_pwrites, "");
        }
        else
        {
            pwritev((const iovec*)ptr_to_write, __IOV_MAX, file_fd, current_log_file_offset);
            current_log_file_offset += calculate_total_pwritev_size((const iovec*)current_helper_ptr, __IOV_MAX);
            remaining_size -= __IOV_MAX * sizeof(iovec);
            current_helper_ptr += __IOV_MAX * sizeof(iovec);
            remaining_count -= __IOV_MAX;
        }
    }
}

bool io_uring_manager_t::submit_if_full(int file_fd, size_t required_entries)
{
    bool res = in_progress_buffer.get()->space_left() - submit_batch_sqe_count - required_entries <= 0;

    if (res)
    {
        // This call will block if flushing the in_flight batch is still in progress.
        // This generates back pressure on clients as they will get blocked waiting on persistence
        // to make progress.
        handle_submit(file_fd);
    }
    return res;
}

// API assumes that there is enough space remaining in file.
size_t io_uring_manager_t::pwritev(
    const struct iovec* iovecs,
    size_t iovcnt,
    int file_fd,
    uint64_t current_offset)
{
    size_t submission_entries_needed = 1;
    submit_if_full(file_fd, submission_entries_needed);
    std::cout << "current offset = " << current_offset << std::endl;
    in_progress_buffer.get()->pwritev(iovecs, iovcnt, file_fd, current_offset, get_enum_value(uring_op_t::PWRITEV), 0);
    return submission_entries_needed;
}

void io_uring_manager_t::swap_buffers()
{
    in_flight_buffer.swap(in_progress_buffer);
}

} // namespace db
} // namespace gaia
