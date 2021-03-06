/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

static constexpr char c_message_uninitialized_timestamp[] = "Uninitialized timestamp!";
static constexpr char c_message_not_a_begin_timestamp[] = "Not a begin timestamp!";
static constexpr char c_message_not_a_commit_timestamp[] = "Not a commit timestamp!";

void txn_metadata_t::check_ts_size(gaia_txn_id_t ts)
{
    common::retail_assert(ts < (1ULL << c_txn_ts_bits), "Timestamp values must fit in 42 bits!");
}

bool txn_metadata_t::is_uninitialized_ts(gaia_txn_id_t ts)
{
    check_ts_size(ts);

    txn_metadata_t txn_metadata = s_txn_metadata_map[ts];
    return txn_metadata.is_uninitialized();
}

// bool txn_metadata_t::is_sealed_ts(gaia_txn_id_t ts)
// {
//     txn_metadata_t txn_metadata = s_txn_metadata_map[ts];
//     return txn_metadata.is_sealed();
// }

bool txn_metadata_t::is_begin_ts(gaia_txn_id_t ts)
{
    common::retail_assert(!is_uninitialized_ts(ts), c_message_uninitialized_timestamp);

    txn_metadata_t txn_metadata = s_txn_metadata_map[ts];
    return txn_metadata.is_begin_ts();
}

bool txn_metadata_t::is_commit_ts(gaia_txn_id_t ts)
{
    common::retail_assert(!is_uninitialized_ts(ts), c_message_uninitialized_timestamp);

    txn_metadata_t txn_metadata = s_txn_metadata_map[ts];
    return txn_metadata.is_commit_ts();
}

bool txn_metadata_t::is_txn_submitted(gaia_txn_id_t begin_ts)
{
    common::retail_assert(is_begin_ts(begin_ts), c_message_not_a_begin_timestamp);

    txn_metadata_t begin_ts_metadata = s_txn_metadata_map[begin_ts];
    return begin_ts_metadata.is_submitted();
}

bool txn_metadata_t::is_txn_validating(gaia_txn_id_t commit_ts)
{
    common::retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);

    txn_metadata_t commit_ts_metadata = s_txn_metadata_map[commit_ts];
    return commit_ts_metadata.is_validating();
}

bool txn_metadata_t::is_txn_decided(gaia_txn_id_t commit_ts)
{
    common::retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);

    txn_metadata_t commit_ts_metadata = s_txn_metadata_map[commit_ts];
    return commit_ts_metadata.is_decided();
}

bool txn_metadata_t::is_txn_committed(gaia_txn_id_t commit_ts)
{
    common::retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);

    txn_metadata_t commit_ts_metadata = s_txn_metadata_map[commit_ts];
    return commit_ts_metadata.is_committed();
}

// bool txn_metadata_t::is_txn_aborted(gaia_txn_id_t commit_ts)
// {
//     common::retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);

//     txn_metadata_t commit_ts_metadata = s_txn_metadata_map[commit_ts];
//     return commit_ts_metadata.is_aborted();
// }

bool txn_metadata_t::is_txn_gc_complete(gaia_txn_id_t commit_ts)
{
    common::retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);

    txn_metadata_t commit_ts_metadata = s_txn_metadata_map[commit_ts];
    return commit_ts_metadata.is_gc_complete();
}

bool txn_metadata_t::is_txn_durable(gaia_txn_id_t commit_ts)
{
    common::retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);

    txn_metadata_t commit_ts_metadata = s_txn_metadata_map[commit_ts];
    return commit_ts_metadata.is_durable();
}

bool txn_metadata_t::is_txn_active(gaia_txn_id_t begin_ts)
{
    common::retail_assert(is_begin_ts(begin_ts), c_message_not_a_begin_timestamp);

    txn_metadata_t begin_ts_metadata = s_txn_metadata_map[begin_ts];
    return begin_ts_metadata.is_active();
}

// bool txn_metadata_t::is_txn_terminated(gaia_txn_id_t begin_ts)
// {
//     common::retail_assert(is_begin_ts(begin_ts), c_message_not_a_begin_timestamp);

//     txn_metadata_t begin_ts_metadata = s_txn_metadata_map[begin_ts];
//     return begin_ts_metadata.is_terminated();
// }

gaia_txn_id_t txn_metadata_t::get_begin_ts(gaia_txn_id_t commit_ts)
{
    common::retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);

    txn_metadata_t commit_ts_metadata = s_txn_metadata_map[commit_ts];
    gaia_txn_id_t begin_ts = commit_ts_metadata.get_timestamp();
    common::retail_assert(begin_ts != c_invalid_gaia_txn_id, "begin_ts must be valid!");

    return begin_ts;
}

gaia_txn_id_t txn_metadata_t::get_commit_ts(gaia_txn_id_t begin_ts)
{
    common::retail_assert(is_begin_ts(begin_ts), c_message_not_a_begin_timestamp);
    common::retail_assert(is_txn_submitted(begin_ts), "begin_ts must be submitted!");

    txn_metadata_t begin_ts_metadata = s_txn_metadata_map[begin_ts];
    gaia_txn_id_t commit_ts = begin_ts_metadata.get_timestamp();
    if (is_txn_submitted(begin_ts))
    {
        common::retail_assert(commit_ts != c_invalid_gaia_txn_id, "commit_ts must be valid for a submitted begin_ts!");
    }

    return commit_ts;
}

// uint64_t txn_metadata_t::get_status(gaia_txn_id_t ts)
// {
//     common::retail_assert(
//         !is_uninitialized_ts(ts) && !is_sealed_ts(ts),
//         "Invalid txn metadata!");

//     txn_metadata_t txn_metadata = s_txn_metadata_map[ts];
//     return txn_metadata.get_status();
// }

int txn_metadata_t::get_txn_log_fd(gaia_txn_id_t commit_ts)
{
    common::retail_assert(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);

    txn_metadata_t commit_ts_metadata = s_txn_metadata_map[commit_ts];
    return commit_ts_metadata.get_txn_log_fd();
}

bool txn_metadata_t::is_uninitialized() const
{
    return (m_value == c_value_uninitialized);
}

bool txn_metadata_t::is_sealed() const
{
    return (m_value == c_value_sealed);
}

bool txn_metadata_t::is_begin_ts() const
{
    // The "uninitialized" value has the commit bit unset, so we need to check it as well.
    return (!is_uninitialized() && ((m_value & c_txn_status_commit_mask) == 0));
}

bool txn_metadata_t::is_commit_ts() const
{
    // The "sealed" value has the commit bit set, so we need to check it as well.
    return (!is_sealed() && ((m_value & c_txn_status_commit_mask) == c_txn_status_commit_mask));
}

bool txn_metadata_t::is_submitted() const
{
    return (get_status() == c_txn_status_submitted);
}

bool txn_metadata_t::is_validating() const
{
    return (get_status() == c_txn_status_validating);
}

bool txn_metadata_t::is_decided() const
{
    constexpr uint64_t c_decided_mask = c_txn_status_decided << c_txn_status_flags_shift;
    return ((m_value & c_decided_mask) == c_decided_mask);
}

bool txn_metadata_t::is_committed() const
{
    return (get_status() == c_txn_status_committed);
}

// bool txn_metadata_t::is_aborted() const
// {
//     return (get_status() == c_txn_status_aborted);
// }

bool txn_metadata_t::is_gc_complete() const
{
    uint64_t gc_flags = (m_value & c_txn_gc_flags_mask) >> c_txn_gc_flags_shift;
    return (gc_flags == c_txn_gc_complete);
}

bool txn_metadata_t::is_durable() const
{
    uint64_t persistence_flags = (m_value & c_txn_persistence_flags_mask) >> c_txn_persistence_flags_shift;
    return (persistence_flags == c_txn_persistence_complete);
}

bool txn_metadata_t::is_active() const
{
    return (get_status() == c_txn_status_active);
}

// bool txn_metadata_t::is_terminated() const
// {
//     return (get_status() == c_txn_status_terminated);
// }

uint64_t txn_metadata_t::get_status() const
{
    return ((m_value & c_txn_status_flags_mask) >> c_txn_status_flags_shift);
}

gaia_txn_id_t txn_metadata_t::get_timestamp() const
{
    // The timestamp is in the low 42 bits of the ts metadata.
    return (m_value & c_txn_ts_mask);
}

int txn_metadata_t::get_txn_log_fd() const
{
    // The txn log fd is the 16 bits of the ts metadata after the 3 status bits.
    uint16_t fd = (m_value & c_txn_log_fd_mask) >> c_txn_log_fd_shift;

    // If the log fd is invalidated, then return an invalid fd value (-1).
    return (fd != c_invalid_txn_log_fd_bits) ? static_cast<int>(fd) : -1;
}

void txn_metadata_t::invalidate_txn_log_fd()
{
    m_value |= c_txn_log_fd_mask;
}

void txn_metadata_t::set_terminated()
{
    constexpr uint64_t c_terminated_flags = c_txn_status_terminated << c_txn_status_flags_shift;

    m_value = c_terminated_flags | (m_value & ~c_txn_status_flags_mask);
}

void txn_metadata_t::set_gc_complete()
{
    m_value |= (c_txn_gc_complete << c_txn_gc_flags_shift);
}
