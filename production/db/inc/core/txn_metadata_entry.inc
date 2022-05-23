////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

bool operator==(txn_metadata_entry_t a, txn_metadata_entry_t b)
{
    return (a.m_word == b.m_word);
}

bool operator!=(txn_metadata_entry_t a, txn_metadata_entry_t b)
{
    return (a.m_word != b.m_word);
}

uint64_t txn_metadata_entry_t::get_word()
{
    return m_word;
}

void txn_metadata_entry_t::check_ts_size(gaia_txn_id_t ts)
{
    ASSERT_PRECONDITION(
        ts < (1UL << c_txn_ts_bit_width),
        "Timestamp values must fit in 42 bits!");
}

constexpr size_t txn_metadata_entry_t::get_max_ts_count()
{
    return (1UL << c_txn_ts_bit_width);
}

txn_metadata_entry_t txn_metadata_entry_t::uninitialized_value()
{
    return txn_metadata_entry_t{c_value_uninitialized};
}

txn_metadata_entry_t txn_metadata_entry_t::sealed_value()
{
    return txn_metadata_entry_t{c_value_sealed};
}

txn_metadata_entry_t txn_metadata_entry_t::new_begin_ts_entry()
{
    // Any initial begin_ts metadata entry must have its status initialized to TXN_ACTIVE.
    // All other bits should be 0.
    return txn_metadata_entry_t{c_txn_status_active << c_txn_status_flags_shift};
}

txn_metadata_entry_t txn_metadata_entry_t::new_commit_ts_entry(gaia_txn_id_t begin_ts, db::log_offset_t log_offset)
{
    // We construct the commit_ts metadata by concatenating required bits.
    uint64_t shifted_log_offset = static_cast<uint64_t>(log_offset) << c_txn_log_offset_shift;
    constexpr uint64_t c_shifted_status_flags{
        c_txn_status_validating << c_txn_status_flags_shift};
    return txn_metadata_entry_t{c_shifted_status_flags | shifted_log_offset | begin_ts};
}

bool txn_metadata_entry_t::is_uninitialized()
{
    return (m_word == c_value_uninitialized);
}

bool txn_metadata_entry_t::is_sealed()
{
    return (m_word == c_value_sealed);
}

bool txn_metadata_entry_t::is_begin_ts_entry()
{
    // "Uninitialized" values also have the commit bit unset, but they're not considered to be begin timestamps.
    return (!is_uninitialized() && ((m_word & c_txn_status_commit_mask) == 0));
}

bool txn_metadata_entry_t::is_commit_ts_entry()
{
    // "Sealed" values also have the commit bit set, but they're not considered to be commit timestamps.
    return (!is_sealed() && ((m_word & c_txn_status_commit_mask) == c_txn_status_commit_mask));
}

bool txn_metadata_entry_t::is_submitted()
{
    return (get_status() == c_txn_status_submitted);
}

bool txn_metadata_entry_t::is_validating()
{
    return (get_status() == c_txn_status_validating);
}

bool txn_metadata_entry_t::is_decided()
{
    constexpr uint64_t c_decided_mask = c_txn_status_decided << c_txn_status_flags_shift;
    return ((m_word & c_decided_mask) == c_decided_mask);
}

bool txn_metadata_entry_t::is_committed()
{
    return (get_status() == c_txn_status_committed);
}

bool txn_metadata_entry_t::is_aborted()
{
    return (get_status() == c_txn_status_aborted);
}

bool txn_metadata_entry_t::is_gc_complete()
{
    uint64_t gc_flags = (m_word & c_txn_gc_flags_mask) >> c_txn_gc_flags_shift;
    return (gc_flags == c_txn_gc_complete);
}

bool txn_metadata_entry_t::is_durable()
{
    uint64_t persistence_flags = (m_word & c_txn_persistence_flags_mask) >> c_txn_persistence_flags_shift;
    return (persistence_flags == c_txn_persistence_complete);
}

bool txn_metadata_entry_t::is_active()
{
    return (get_status() == c_txn_status_active);
}

bool txn_metadata_entry_t::is_terminated()
{
    return (get_status() == c_txn_status_terminated);
}

uint64_t txn_metadata_entry_t::get_status()
{
    return ((m_word & c_txn_status_flags_mask) >> c_txn_status_flags_shift);
}

gaia_txn_id_t txn_metadata_entry_t::get_timestamp()
{
    // The timestamp is in the low 42 bits of the txn metadata.
    return (m_word & c_txn_ts_mask);
}

db::log_offset_t txn_metadata_entry_t::get_log_offset()
{
    ASSERT_PRECONDITION(is_commit_ts_entry(), "Not a commit timestamp!");

    // The txn log offset is the 16 bits of the ts metadata after the 3 status bits.
    return (m_word & c_txn_log_offset_mask) >> c_txn_log_offset_shift;
}

txn_metadata_entry_t txn_metadata_entry_t::set_submitted(gaia_txn_id_t commit_ts)
{
    ASSERT_PRECONDITION(is_active(), "Not an active transaction!");

    // Transition the begin_ts metadata to the TXN_SUBMITTED state.
    constexpr uint64_t c_submitted_flags
        = c_txn_status_submitted << c_txn_status_flags_shift;

    // A submitted begin_ts metadata has the commit_ts in its low bits.
    return txn_metadata_entry_t{c_submitted_flags | commit_ts};
}

txn_metadata_entry_t txn_metadata_entry_t::set_terminated()
{
    ASSERT_PRECONDITION(is_active(), "Not an active transaction!");

    constexpr uint64_t c_terminated_flags = c_txn_status_terminated << c_txn_status_flags_shift;
    return txn_metadata_entry_t{
        c_terminated_flags | (m_word & ~c_txn_status_flags_mask)};
}

txn_metadata_entry_t txn_metadata_entry_t::set_decision(bool has_committed)
{
    // The commit_ts metadata must be in state TXN_VALIDATING or TXN_DECIDED.
    // (We allow the latter to enable idempotent concurrent validation.)
    ASSERT_PRECONDITION(
        is_validating() || is_decided(),
        "commit_ts metadata must be in validating or decided state!");

    uint64_t decided_status_flags{has_committed ? c_txn_status_committed : c_txn_status_aborted};

    // This masks out all bits except the commit_ts flag bits.
    constexpr uint64_t c_commit_flags_mask = ~c_txn_status_commit_ts << c_txn_status_flags_shift;

    // Turn off all commit flag bits, then set the decision flags.
    return txn_metadata_entry_t{
        (m_word & ~c_commit_flags_mask) | (decided_status_flags << c_txn_status_flags_shift)};
}

txn_metadata_entry_t txn_metadata_entry_t::set_durable()
{
    ASSERT_PRECONDITION(is_decided(), "Not a decided transaction!");

    // Set persistence status to TXN_DURABLE.
    return txn_metadata_entry_t{
        m_word | (c_txn_persistence_complete << c_txn_persistence_flags_shift)};
}

txn_metadata_entry_t txn_metadata_entry_t::set_gc_complete()
{
    ASSERT_PRECONDITION(is_decided(), "Not a decided transaction!");

    // Set GC status to TXN_GC_COMPLETE.
    return txn_metadata_entry_t{
        m_word | (c_txn_gc_complete << c_txn_gc_flags_shift)};
}

const char* txn_metadata_entry_t::status_to_str()
{
    ASSERT_PRECONDITION(
        !is_uninitialized() && !is_sealed(),
        "Not a valid txn metadata entry!");

    uint64_t status = get_status();
    switch (status)
    {
    case c_txn_status_active:
        return "ACTIVE";
    case c_txn_status_submitted:
        return "SUBMITTED";
    case c_txn_status_terminated:
        return "TERMINATED";
    case c_txn_status_validating:
        return "VALIDATING";
    case c_txn_status_committed:
        return "COMMITTED";
    case c_txn_status_aborted:
        return "ABORTED";
    default:
        ASSERT_UNREACHABLE("Unexpected txn metadata status flags!");
    }

    return nullptr;
}

std::string txn_metadata_entry_t::dump_metadata()
{
    std::stringstream str;
    std::bitset<c_txn_metadata_bit_width> metadata_bits{m_word};

    str << "Transaction metadata bits: " << metadata_bits << std::endl;

    if (is_uninitialized())
    {
        str << "UNINITIALIZED" << std::endl;
        return str.str();
    }

    if (is_sealed())
    {
        str << "SEALED" << std::endl;
        return str.str();
    }

    str << "Type: " << (is_commit_ts_entry() ? "COMMIT_TIMESTAMP" : "BEGIN_TIMESTAMP") << std::endl;
    str << "Status: " << status_to_str() << std::endl;

    if (is_commit_ts_entry())
    {
        str << "Log offset for commit_ts metadata: " << get_log_offset() << std::endl;
    }
    return str.str();
}
