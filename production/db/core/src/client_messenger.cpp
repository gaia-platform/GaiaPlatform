/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "client_messenger.hpp"

#include <sstream>

#include "gaia/common.hpp"

using namespace gaia::common;
using namespace gaia::db::messages;

namespace gaia
{
namespace db
{

void client_messenger_t::send_and_receive(
    int socket,
    int* fds_to_send,
    size_t count_fds_to_send,
    const flatbuffers::FlatBufferBuilder& builder,
    size_t expected_count_received_fds)
{
    m_socket = socket;

    // Send our message.
    send_msg_with_fds(
        m_socket,
        fds_to_send,
        count_fds_to_send,
        builder.GetBufferPointer(),
        builder.GetSize());

    // Receive server reply,
    receive_server_reply(expected_count_received_fds);

    // Deserialize the server message.
    deserialize_server_message();
}

void client_messenger_t::receive_server_reply(
    size_t expected_count_received_fds)
{
    // Special scenario when we're expecting a bunch of fds.
    // This is how we're called for retrieving log fds.
    bool is_in_bulk_fd_retrieval_mode = (expected_count_received_fds == common::c_max_fd_count);

    // Clear information that we may have read in previous calls.
    clear();

    // Read server response.
    m_count_received_fds = expected_count_received_fds;
    size_t bytes_read = recv_msg_with_fds(
        m_socket,
        (expected_count_received_fds > 0) ? m_received_fds : nullptr,
        (expected_count_received_fds > 0) ? &m_count_received_fds : nullptr,
        m_message_buffer,
        sizeof(m_message_buffer));

    // Sanity checks.
    retail_assert(bytes_read > 0, "Failed to read message!");

    if (is_in_bulk_fd_retrieval_mode)
    {
        // In this mode, the fds are attached to a dummy 1-byte datagram.
        retail_assert(bytes_read == 1, "Unexpected message size!");

        retail_assert(m_count_received_fds > 0, "No fds were received!");
    }
    else if (m_count_received_fds != expected_count_received_fds)
    {
        std::stringstream message_stream;
        message_stream
            << "Expected " << expected_count_received_fds
            << " fds, but received " << m_count_received_fds << " fds!";
        retail_assert(m_count_received_fds == expected_count_received_fds, message_stream.str());
    }

    for (size_t index_fd = 0; index_fd < m_count_received_fds; index_fd++)
    {
        int current_fd = m_received_fds[index_fd];
        if (current_fd == -1)
        {
            std::stringstream message_stream;
            message_stream << "The fd received at index " << index_fd << " is invalid!";
            retail_assert(current_fd != -1, message_stream.str());
        }
    }
}

void client_messenger_t::deserialize_server_message()
{
    const message_t* message = Getmessage_t(m_message_buffer);
    m_server_reply = message->msg_as_reply();
}

void client_messenger_t::clear()
{
    m_count_received_fds = 0;
    m_server_reply = nullptr;
}

int client_messenger_t::get_received_fd(size_t index_fd)
{
    retail_assert(
        index_fd < m_count_received_fds,
        "Attempt to access fd is outside the bounds of the fd array!");

    return m_received_fds[index_fd];
}

} // namespace db
} // namespace gaia
