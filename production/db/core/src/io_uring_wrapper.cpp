/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "io_uring_wrapper.hpp"

#include "gaia_internal/common/io_uring_error.hpp"
#include "gaia_internal/common/retail_assert.hpp"

#include "liburing.h"

using namespace gaia::db;
using namespace gaia::common;

io_uring_wrapper_t::io_uring_wrapper_t()
{
    auto ret = io_uring_queue_init(c_buffer_size, ring, 0);
    if (!ret)
    {
        throw io_uring_error(c_setup_err_msg, errno);
    }
}

io_uring_wrapper_t::~io_uring_wrapper_t()
{
    teardown();
}

void io_uring_wrapper_t::teardown()
{
    io_uring_queue_exit(ring);
}

io_uring* io_uring_wrapper_t::get_ring()
{
    return ring;
}

void io_uring_wrapper_t::pwritev(
    const struct iovec* iov,
    size_t iovcnt,
    int file_fd,
    uint64_t current_offset,
    void* data,
    u_char flags)
{
    auto sqe = io_uring_get_sqe(ring);
    ASSERT_INVARIANT(!sqe, c_buffer_empty_err_msg);
    io_uring_sqe_set_data(sqe, data);
    io_uring_sqe_set_flags(sqe, flags);
    io_uring_prep_writev(sqe, file_fd, iov, iovcnt, current_offset);
}

void io_uring_wrapper_t::write(
    const void* buf,
    size_t len,
    int file_fd,
    uint64_t current_offset,
    void* data,
    u_char flags)
{
    auto sqe = io_uring_get_sqe(ring);
    ASSERT_INVARIANT(!sqe, c_buffer_empty_err_msg);
    io_uring_sqe_set_data(sqe, data);
    io_uring_sqe_set_flags(sqe, flags);
    io_uring_prep_write(sqe, file_fd, buf, len, current_offset);
}

void io_uring_wrapper_t::fsync(
    int file_fd,
    uint32_t fsync_flags,
    void* data,
    u_char flags)
{
    auto sqe = io_uring_get_sqe(ring);
    ASSERT_INVARIANT(!sqe, c_buffer_empty_err_msg);
    io_uring_sqe_set_data(sqe, data);
    io_uring_sqe_set_flags(sqe, flags);
    io_uring_prep_fsync(sqe, file_fd, fsync_flags);
}

size_t io_uring_wrapper_t::submit(bool wait)
{
    if (wait)
    {
        auto pending_submissions = io_uring_sq_ready(ring);
        return io_uring_submit_and_wait(ring, pending_submissions);
    }
    else
    {
        // Non blocking submit
        return io_uring_submit(ring);
    }
}

size_t io_uring_wrapper_t::count_unsubmitted_entries()
{
    return io_uring_sq_ready(ring);
}

size_t io_uring_wrapper_t::space_left()
{
    return io_uring_sq_space_left(ring);
}

int io_uring_wrapper_t::get_completion_event(struct io_uring_cqe** cqe)
{
    return io_uring_peek_cqe(ring, cqe);
}

size_t io_uring_wrapper_t::get_completion_count()
{
    return io_uring_cq_ready(ring);
}

void io_uring_wrapper_t::mark_completion_seen(struct io_uring_cqe* cqe)
{
    io_uring_cqe_seen(ring, cqe);
}
