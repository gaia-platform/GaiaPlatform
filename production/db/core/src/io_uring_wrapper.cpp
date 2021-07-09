/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "io_uring_wrapper.hpp"

#include <iostream>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"

#include "liburing.h"

using namespace gaia::db;
using namespace gaia::common;

void io_uring_wrapper_t::open(size_t buffer_size)
{
    auto r = new io_uring();
    int ret = io_uring_queue_init(buffer_size, r, 0);
    if (ret < 0)
    {
        throw_system_error(c_setup_err_msg);
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
    auto sqe = io_uring_get_sqe(m_ring.get());
    ASSERT_INVARIANT(sqe, c_buffer_empty_err_msg);
    return sqe;
}

void io_uring_wrapper_t::prep_sqe(uint64_t data, u_char flags, io_uring_sqe* sqe)
{
    ASSERT_PRECONDITION(sqe, "Submission queue entry cannot be null.");
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

void io_uring_wrapper_t::add_fdatasync_op_to_batch(
    int file_fd,
    uint64_t data,
    u_char flags)
{
    auto sqe = get_sqe();
    io_uring_prep_fsync(sqe, file_fd, IORING_FSYNC_DATASYNC);
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
            for (log_file_info_t file_info : m_files_to_close)
            {
                close_fd(file_info.second);
            }
        });
    }
    m_files_to_close.clear();
}

uint64_t io_uring_wrapper_t::get_max_file_seq_to_close()
{
    if (m_files_to_close.size() > 0)
    {
        return m_files_to_close.back().first;
    }
    return 0;
}

void io_uring_wrapper_t::append_file_to_batch(int fd, uint64_t log_seq)
{
    log_file_info_t info{log_seq, fd};
    m_files_to_close.push_back(info);
}

size_t io_uring_wrapper_t::get_unsubmitted_entries_count()
{
    return io_uring_sq_ready(m_ring.get());
}

size_t io_uring_wrapper_t::get_unused_submission_entries_count()
{
    return io_uring_sq_space_left(m_ring.get());
}

void io_uring_wrapper_t::validate_next_completion_event()
{
    io_uring_cqe* cqe;
    int ret = io_uring_peek_cqe(m_ring.get(), &cqe);
    if (ret != 0)
    {
        throw_system_error("Expected completions to be ready post flush_fd write.", ret);
    }

    // Validate completion result.
    if (cqe->res < 0)
    {
        std::stringstream ss;
        ss << "CQE completion failure for op: " << cqe->user_data;
        throw_system_error(ss.str(), cqe->res);
    }

    // Mark completion as seen.
    mark_completion_seen(cqe);
}

size_t io_uring_wrapper_t::get_completion_count()
{
    return io_uring_cq_ready(m_ring.get());
}

void io_uring_wrapper_t::mark_completion_seen(struct io_uring_cqe* cqe)
{
    io_uring_cqe_seen(m_ring.get(), cqe);
}

void io_uring_wrapper_t::insert_in_decision_batch(decision_entry_t decision)
{
    m_batch_decisions.push_back(decision);
}

const decision_list_t& io_uring_wrapper_t::get_decision_batch_entries() const
{
    return m_batch_decisions;
}

size_t io_uring_wrapper_t::get_decision_batch_size()
{
    return m_batch_decisions.size();
}
void io_uring_wrapper_t::clear_decision_batch()
{
    m_batch_decisions.clear();
}
