/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

bool txn_metadata_t::is_uninitialized() const
{
    return (value == c_value_uninitialized);
}

bool txn_metadata_t::is_sealed() const
{
    return (value == c_value_sealed);
}

bool txn_metadata_t::is_begin_ts() const
{
    // The "uninitialized" value has the commit bit unset, so we need to check it as well.
    return (!is_uninitialized() && ((value & c_txn_status_commit_mask) == 0));
}

bool txn_metadata_t::is_commit_ts() const
{
    // The "sealed" value has the commit bit set, so we need to check it as well.
    return (!is_sealed() && ((value & c_txn_status_commit_mask) == c_txn_status_commit_mask));
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
    return ((value & c_decided_mask) == c_decided_mask);
}

bool txn_metadata_t::is_committed() const
{
    return (get_status() == c_txn_status_committed);
}

bool txn_metadata_t::is_aborted() const
{
    return (get_status() == c_txn_status_aborted);
}

bool txn_metadata_t::is_gc_complete() const
{
    uint64_t gc_flags = (value & c_txn_gc_flags_mask) >> c_txn_gc_flags_shift;
    return (gc_flags == c_txn_gc_complete);
}

bool txn_metadata_t::is_durable() const
{
    uint64_t persistence_flags = (value & c_txn_persistence_flags_mask) >> c_txn_persistence_flags_shift;
    return (persistence_flags == c_txn_persistence_complete);
}

bool txn_metadata_t::is_active() const
{
    return (get_status() == c_txn_status_active);
}

bool txn_metadata_t::is_terminated() const
{
    return (get_status() == c_txn_status_terminated);
}

void txn_metadata_t::invalidate_txn_log_fd()
{
    value |= c_txn_log_fd_mask;
}

void txn_metadata_t::set_terminated()
{
    constexpr uint64_t c_terminated_flags = c_txn_status_terminated << c_txn_status_flags_shift;

    value = c_terminated_flags | (value & ~c_txn_status_flags_mask);
}

void txn_metadata_t::set_gc_complete()
{
    value |= (c_txn_gc_complete << c_txn_gc_flags_shift);
}

uint64_t txn_metadata_t::get_status() const
{
    return ((value & c_txn_status_flags_mask) >> c_txn_status_flags_shift);
}

uint64_t txn_metadata_t::get_timestamp() const
{
    // The timestamp is in the low 42 bits of the ts metadata.
    return (value & c_txn_ts_mask);
}

int txn_metadata_t::get_txn_log_fd() const
{
    // The txn log fd is the 16 bits of the ts metadata after the 3 status bits.
    uint16_t fd = (value & c_txn_log_fd_mask) >> c_txn_log_fd_shift;

    // If the log fd is invalidated, then return an invalid fd value (-1).
    return (fd != c_invalid_txn_log_fd_bits) ? static_cast<int>(fd) : -1;
}
