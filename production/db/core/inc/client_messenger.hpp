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

class client_messenger_t
{
public:
    client_messenger_t() = delete;

    explicit client_messenger_t(size_t expected_count_received_fds);

    ~client_messenger_t();

    void send_and_receive(
        int socket,
        int* fds_to_send,
        size_t count_fds_to_send,
        const flatbuffers::FlatBufferBuilder& builder);

    inline int* get_received_fds()
    {
        return m_received_fds;
    }

    inline const messages::server_reply_t* get_server_reply()
    {
        return m_server_reply;
    }

protected:
    uint8_t m_message_buffer[common::c_max_msg_size] = {0};
    int* m_received_fds;
    size_t m_expected_count_received_fds;
    const messages::server_reply_t* m_server_reply;
};

} // namespace db
} // namespace gaia
