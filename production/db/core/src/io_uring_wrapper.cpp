/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "io_uring_wrapper.hpp"

#include <iostream>

#include "gaia_internal/common/io_uring_error.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"

#include "liburing.h"

using namespace gaia::db;
using namespace gaia::common;

void io_uring_wrapper_t::open(size_t buffer_size)
{
    auto r = new io_uring();
    auto ret = io_uring_queue_init(buffer_size, r, 0);
    if (ret < 0)
    {
        throw io_uring_error(c_setup_err_msg, errno);
    }
    ring.reset(r);
}

io_uring_wrapper_t::~io_uring_wrapper_t()
{
    teardown();
}

void io_uring_wrapper_t::teardown()
{
    if (ring)
    {
        io_uring_queue_exit(ring.get());
    }
}

io_uring* io_uring_wrapper_t::get_ring()
{
    return ring.get();
}

io_uring_sqe* io_uring_wrapper_t::get_sqe()
{
    if (io_uring_sq_space_left(ring.get()) <= 0)
    {
        throw io_uring_error(c_buffer_empty_err_msg);
    }
    return io_uring_get_sqe(ring.get());
}

void io_uring_wrapper_t::prep_sqe(uint64_t data, u_char flags, io_uring_sqe* sqe)
{
    if (!sqe)
    {
        throw io_uring_error(c_buffer_empty_err_msg);
    }
    sqe->user_data = data;
    sqe->flags |= flags;
}

void io_uring_wrapper_t::append_pwritev(
    const struct iovec* iov,
    size_t iovcnt,
    int file_fd,
    uint64_t current_offset,
    uint64_t data,
    u_char flags)
{
    auto sqe = get_sqe();
    io_uring_prep_writev(sqe, file_fd, iov, iovcnt, current_offset);
    prep_sqe(data, flags, sqe);
}

void io_uring_wrapper_t::append_fsync(
    int file_fd,
    uint32_t fsync_flags,
    uint64_t data,
    u_char flags)
{
    auto sqe = get_sqe();
    io_uring_prep_fsync(sqe, file_fd, fsync_flags);
    prep_sqe(data, flags, sqe);
}

size_t io_uring_wrapper_t::submit(bool wait)
{
    if (wait)
    {
        auto pending_submissions = io_uring_sq_ready(ring.get());
        return io_uring_submit_and_wait(ring.get(), pending_submissions);
    }
    else
    {
        // Non blocking submit
        return io_uring_submit(ring.get());
    }
}

void io_uring_wrapper_t::close_all_files_in_batch()
{
    for (auto entry : file_fds)
    {
        int fd = entry.first;
        auto cleanup = gaia::common::scope_guard::make_scope_guard([&]() { close_fd(fd); });
        ASSERT_INVARIANT(entry.second > 0, "Persistent log file size has to be greater than zero.");
        common::truncate_fd(fd, entry.second);

        // Fync so that fstat returns correct file size.
        int res = fsync(fd);
        if (res < 0)
        {
            throw_system_error("Fsync on persistent log file failed when closing files in batch.", errno);
        }
    }

    file_fds.clear();
}

void io_uring_wrapper_t::append_file_to_batch(int fd, size_t file_size)
{
    file_fds.emplace_back(std::pair(fd, file_size));
}

size_t io_uring_wrapper_t::count_unsubmitted_entries()
{
    return io_uring_sq_ready(ring.get());
}

size_t io_uring_wrapper_t::space_left()
{
    return io_uring_sq_space_left(ring.get());
}

int io_uring_wrapper_t::get_completion_event(struct io_uring_cqe** cqe)
{
    return io_uring_peek_cqe(ring.get(), cqe);
}

size_t io_uring_wrapper_t::get_completion_count()
{
    return io_uring_cq_ready(ring.get());
}

void io_uring_wrapper_t::mark_completion_seen(struct io_uring_cqe* cqe)
{
    io_uring_cqe_seen(ring.get(), cqe);
}
