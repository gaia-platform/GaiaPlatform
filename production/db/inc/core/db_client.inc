////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

bool client_t::is_session_open()
{
    return (s_session_context != nullptr && session_socket() != -1);
}

bool client_t::is_ping_session_open()
{
    return is_session_open() && session_options().session_type == session_type_t::ping;
}

bool client_t::is_ddl_session_open()
{
    return is_session_open() && session_options().session_type == session_type_t::ddl;
}

bool client_t::is_transaction_open()
{
    return (s_session_context && s_session_context->private_locators.is_set());
}

gaia_txn_id_t client_t::get_current_txn_id()
{
    return txn_id();
}

void client_t::set_commit_trigger(triggers::commit_trigger_fn trigger_fn)
{
    s_txn_commit_trigger = trigger_fn;
}

bool client_t::has_commit_trigger()
{
    return (s_txn_commit_trigger != nullptr);
}

int client_t::get_session_socket_for_txn()
{
    return session_socket();
}

bool client_t::is_valid_event(common::gaia_type_t type)
{
    return (s_txn_commit_trigger && !is_system_object(type));
}

void client_t::verify_txn_active()
{
    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }
}

void client_t::verify_no_txn()
{
    if (is_transaction_open())
    {
        throw transaction_in_progress_internal();
    }
}

void client_t::verify_session_active()
{
    if (s_session_context == nullptr || session_socket() == -1)
    {
        throw no_open_session_internal();
    }
}

void client_t::verify_no_session()
{
    if (s_session_context != nullptr && session_socket() != -1)
    {
        throw session_exists_internal();
    }
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
    gaia_txn_id_t owning_txn_id = txn_id();

    // The userspace buffer that we use to receive a batch datagram message.
    std::vector<T_element_type> batch_buffer;

    // The definition of the generator we return.
    return [stream_socket_ptr, owning_txn_id, batch_buffer]() mutable -> std::optional<T_element_type> {
        // We shouldn't be called again after we received EOF from the server.
        ASSERT_INVARIANT(*stream_socket_ptr != -1, c_message_stream_socket_is_invalid);

        // The cursor should only be called from within the scope of its owning transaction.
        ASSERT_INVARIANT(
            txn_id() == owning_txn_id,
            "Cursor was not called from the scope of its own transaction!");

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
                // We received EOF from the server, so tell the caller to stop iterating.
                // We also close the socket here to make it easier to detect further calls
                // into the iterator, which would now hit the earlier assertion.
                // This early close is also an optimization to allow the server to close
                // its cursor socket and terminate its cursor thread immediately
                // after the client detects the end of iteration.
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

        // REVIEW: uncomment this assert after index iterators no longer send invalid records.
        // ASSERT_POSTCONDITION(next_value.is_valid(), "The stream generator has produced an invalid element value!");

        return next_value;
    };
}

void client_t::log_event(
    triggers::event_type_t event_type, common::gaia_type_t type,
    common::gaia_id_t id, common::field_position_list_t changed_fields)
{
    DEBUG_ASSERT_PRECONDITION(
        txn_id().is_valid(), "Event can only be logged in a transaction!");
    events().emplace_back(event_type, type, id, changed_fields, txn_id());
}

gaia_txn_id_t client_t::txn_id()
{
    return s_session_context->txn_context->txn_id;
}

log_offset_t client_t::txn_log_offset()
{
    return s_session_context->txn_context->txn_log_offset;
}

std::vector<gaia::db::triggers::trigger_event_t>& client_t::events()
{
    return s_session_context->txn_context->events;
}

config::session_options_t& client_t::session_options()
{
    return s_session_context->session_options;
}

int client_t::session_socket()
{
    return s_session_context->session_socket;
}

mapped_data_t<locators_t>& client_t::private_locators()
{
    return s_session_context->private_locators;
}

mapped_data_t<data_t>& client_t::shared_data()
{
    return s_session_context->shared_data;
}

std::vector<data_mapping_t>& client_t::data_mappings()
{
    return s_session_context->data_mappings;
}
