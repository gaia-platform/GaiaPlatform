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
    m_ring.reset(r);
}

io_uring_wrapper_t::~io_uring_wrapper_t()
{
    teardown();
}

void io_uring_wrapper_t::teardown()
{
    if (m_ring)
    {
        io_uring_queue_exit(m_ring.get());
    }
}

io_uring_sqe* io_uring_wrapper_t::get_sqe()
{
    if (io_uring_sq_space_left(m_ring.get()) <= 0)
    {
        throw io_uring_error(c_buffer_empty_err_msg);
    }
    return io_uring_get_sqe(m_ring.get());
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

void io_uring_wrapper_t::add_pwritev_op_to_batch(
    const iovec* iovecs,
    size_t num_iovecs,
    int file_fd,
    uint64_t current_offset,
    uint64_t data,
    u_char flags)
{
    auto sqe = get_sqe();
    io_uring_prep_writev(sqe, file_fd, iovecs, num_iovecs, current_offset);
    prep_sqe(data, flags, sqe);
}

void io_uring_wrapper_t::add_fsync_op_to_batch(
    int file_fd,
    uint32_t fsync_flags,
    uint64_t data,
    u_char flags)
{
    auto sqe = get_sqe();
    io_uring_prep_fsync(sqe, file_fd, fsync_flags);
    prep_sqe(data, flags, sqe);
}

size_t io_uring_wrapper_t::submit_operation_batch(bool wait)
{
    if (wait)
    {
        auto pending_submissions = io_uring_sq_ready(m_ring.get());
        return io_uring_submit_and_wait(m_ring.get(), pending_submissions);
    }
    else
    {
        // Non blocking submit
        return io_uring_submit(m_ring.get());
    }
}

void io_uring_wrapper_t::close_all_files_in_batch()
{
    {
        auto cleanup = gaia::common::scope_guard::make_scope_guard([&]() {
            for (auto fd : m_file_fds)
            {
                close_fd(fd);
            }
        });
    }

    m_file_fds.clear();
}

void io_uring_wrapper_t::append_file_to_batch(int fd)
{
    m_file_fds.emplace_back(fd);
}

size_t io_uring_wrapper_t::get_unsubmitted_entries_count()
{
    return io_uring_sq_ready(m_ring.get());
}

size_t io_uring_wrapper_t::get_unused_submission_entries_count()
{
    return io_uring_sq_space_left(m_ring.get());
}

int io_uring_wrapper_t::get_completion_event(struct io_uring_cqe** cqe)
{
    return io_uring_peek_cqe(m_ring.get(), cqe);
}

size_t io_uring_wrapper_t::get_completion_count()
{
    return io_uring_cq_ready(m_ring.get());
}

void io_uring_wrapper_t::mark_completion_seen(struct io_uring_cqe* cqe)
{
    io_uring_cqe_seen(m_ring.get(), cqe);
}
