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

server_t::safe_fd_from_ts_t::safe_fd_from_ts_t(gaia_txn_id_t commit_ts, bool auto_close_fd)
    : m_auto_close_fd(auto_close_fd)
{
    ASSERT_PRECONDITION(
        transactions::txn_metadata_t::is_commit_ts(commit_ts),
        "You must initialize safe_fd_from_ts_t from a valid commit_ts!");

    // If the log fd was invalidated, it is either closed or soon will be
    // closed, and therefore we cannot use it. We return early if the fd has
    // already been invalidated to avoid the dup(2) call.
    int log_fd = transactions::txn_metadata_t::get_txn_log_fd(commit_ts);
    if (log_fd == -1)
    {
        throw invalid_log_fd(commit_ts);
    }

    // To avoid races from the log fd being closed out from under us by a
    // concurrent updater, we first dup(2) the fd, and then if the dup was
    // successful, test the log fd entry again to ensure we aren't reusing a
    // closed log fd. If the log fd was invalidated, then we need to close our
    // dup fd and return false.
    try
    {
        m_local_log_fd = common::duplicate_fd(log_fd);
    }
    catch (const common::system_error& e)
    {
        // The log fd was already closed by another thread (after being
        // invalidated).
        // NB: This does not handle the case of the log fd being closed and then
        // reused before our dup(2) call. That case is handled below, where we
        // check for invalidation.
        if (e.get_errno() == EBADF)
        {
            // The log fd must have been invalidated before it was closed.
            ASSERT_INVARIANT(
                transactions::txn_metadata_t::get_txn_log_fd(commit_ts) == -1,
                "log fd was closed without being invalidated!");

            // We lost the race because the log fd was invalidated and closed
            // after our check.
            throw invalid_log_fd(commit_ts);
        }
        else
        {
            throw;
        }
    }

    // If we were able to duplicate the log fd, check to be sure it wasn't
    // invalidated (and thus possibly closed and reused before the call to
    // dup(2)), so we know we aren't reusing a closed fd.
    if (transactions::txn_metadata_t::get_txn_log_fd(commit_ts) == -1)
    {
        // If we got here, we must have a valid dup fd.
        ASSERT_INVARIANT(
            common::is_fd_valid(m_local_log_fd),
            "fd should be valid if dup() succeeded!");

        // We need to close the duplicated fd because the original fd might have
        // been reused and we would leak it otherwise (because the destructor
        // isn't called if the constructor throws).
        common::close_fd(m_local_log_fd);
        throw invalid_log_fd(commit_ts);
    }
}

server_t::safe_fd_from_ts_t::~safe_fd_from_ts_t()
{
    // Ensure we close the dup log fd. If the original log fd was closed already
    // (indicated by get_txn_log_fd() returning -1), this will free the
    // shared-memory object referenced by the fd.
    if (m_auto_close_fd)
    {
        // If the constructor fails, this will handle an invalid fd (-1)
        // correctly.
        common::close_fd(m_local_log_fd);
    }
}

int server_t::safe_fd_from_ts_t::get_fd() const
{
    return m_local_log_fd;
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
    if (m_ts == c_invalid_gaia_txn_id)
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
            // NB: It is legal to access safe_ts after moving from it,
            // because the standard specifies that "Moved-from objects
            // shall be placed in a valid but unspecified state."
            ASSERT_POSTCONDITION(
                static_cast<gaia_txn_id_t>(safe_ts) == c_invalid_gaia_txn_id,
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
