/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

bool ts_info_t::is_unknown() const
{
    return value == c_value_unknown;
}

bool ts_info_t::is_invalid() const
{
    return value == c_value_invalid;
}

bool ts_info_t::is_begin_ts() const
{
    // The "unknown" value has the commit bit unset, so we need to check it as well.
    return (!is_unknown() && ((value & c_txn_status_commit_mask) == 0));
}

bool ts_info_t::is_commit_ts() const
{
    // The "invalid" value has the commit bit set, so we need to check it as well.
    return (!is_invalid() && ((value & c_txn_status_commit_mask) == c_txn_status_commit_mask));
}

bool ts_info_t::is_submitted() const
{
    return (get_status() == c_txn_status_submitted);
}

bool ts_info_t::is_validating() const
{
    return (get_status() == c_txn_status_validating);
}

bool ts_info_t::is_decided() const
{
    constexpr uint64_t c_decided_mask = c_txn_status_decided << c_txn_status_flags_shift;
    return ((value & c_decided_mask) == c_decided_mask);
}

bool ts_info_t::is_committed() const
{
    return (get_status() == c_txn_status_committed);
}

bool ts_info_t::is_aborted() const
{
    return (get_status() == c_txn_status_aborted);
}

bool ts_info_t::is_gc_complete() const
{
    uint64_t gc_flags = (value & c_txn_gc_flags_mask) >> c_txn_gc_flags_shift;
    return (gc_flags == c_txn_gc_complete);
}

bool ts_info_t::is_durable() const
{
    uint64_t persistence_flags = (value & c_txn_persistence_flags_mask) >> c_txn_persistence_flags_shift;
    return (persistence_flags == c_txn_persistence_complete);
}

bool ts_info_t::is_active() const
{
    return (get_status() == c_txn_status_active);
}

bool ts_info_t::is_terminated() const
{
    return (get_status() == c_txn_status_terminated);
}

ts_info_t ts_info_t::invalidate_txn_log_fd() const
{
    ts_info_t new_ts_info(value | c_txn_log_fd_mask);

    return new_ts_info;
}

ts_info_t ts_info_t::set_terminated() const
{
    constexpr uint64_t c_terminated_flags = c_txn_status_terminated << c_txn_status_flags_shift;

    ts_info_t new_ts_info(c_terminated_flags | (value & ~c_txn_status_flags_mask));

    return new_ts_info;
}

ts_info_t ts_info_t::set_gc_complete() const
{
    ts_info_t new_ts_info(value | (c_txn_gc_complete << c_txn_gc_flags_shift));

    return new_ts_info;
}

uint64_t ts_info_t::get_status() const
{
    return ((value & c_txn_status_flags_mask) >> c_txn_status_flags_shift);
}

uint64_t ts_info_t::get_timestamp() const
{
    // The timestamp is in the low 42 bits of the ts info.
    return (value & c_txn_ts_mask);
}

int ts_info_t::get_txn_log_fd() const
{
    // The txn log fd is the 16 bits of the ts info after the 3 status bits.
    uint16_t fd = (value & c_txn_log_fd_mask) >> c_txn_log_fd_shift;

    // If the log fd is invalidated, then return an invalid fd value (-1).
    return (fd != c_invalid_txn_log_fd_bits) ? static_cast<int>(fd) : -1;
}
