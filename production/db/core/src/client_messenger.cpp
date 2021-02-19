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

client_messenger_t::client_messenger_t(size_t expected_count_received_fds)
{
    m_expected_count_received_fds = expected_count_received_fds;
    m_received_fds = (m_expected_count_received_fds > 0) ? new int[m_expected_count_received_fds] : nullptr;
}

client_messenger_t::~client_messenger_t()
{
    if (m_received_fds != nullptr)
    {
        delete[] m_received_fds;
        m_received_fds = nullptr;
    }
}

void client_messenger_t::send_and_receive(
    int socket,
    int* fds_to_send,
    size_t count_fds_to_send,
    const flatbuffers::FlatBufferBuilder& builder)
{
    // Send our message.
    send_msg_with_fds(
        socket,
        fds_to_send,
        count_fds_to_send,
        builder.GetBufferPointer(),
        builder.GetSize());

    // Receive server reply,
    size_t count_received_fds = m_expected_count_received_fds;
    size_t bytes_read = recv_msg_with_fds(
        socket,
        m_received_fds,
        (m_expected_count_received_fds > 0) ? &count_received_fds : nullptr,
        m_message_buffer,
        sizeof(m_message_buffer));

    // Sanity checks.
    retail_assert(bytes_read > 0, "Failed to read message!");

    std::stringstream message_stream;
    message_stream
        << "Expected " << m_expected_count_received_fds
        << " fds, but received " << count_received_fds << " fds!";
    retail_assert(count_received_fds == m_expected_count_received_fds, message_stream.str());

    for (size_t index_fd = 0; index_fd < m_expected_count_received_fds; index_fd++)
    {
        // Reset the message stream.
        message_stream.str(std::string());

        message_stream << "The fd received at index " << index_fd << " is invalid!";
        retail_assert(m_received_fds[index_fd] != -1, message_stream.str());
    }

    // Deserialize the server message.
    const message_t* message = Getmessage_t(m_message_buffer);
    m_server_reply = message->msg_as_reply();
}

int client_messenger_t::get_received_fd(size_t index_fd)
{
    retail_assert(
        m_received_fds,
        "Attempt to access fd when no fd array exists!");
    retail_assert(
        index_fd < m_expected_count_received_fds,
        "Attempt to access fd is outside the bounds of the fd array!");

    return m_received_fds[index_fd];
}

} // namespace db
} // namespace gaia
