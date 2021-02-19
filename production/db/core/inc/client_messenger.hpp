/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <flatbuffers/flatbuffers.h>

#include "gaia/common.hpp"

#include "gaia_internal/common/socket_helpers.hpp"

#include "messages_generated.h"

namespace gaia
{
namespace db
{

// Instances of this class are used to communicate with the server.
//
// The send_and_receive() method enables sending a request to the server,
// receiving the server response, and performing standard validations on that response.
//
// The fds returned by the server, if any, are surfaced through get_received_fd().
// The server reply is surfaced through get_server_reply().
class client_messenger_t
{
public:
    explicit client_messenger_t(size_t expected_count_received_fds = 0);

    ~client_messenger_t();

    void send_and_receive(
        int socket,
        int* fds_to_send,
        size_t count_fds_to_send,
        const flatbuffers::FlatBufferBuilder& builder);

    int get_received_fd(size_t index_fd);

    inline const messages::server_reply_t* get_server_reply()
    {
        return m_server_reply;
    }

protected:
    size_t m_expected_count_received_fds = 0;
    int* m_received_fds = nullptr;
    uint8_t m_message_buffer[common::c_max_msg_size] = {0};
    const messages::server_reply_t* m_server_reply = nullptr;
};

} // namespace db
} // namespace gaia
