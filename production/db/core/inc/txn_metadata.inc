/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

constexpr char c_message_uninitialized_timestamp[] = "Uninitialized timestamp!";
constexpr char c_message_not_a_begin_timestamp[] = "Not a begin timestamp!";
constexpr char c_message_not_a_commit_timestamp[] = "Not a commit timestamp!";

txn_metadata_t::txn_metadata_t(gaia_txn_id_t ts)
    : m_ts(ts)
{
    ASSERT_PRECONDITION(m_ts.is_valid(), "Invalid txn timestamp!");
    txn_metadata_entry_t::check_ts_size(m_ts);
}

bool operator==(txn_metadata_t a, txn_metadata_t b)
{
    return (a.m_ts == b.m_ts);
}

bool operator!=(txn_metadata_t a, txn_metadata_t b)
{
    return (a.m_ts != b.m_ts);
}

gaia_txn_id_t txn_metadata_t::get_timestamp()
{
    return m_ts;
}

bool txn_metadata_t::is_uninitialized()
{
    return get_entry().is_uninitialized();
}

bool txn_metadata_t::is_sealed()
{
    return get_entry().is_sealed();
}

bool txn_metadata_t::is_begin_ts()
{
    return get_entry().is_begin_ts_entry();
}

bool txn_metadata_t::is_commit_ts()
{
    return get_entry().is_commit_ts_entry();
}

bool txn_metadata_t::is_submitted()
{
    return get_entry().is_submitted();
}

bool txn_metadata_t::is_validating()
{
    return get_entry().is_validating();
}

bool txn_metadata_t::is_decided()
{
    return get_entry().is_decided();
}

bool txn_metadata_t::is_committed()
{
    return get_entry().is_committed();
}

bool txn_metadata_t::is_aborted()
{
    return get_entry().is_aborted();
}

bool txn_metadata_t::is_gc_complete()
{
    return get_entry().is_gc_complete();
}

bool txn_metadata_t::is_durable()
{
    return get_entry().is_durable();
}

bool txn_metadata_t::is_active()
{
    return get_entry().is_active();
}

bool txn_metadata_t::is_terminated()
{
    return get_entry().is_terminated();
}

bool txn_metadata_t::is_uninitialized_ts(gaia_txn_id_t ts)
{
    return txn_metadata_t(ts).is_uninitialized();
}

bool txn_metadata_t::is_sealed_ts(gaia_txn_id_t ts)
{
    return txn_metadata_t(ts).is_sealed();
}

bool txn_metadata_t::is_begin_ts(gaia_txn_id_t ts)
{
    return txn_metadata_t(ts).is_begin_ts();
}

bool txn_metadata_t::is_commit_ts(gaia_txn_id_t ts)
{
    return txn_metadata_t(ts).is_commit_ts();
}

bool txn_metadata_t::is_txn_submitted(gaia_txn_id_t begin_ts)
{
    return txn_metadata_t(begin_ts).is_submitted();
}

bool txn_metadata_t::is_txn_validating(gaia_txn_id_t commit_ts)
{
    return txn_metadata_t(commit_ts).is_validating();
}

bool txn_metadata_t::is_txn_decided(gaia_txn_id_t commit_ts)
{
    return txn_metadata_t(commit_ts).is_decided();
}

bool txn_metadata_t::is_txn_committed(gaia_txn_id_t commit_ts)
{
    return txn_metadata_t(commit_ts).is_committed();
}

bool txn_metadata_t::is_txn_aborted(gaia_txn_id_t commit_ts)
{
    return txn_metadata_t(commit_ts).is_aborted();
}

bool txn_metadata_t::is_txn_gc_complete(gaia_txn_id_t commit_ts)
{
    return txn_metadata_t(commit_ts).is_gc_complete();
}

bool txn_metadata_t::is_txn_durable(gaia_txn_id_t commit_ts)
{
    return txn_metadata_t(commit_ts).is_durable();
}

bool txn_metadata_t::is_txn_active(gaia_txn_id_t begin_ts)
{
    return txn_metadata_t(begin_ts).is_active();
}

bool txn_metadata_t::is_txn_terminated(gaia_txn_id_t begin_ts)
{
    return txn_metadata_t(begin_ts).is_terminated();
}

txn_metadata_t txn_metadata_t::get_begin_ts_metadata()
{
    ASSERT_PRECONDITION(is_commit_ts(), "Not a commit timestamp!");
    return txn_metadata_t(get_entry().get_timestamp());
}

std::optional<txn_metadata_t> txn_metadata_t::get_commit_ts_metadata()
{
    ASSERT_PRECONDITION(is_begin_ts(), "Not a begin timestamp!");
    gaia_txn_id_t commit_ts = get_entry().get_timestamp();
    if (commit_ts.is_valid())
    {
        return txn_metadata_t(commit_ts);
    }
    return std::nullopt;
}

gaia_txn_id_t txn_metadata_t::get_begin_ts_from_commit_ts(gaia_txn_id_t commit_ts)
{
    return txn_metadata_t(commit_ts).get_begin_ts_metadata().get_timestamp();
}

gaia_txn_id_t txn_metadata_t::get_commit_ts_from_begin_ts(gaia_txn_id_t begin_ts)
{
    auto commit_ts_metadata = txn_metadata_t(begin_ts).get_commit_ts_metadata();
    return commit_ts_metadata ? commit_ts_metadata->get_timestamp() : c_invalid_gaia_txn_id;
}

int txn_metadata_t::get_txn_log_fd()
{
    return get_entry().get_log_fd();
}

bool txn_metadata_t::invalidate_txn_log_fd()
{
    // We need a CAS because only one thread can be allowed to invalidate the fd
    // entry and hence to close the fd.
    while (true)
    {
        txn_metadata_entry_t expected_value{
            get_entry()};

        // If this metadata entry was already invalidated, then return failure.
        if (expected_value.get_log_fd() == -1)
        {
            return false;
        }

        txn_metadata_entry_t desired_value{
            expected_value.invalidate_log_fd()};
        txn_metadata_entry_t actual_value{
            compare_exchange(expected_value, desired_value)};

        if (actual_value == expected_value)
        {
            break;
        }
    }

    return true;
}

void txn_metadata_t::set_active_txn_submitted(gaia_txn_id_t commit_ts)
{
    // Only an active txn can be submitted.
    ASSERT_PRECONDITION(is_active(), "Not an active transaction!");
    ASSERT_PRECONDITION(is_commit_ts(commit_ts), c_message_not_a_commit_timestamp);

    // We don't need a CAS here because only the session thread can submit or terminate a txn,
    // and an active txn cannot be sealed.
    set_entry(get_entry().set_submitted(commit_ts));
}

void txn_metadata_t::set_active_txn_terminated()
{
    // Only an active txn can be terminated.
    ASSERT_PRECONDITION(is_active(), "Not an active transaction!");

    // We don't need a CAS here because only the session thread can submit or terminate a txn,
    // and an active txn cannot be sealed.
    set_entry(get_entry().set_terminated());
}

void txn_metadata_t::update_txn_decision(bool has_committed)
{
    txn_metadata_entry_t expected_value{
        get_entry()};
    txn_metadata_entry_t desired_value{
        expected_value.set_decision(has_committed)};

    if (expected_value != compare_exchange(expected_value, desired_value))
    {
        // The only state transition allowed from TXN_VALIDATING is to TXN_DECIDED.
        ASSERT_POSTCONDITION(
            is_decided(),
            "commit_ts metadata in validating state can only transition to a decided state!");

        // If another txn validated before us, they should have reached the same decision.
        ASSERT_POSTCONDITION(
            is_committed() == has_committed,
            "Inconsistent txn decision detected!");
    }
}

void txn_metadata_t::set_txn_durable()
{
    while (true)
    {
        txn_metadata_entry_t expected_value{
            get_entry()};
        txn_metadata_entry_t desired_value{
            expected_value.set_durable()};
        txn_metadata_entry_t actual_value{
            compare_exchange(expected_value, desired_value)};

        if (actual_value == expected_value)
        {
            break;
        }
    }
}

bool txn_metadata_t::set_txn_gc_complete()
{
    txn_metadata_entry_t expected_value{
        get_entry()};
    txn_metadata_entry_t desired_value{
        expected_value.set_gc_complete()};
    txn_metadata_entry_t actual_value{
        compare_exchange(expected_value, desired_value)};

    return (actual_value == expected_value);
}

// This is designed for implementing "fences" that can guarantee no thread can
// ever claim a timestamp, by marking that timestamp permanently sealed. Sealing
// can only be performed on an "uninitialized" metadata entry, not on any valid
// metadata entry. When a session thread beginning or committing a txn finds
// that its begin_ts or commit_ts has been sealed upon initializing the metadata
// entry for that timestamp, it simply allocates another timestamp and retries.
// This is possible because we never publish a newly allocated timestamp until
// we know that its metadata entry has been successfully initialized.
bool txn_metadata_t::seal_if_uninitialized()
{
    // If the metadata is not uninitialized, we can't seal it.
    if (!is_uninitialized())
    {
        return false;
    }

    txn_metadata_entry_t expected_value{
        txn_metadata_entry_t::uninitialized_value()};
    txn_metadata_entry_t desired_value{
        txn_metadata_entry_t::sealed_value()};
    txn_metadata_entry_t actual_value{
        compare_exchange(expected_value, desired_value)};

    if (actual_value != expected_value)
    {
        // We don't consider TXN_SUBMITTED or TXN_TERMINATED to be valid prior
        // states, because only the submitting thread can transition the txn to
        // these states.
        ASSERT_INVARIANT(
            actual_value != txn_metadata_entry_t::uninitialized_value(),
            "An uninitialized txn metadata entry cannot fail to be sealed!");
        return false;
    }

    return true;
}

int txn_metadata_t::get_txn_log_fd(gaia_txn_id_t commit_ts)
{
    return txn_metadata_t(commit_ts).get_txn_log_fd();
}

bool txn_metadata_t::seal_uninitialized_ts(gaia_txn_id_t ts)
{
    return txn_metadata_t(ts).seal_if_uninitialized();
}

bool txn_metadata_t::invalidate_txn_log_fd(gaia_txn_id_t commit_ts)
{
    return txn_metadata_t(commit_ts).invalidate_txn_log_fd();
}

void txn_metadata_t::set_active_txn_submitted(gaia_txn_id_t begin_ts, gaia_txn_id_t commit_ts)
{
    return txn_metadata_t(begin_ts).set_active_txn_submitted(commit_ts);
}

void txn_metadata_t::set_active_txn_terminated(gaia_txn_id_t begin_ts)
{
    return txn_metadata_t(begin_ts).set_active_txn_terminated();
}

void txn_metadata_t::update_txn_decision(gaia_txn_id_t commit_ts, bool has_committed)
{
    return txn_metadata_t(commit_ts).update_txn_decision(has_committed);
}

void txn_metadata_t::set_txn_durable(gaia_txn_id_t commit_ts)
{
    return txn_metadata_t(commit_ts).set_txn_durable();
}

bool txn_metadata_t::set_txn_gc_complete(gaia_txn_id_t commit_ts)
{
    return txn_metadata_t(commit_ts).set_txn_gc_complete();
}

txn_metadata_entry_t txn_metadata_t::get_entry()
{
    ASSERT_PRECONDITION(s_txn_metadata_map, "Transaction metadata has not been initialized!");

    return txn_metadata_entry_t{s_txn_metadata_map[m_ts].load()};
}

void txn_metadata_t::set_entry(txn_metadata_entry_t entry)
{
    s_txn_metadata_map[m_ts].store(entry.get_word());
}

txn_metadata_entry_t txn_metadata_t::compare_exchange(
    txn_metadata_entry_t expected_value, txn_metadata_entry_t desired_value)
{
    auto expected_word = expected_value.get_word();
    auto desired_word = desired_value.get_word();
    // compare_exchange_strong() cannot spuriously fail, so if
    // actual_value == expected_value, then we know that
    // desired_value must have been stored at the address.
    s_txn_metadata_map[m_ts].compare_exchange_strong(expected_word, desired_word);
    // expected_word is passed by reference, and on exit holds the initial word at the address.
    txn_metadata_entry_t actual_value{expected_word};
    return actual_value;
}
