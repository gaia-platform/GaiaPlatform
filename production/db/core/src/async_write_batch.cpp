/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "async_write_batch.hpp"

#include <iostream>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"

#include "liburing.h"

using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace persistence
{

void async_write_batch_t::setup(size_t buffer_size)
{
    m_ring = io_uring{};
    int ret = io_uring_queue_init(buffer_size, &m_ring, 0);
    if (ret < 0)
    {
        throw_system_error(c_setup_err_msg);
    }
}

async_write_batch_t::~async_write_batch_t()
{
    teardown();
}

void async_write_batch_t::teardown()
{
    if (m_ring.ring_fd > 0)
    {
        io_uring_queue_exit(&m_ring);
    }
}

io_uring_sqe* async_write_batch_t::get_submission_queue_entry()
{
    auto sqe = io_uring_get_sqe(&m_ring);
    ASSERT_POSTCONDITION(sqe, c_get_sqe_failure_err_msg);
    return sqe;
}

void async_write_batch_t::prepare_submission_queue_entry(uint64_t data, u_char flags, io_uring_sqe* sqe)
{
    ASSERT_PRECONDITION(sqe, "Submission queue entry cannot be null.");
    sqe->user_data = data;
    sqe->flags |= flags;
}

void async_write_batch_t::add_pwritev_op_to_batch(
    const iovec* iovecs,
    size_t num_iovecs,
    int file_fd,
    uint64_t current_offset,
    uint64_t data,
    uint8_t flags)
{
    auto sqe = get_submission_queue_entry();
    io_uring_prep_writev(sqe, file_fd, iovecs, num_iovecs, current_offset);
    prepare_submission_queue_entry(data, flags, sqe);
}

void async_write_batch_t::add_fdatasync_op_to_batch(
    int file_fd,
    uint64_t data,
    uint8_t flags)
{
    auto sqe = get_submission_queue_entry();
    io_uring_prep_fsync(sqe, file_fd, IORING_FSYNC_DATASYNC);
    prepare_submission_queue_entry(data, flags, sqe);
}

size_t async_write_batch_t::submit_operation_batch(bool wait)
{
    if (wait)
    {
        auto pending_submission_count = io_uring_sq_ready(&m_ring);
        return io_uring_submit_and_wait(&m_ring, pending_submission_count);
    }
    else
    {
        // Non blocking submit
        return io_uring_submit(&m_ring);
    }
}

void async_write_batch_t::close_all_files_in_batch()
{
    for (log_file_info_t file_info : m_files_to_close)
    {
        close_fd(file_info.file_fd);
    }

    m_files_to_close.clear();
}

void async_write_batch_t::append_file_to_batch(int fd, file_sequence_t log_seq)
{
    log_file_info_t info{log_seq, fd};
    m_files_to_close.push_back(info);
}

file_sequence_t async_write_batch_t::get_max_file_seq_to_close()
{
    if (m_files_to_close.size() > 0)
    {
        return m_files_to_close.back().sequence;
    }
    return 0;
}

size_t async_write_batch_t::get_unsubmitted_entries_count()
{
    return io_uring_sq_ready(&m_ring);
}

size_t async_write_batch_t::get_unused_submission_entries_count()
{
    return io_uring_sq_space_left(&m_ring);
}

void async_write_batch_t::validate_next_completion_event()
{
    io_uring_cqe* cqe;
    int ret = io_uring_peek_cqe(&m_ring, &cqe);
    ASSERT_INVARIANT(ret == 0, "Expected completion queue entry to be available!");

    // Validate completion result.
    if (cqe->res < 0)
    {
        std::stringstream ss;
        ss << "CQE completion failure for op: " << cqe->user_data;
        throw_system_error(ss.str(), -cqe->res);
    }

    // Mark completion as seen.
    mark_completion_seen(cqe);
}

size_t async_write_batch_t::get_completion_count()
{
    return io_uring_cq_ready(&m_ring);
}

void async_write_batch_t::mark_completion_seen(struct io_uring_cqe* cqe)
{
    io_uring_cqe_seen(&m_ring, cqe);
}

void async_write_batch_t::add_decision_to_batch(decision_entry_t decision)
{
    m_batch_decisions.push_back(decision);
}

const decision_list_t& async_write_batch_t::get_decision_batch_entries() const
{
    return m_batch_decisions;
}

size_t async_write_batch_t::get_decision_batch_size()
{
    return m_batch_decisions.size();
}

void async_write_batch_t::clear_decision_batch()
{
    m_batch_decisions.clear();
}

} // namespace persistence
} // namespace db
} // namespace gaia
