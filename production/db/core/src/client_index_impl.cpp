/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "client_index_impl.hpp"

#include <unistd.h>

#include <flatbuffers/flatbuffers.h>

#include "messages_generated.h"
#include "db_client.hpp"

using namespace gaia::db::messages;
using namespace gaia::common;
using namespace flatbuffers;
using namespace scope_guard;

namespace gaia
{
namespace db
{
namespace index
{

int client_index_stream::get_record_cursor_socket_for_index(gaia_id_t index_id)
{
    FlatBufferBuilder builder;
    auto index_scan_info = Createindex_scan_info_t(builder, index_id);
    auto client_request = Createclient_request_t(builder, session_event_t::REQUEST_STREAM, request_data_t::index_scan, index_scan_info.Union());
    auto message = Createmessage_t(builder, any_message_t::request, client_request.Union());
    builder.Finish(message);

    return client::get_stream_socket(builder.GetBufferPointer(), builder.GetSize());
}

std::function<std::optional<index_record_t>()>
client_index_stream::get_record_generator_for_index(gaia_id_t index_id)
{
    int stream_socket = get_record_cursor_socket_for_index(index_id);
    auto cleanup_stream_socket = make_scope_guard([stream_socket]() {
        ::close(stream_socket);
    });
    auto record_generator = client::get_stream_generator_for_socket<index_record_t>(stream_socket);
    cleanup_stream_socket.dismiss();
    return record_generator;
}

index_stream_iterator_t client_index_stream::stream_iterator(common::gaia_id_t index_id)
{
    return index_stream_iterator_t(get_record_generator_for_index(index_id));
}

index_stream_range_t client_index_stream::stream_iterator_range(common::gaia_id_t index_id)
{
    return common::iterators::range_from_generator(get_record_generator_for_index(index_id));
}

} // namespace index
} // namespace db
} // namespace gaia
