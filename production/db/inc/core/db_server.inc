/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

server_config_t::persistence_mode_t server_config_t::persistence_mode()
{
    return m_persistence_mode;
}

const std::string& server_config_t::data_dir()
{
    return m_data_dir;
}

const std::string& server_config_t::instance_name()
{
    return m_instance_name;
}

bool server_config_t::skip_catalog_integrity_checks()
{
    return m_skip_catalog_integrity_checks;
}

gaia_txn_id_t server_t::txn_id()
{
    return s_session_context->txn_context->txn_id;
}

log_offset_t server_t::txn_log_offset()
{
    return s_session_context->txn_context->txn_log_offset;
}

std::vector<std::pair<gaia_txn_id_t, log_offset_t>>& server_t::txn_logs_for_snapshot()
{
    return s_session_context->txn_context->txn_logs_for_snapshot;
}

mapped_data_t<locators_t>& server_t::local_snapshot_locators()
{
    return s_session_context->txn_context->local_snapshot_locators;
}

size_t server_t::last_snapshot_processed_log_record_count()
{
    return s_session_context->txn_context->last_snapshot_processed_log_record_count;
}

int server_t::session_socket()
{
    return s_session_context->session_socket;
}

std::vector<std::thread>& server_t::session_owned_threads()
{
    return s_session_context->session_owned_threads;
}

std::unordered_map<chunk_offset_t, chunk_version_t>& server_t::map_gc_chunks_to_versions()
{
    return s_session_context->map_gc_chunks_to_versions;
}

server_t::safe_ts_t::safe_ts_t(gaia_txn_id_t safe_ts)
    : m_ts(safe_ts)
{
    // If safe_ts is the minimum element in the safe_ts array, then
    // replace this thread's reserved safe_ts entry.
    bool is_new_min_safe_ts = true;
    if (!s_safe_ts_values.empty())
    {
        is_new_min_safe_ts = (safe_ts < s_safe_ts_values[0]);
    }

    if (is_new_min_safe_ts)
    {
        if (!gaia::db::server_t::reserve_safe_ts(safe_ts))
        {
            // Force the client to handle the validation failure.
            throw safe_ts_failure();
        }
    }

    // We have successfully reserved the new safe_ts if necessary, so
    // add the new safe_ts to the sorted array of safe_ts values.
    s_safe_ts_values.push_back(safe_ts);
    std::sort(s_safe_ts_values.begin(), s_safe_ts_values.end());
}

server_t::safe_ts_t::safe_ts_t(safe_ts_t&& other) noexcept
    : m_ts(std::exchange(other.m_ts, c_invalid_gaia_txn_id))
{
}

server_t::safe_ts_t& server_t::safe_ts_t::operator=(safe_ts_t&& other) noexcept
{
    this->m_ts = std::exchange(other.m_ts, c_invalid_gaia_txn_id);
    return *this;
}

server_t::safe_ts_t::operator gaia_txn_id_t() const
{
    return m_ts;
}

server_t::safe_ts_t::~safe_ts_t()
{
    // A moved-from object has an invalid timestamp.
    if (!m_ts.is_valid())
    {
        return;
    }

    // The destructor cannot fail, so we need to ensure that all
    // consistency conditions we already satisfied still hold.
    ASSERT_INVARIANT(!s_safe_ts_values.empty(), "Expected safe_ts array to be non-empty!");

    // First handle the case where our safe_ts was the only value in the
    // safe_ts array.
    if (s_safe_ts_values.size() == 1)
    {
        // Release this thread's reserved safe_ts value.
        gaia::db::server_t::release_safe_ts();
        // Remove the last element from the safe_ts array.
        s_safe_ts_values.pop_back();
        return;
    }

    // Save the current minimum safe_ts value.
    gaia_txn_id_t prev_min_ts = s_safe_ts_values[0];
    // Because our safe_ts array is sorted, we can use binary search to
    // find the index of our safe_ts.
    auto it = std::lower_bound(s_safe_ts_values.begin(), s_safe_ts_values.end(), m_ts);
    ASSERT_INVARIANT(*it == m_ts, "Cannot find this timestamp in the safe_ts array!");
    // Move the last element into our entry.
    *it = std::move(s_safe_ts_values.back());
    // Remove the moved last element.
    s_safe_ts_values.pop_back();
    // Restore sorted order.
    std::sort(s_safe_ts_values.begin(), s_safe_ts_values.end());

    // If the minimum safe_ts has changed, then we need to replace the
    // published safe_ts value.
    gaia_txn_id_t new_min_ts = s_safe_ts_values[0];
    if (new_min_ts != prev_min_ts)
    {
        ASSERT_INVARIANT(
            new_min_ts > prev_min_ts,
            "The new minimum safe_ts must be larger than the previous minimum safe_ts!");
        bool reservation_succeeded = gaia::db::server_t::reserve_safe_ts(s_safe_ts_values[0]);
        // The new minimum safe_ts was protected by the previous minimum
        // safe_ts from falling behind the pre-truncate watermark, so
        // its reservation must succeed.
        ASSERT_INVARIANT(reservation_succeeded, "Reservation of new minimum safe_ts cannot fail!");
    }
}

// The constructor reads the current value of the given watermark, and
// initializes a safe_ts_t object to that value. If initialization fails, then
// it retries after reading the value of the given watermark again.
server_t::safe_watermark_t::safe_watermark_t(watermark_type_t watermark_type)
{
    // Retry until we have a valid safe_ts for the current value of
    // the watermark.
    while (true)
    {
        gaia_txn_id_t watermark_ts = get_watermark(watermark_type);
        try
        {
            // First try to obtain a local safe_ts for the current value of the
            // watermark. If the local safe_ts is successfully initialized, use
            // it to initialize our member safe_ts. (Initialization cannot fail
            // because the move assignment operator does not release the "safe
            // timestamp" that was reserved in the local safe_ts's constructor.)
            safe_ts_t safe_ts(watermark_ts);
            m_safe_ts = std::move(safe_ts);
            // NB: It is legal to call any method without preconditions on a
            // moved-from safe_ts, because the standard specifies that
            // "Moved-from objects shall be placed in a valid but unspecified
            // state."
            ASSERT_POSTCONDITION(
                static_cast<gaia_txn_id_t>(safe_ts).is_valid() == false,
                "safe_ts should contain an invalid timestamp after moving!");
            break;
        }
        catch (const safe_ts_failure&)
        {
            continue;
        }
    }
}

server_t::safe_watermark_t::operator gaia_txn_id_t() const
{
    return gaia_txn_id_t(m_safe_ts);
}
