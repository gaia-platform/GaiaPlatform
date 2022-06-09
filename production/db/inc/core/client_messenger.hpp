////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <flatbuffers/flatbuffers.h>

#include "gaia/common.hpp"

#include "gaia_internal/common/socket_helpers.hpp"

#include "messages_generated.h"

namespace gaia
{
namespace db
{

static constexpr char c_message_unexpected_event_received[] = "Unexpected event received!";
static constexpr char c_message_stream_socket_is_invalid[] = "Stream socket is invalid!";
static constexpr char c_message_unexpected_datagram_size[] = "Unexpected datagram size!";
static constexpr char c_message_empty_batch_buffer_detected[] = "Empty batch buffer detected!";

// Instances of this class are used by the client to communicate with the server.
//
// The send_and_receive() method enables sending a request to the server,
// receiving the server response, and performing standard validations on that response.
//
// The receive_server_reply() method is used in scenarios where additional server
// replies need to be processed ("bulk fd retrieval mode").
//
// The fds returned by the server, if any, are surfaced through count_received_fds() and received_fd().
//
// The server reply is surfaced through server_reply().
class client_messenger_t
{
public:
    // Constants for accessing fds in arrays of fds, for different server responses.
    static const int c_index_stream_socket = 0;

public:
    client_messenger_t() = default;
    ~client_messenger_t() = default;

    void send_and_receive(
        int socket,
        int* fds_to_send,
        size_t count_fds_to_send,
        const flatbuffers::FlatBufferBuilder& builder,
        size_t expected_count_received_fds = 0);

    void receive_server_reply(
        size_t expected_count_received_fds = common::c_max_fd_count,
        bool is_in_bulk_fd_retrieval_mode = true);

    size_t count_received_fds()
    {
        return m_count_received_fds;
    }

    int received_fd(size_t fd_index);

    inline const messages::server_reply_t* server_reply()
    {
        return m_server_reply;
    }

protected:
    void deserialize_server_message();

    void clear();

protected:
    int m_socket = -1;
    int m_received_fds[common::c_max_fd_count]{-1};
    size_t m_count_received_fds = 0;
    uint8_t m_message_buffer[common::c_max_msg_size_in_bytes]{0};
    const messages::server_reply_t* m_server_reply = nullptr;
};

} // namespace db
} // namespace gaia
