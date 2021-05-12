/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "io_uring_manager.hpp"

#include <memory>

#include <sys/eventfd.h>

#include "gaia_internal/common/fd_helpers.hpp"
#include "gaia_internal/common/retail_assert.hpp"

#include "io_uring_wrapper.hpp"
#include "liburing.h"

using namespace gaia::db;
using namespace gaia::common;

namespace gaia
{
namespace db
{

io_uring_manager_t::io_uring_manager_t()
{
    in_progress_buffer.reset();
    in_progress_buffer = std::make_unique<io_uring_wrapper_t>();
    in_flight_buffer.reset();
    in_flight_buffer = std::make_unique<io_uring_wrapper_t>();

    flush_efd = eventfd(1, 0);
    if (flush_efd == -1)
    {
        teardown();
    }
}

io_uring_manager_t::~io_uring_manager_t()
{
    in_progress_buffer.reset();
    in_flight_buffer.reset();
    teardown();
}

void io_uring_manager_t::teardown()
{
    close_fd(flush_efd);
}

void io_uring_manager_t::handle_io_uring_error(int ret, std::string err_msg)
{
    if (!ret)
    {
        teardown();
        throw io_uring_error(err_msg, -1);
    }
}

// This function is only called post batch write completion.
void io_uring_manager_t::validate_completion_batch()
{
    for (size_t i = 0; i < in_flight_buffer->get_completion_count(); i++)
    {
        io_uring_cqe* cqe;
        auto ret = in_flight_buffer->get_completion_event(&cqe);
        handle_io_uring_error(ret == 0, "Expected completions to be ready post flush_fd write.");

        // Validate completion result.
        // Todo (mihir): Set user data for submission entries so they can be used to create meaningful error messages.
        handle_io_uring_error(cqe->res >= 0, "CQE completion failure from in_flight batch.");

        // Mark completion as seen.
        in_flight_buffer->mark_completion_seen(cqe);
    }
}

void io_uring_manager_t::handle_submit(int file_fd, bool wait = false)
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
    size_t in_prog_size = in_progress_buffer.get()->count_unsubmitted_entries();
    in_progress_buffer.get()->fsync(file_fd, IORING_FSYNC_DATASYNC, nullptr, IOSQE_IO_DRAIN | IOSQE_IO_LINK);
    in_progress_buffer.get()->write(&in_prog_size, sizeof(eventfd_t), flush_efd, 0, nullptr, 0);
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
    handle_io_uring_error(flushed_batch_size == submission_count, "IOUring submission failure.");
    handle_io_uring_error(in_flight_buffer->count_unsubmitted_entries() == 0, "IOUring batch size after submission should be 0.");
}

void io_uring_manager_t::close_fd(int fd)
{
    in_progress_buffer->close(fd);
}

void io_uring_manager_t::pwritev(
    const struct iovec* iovecs,
    size_t iovcnt,
    int file_fd,
    uint64_t current_offset,
    bool submit)
{
    // Call handle_submit() either when the submit flag is set or when the in-progress buffer gets full.
    if (in_progress_buffer.get()->space_left() - submit_batch_sqe_count == 0 || submit)
    {
        // This call will block if flushing the in_flight batch is still in progress.
        // This generates back pressure on clients as they will get blocked waiting on persistence
        // to make progress.
        handle_submit(file_fd);

        // Write to the in-progress batch.
        in_progress_buffer.get()->pwritev(iovecs, iovcnt, file_fd, current_offset, nullptr, 0);
    }
    else
    {
        // Add to the inprogress batch
        in_progress_buffer.get()->pwritev(iovecs, iovcnt, file_fd, current_offset, nullptr, 0);

        if (in_progress_buffer.get()->space_left() - submit_batch_sqe_count == 0 || submit)
        {
            handle_submit(file_fd);
        }
    }
}

void io_uring_manager_t::swap_buffers()
{
    in_flight_buffer.swap(in_progress_buffer);
}

} // namespace db
} // namespace gaia
