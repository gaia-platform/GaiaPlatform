/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

inline bool client_t::is_session_open()
{
    return (s_session_socket != -1);
}

inline bool client_t::is_transaction_open()
{
    return (s_private_locators.is_set());
}

inline void client_t::set_commit_trigger(triggers::commit_trigger_fn trigger_fn)
{
    s_txn_commit_trigger = trigger_fn;
}

inline int client_t::get_session_socket_for_txn()
{
    return s_session_socket;
}

inline bool client_t::is_valid_event(common::gaia_type_t type)
{
    return (s_txn_commit_trigger && !is_system_object(type));
}

inline void client_t::verify_txn_active()
{
    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }
}

inline void client_t::verify_no_txn()
{
    if (is_transaction_open())
    {
        throw transaction_in_progress_internal();
    }
}

inline void client_t::verify_session_active()
{
    if (s_session_socket == -1)
    {
        throw no_open_session_internal();
    }
}

inline void client_t::verify_no_session()
{
    if (s_session_socket != -1)
    {
        throw session_exists_internal();
    }
}

inline void client_t::txn_log(
    gaia_locator_t locator,
    gaia_offset_t old_offset,
    gaia_offset_t new_offset)
{
    ASSERT_PRECONDITION(s_log.is_set(), "Transaction log must be mapped!");

    // We never allocate more than `c_max_log_records` records in the log.
    if (s_log.data()->record_count == c_max_log_records)
    {
        throw transaction_object_limit_exceeded_internal();
    }

    // Initialize the new record and increment the record count.
    auto& lr = s_log.data()->log_records[s_log.data()->record_count++];
    lr.locator = locator;
    lr.old_offset = old_offset;
    lr.new_offset = new_offset;
}

// This generator wraps a socket which reads a stream of values of `T_element_type` from the server.
template <typename T_element_type>
std::function<std::optional<T_element_type>()>
client_t::get_stream_generator_for_socket(std::shared_ptr<int> stream_socket_ptr)
{
    // Verify that the socket is the correct type for the semantics we assume.
    common::check_socket_type(*stream_socket_ptr, SOCK_SEQPACKET);

    // Currently, we associate a cursor with a snapshot view, i.e., a transaction.
    verify_txn_active();
    gaia_txn_id_t owning_txn_id = s_txn_id;

    // The userspace buffer that we use to receive a batch datagram message.
    std::vector<T_element_type> batch_buffer;

    // The definition of the generator we return.
    return [stream_socket_ptr, owning_txn_id, batch_buffer]() mutable -> std::optional<T_element_type> {
        // We shouldn't be called again after we received EOF from the server.
        ASSERT_INVARIANT(*stream_socket_ptr != -1, c_message_stream_socket_is_invalid);

        // The cursor should only be called from within the scope of its owning transaction.
        ASSERT_INVARIANT(s_txn_id == owning_txn_id, "Cursor was not called from the scope of its own transaction!");

        // If buffer is empty, block until a new batch is available.
        if (batch_buffer.size() == 0)
        {
            // Get the datagram size, and grow the buffer if necessary.
            // This decouples the client from the server (i.e., client
            // doesn't need to know server batch size), at the expense
            // of an extra system call per batch.
            // We set MSG_PEEK to avoid reading the datagram into our buffer,
            // and we set MSG_TRUNC to return the actual buffer size needed.
            ssize_t datagram_size = ::recv(*stream_socket_ptr, nullptr, 0, MSG_PEEK | MSG_TRUNC);
            if (datagram_size == -1)
            {
                common::throw_system_error("recv(MSG_PEEK) failed!");
            }

            if (datagram_size == 0)
            {
                // We received EOF from the server, so close the socket and tell
                // the caller to stop iteration.
                common::close_fd(*stream_socket_ptr);
                return std::nullopt;
            }

            // The datagram size must be an integer multiple of our datum size.
            ASSERT_INVARIANT(datagram_size % sizeof(T_element_type) == 0, c_message_unexpected_datagram_size);

            // Align the end of the buffer to the expected number of elements.
            // Per the C++ standard, this will never reduce capacity.
            batch_buffer.resize(datagram_size / sizeof(T_element_type));

            // Get the actual data.
            // This is a nonblocking read, because the previous blocking
            // read will not return until data is available.
            ssize_t bytes_read = ::recv(*stream_socket_ptr, batch_buffer.data(), datagram_size, MSG_DONTWAIT);
            if (bytes_read == -1)
            {
                // Per above, we should never have to block here.
                ASSERT_INVARIANT(errno != EAGAIN && errno != EWOULDBLOCK, "Unexpected errno value!");
                common::throw_system_error("recv() failed!");
            }

            // Because our buffer is exactly the same size as the datagram,
            // we should read exactly the number of bytes in the datagram.
            ASSERT_INVARIANT(bytes_read == datagram_size, "Bytes read differ from datagram size!");
        }

        // At this point we know our buffer is non-empty.
        ASSERT_INVARIANT(batch_buffer.size() > 0, c_message_empty_batch_buffer_detected);

        // Loop through the buffer and return entries in FIFO order
        // (the server reversed the original buffer before sending).
        T_element_type next_value = batch_buffer.back();
        batch_buffer.pop_back();

        return next_value;
    };
}
