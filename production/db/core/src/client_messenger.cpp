////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "client_messenger.hpp"

#include "gaia/common.hpp"

#include <gaia_spdlog/fmt/fmt.h>

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
    bool is_in_bulk_fd_retrieval_mode = false;
    receive_server_reply(expected_count_received_fds, is_in_bulk_fd_retrieval_mode);

    // Deserialize the server message.
    deserialize_server_message();
}

void client_messenger_t::receive_server_reply(
    size_t expected_count_received_fds,
    bool is_in_bulk_fd_retrieval_mode)
{
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
    ASSERT_INVARIANT(bytes_read > 0, "Failed to read message!");

    if (is_in_bulk_fd_retrieval_mode)
    {
        // In this mode, the fds are attached to a dummy 1-byte datagram.
        ASSERT_INVARIANT(bytes_read == 1, "Unexpected message size!");

        ASSERT_INVARIANT(m_count_received_fds > 0, "No fds were received!");
    }
    else if (m_count_received_fds != expected_count_received_fds)
    {
        ASSERT_UNREACHABLE(
            gaia_fmt::format(
                "Expected {} fds, but received {} fds!",
                expected_count_received_fds, m_count_received_fds)
                .c_str());
    }

    for (size_t fd_index = 0; fd_index < m_count_received_fds; fd_index++)
    {
        int current_fd = m_received_fds[fd_index];
        if (current_fd == -1)
        {
            ASSERT_INVARIANT(
                current_fd != -1,
                gaia_fmt::format("The fd received at index {} is invalid!", fd_index).c_str());
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

int client_messenger_t::received_fd(size_t fd_index)
{
    ASSERT_PRECONDITION(
        fd_index < m_count_received_fds,
        "Attempt to access fd is outside the bounds of the fd array!");

    return m_received_fds[fd_index];
}

} // namespace db
} // namespace gaia
