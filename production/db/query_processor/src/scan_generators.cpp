/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "scan_generators.hpp"

#include "gaia_internal/common/scope_guard.hpp"

#include "client_messenger.hpp"
#include "db_client.hpp"

using namespace gaia::common;
using namespace gaia::db::messages;
using namespace gaia::common;
using namespace flatbuffers;
using namespace gaia::common::scope_guard;

namespace gaia
{
namespace db
{
namespace query_processor
{
namespace scan
{

std::shared_ptr<gaia::common::iterators::generator_t<index::index_record_t>>
scan_generator_t::get_record_generator_for_index(
    common::gaia_id_t index_id,
    gaia_txn_id_t txn_id,
    std::shared_ptr<query_processor::scan::index_predicate_t> predicate)
{
    int stream_socket = get_record_cursor_socket_for_index(index_id, txn_id, predicate);
    auto cleanup_stream_socket = make_scope_guard([&]() {
        close_fd(stream_socket);
    });

    auto record_generator = client_t::get_stream_generator_for_socket<index::index_record_t>(stream_socket);
    cleanup_stream_socket.dismiss();

    return std::make_shared<gaia::common::iterators::generator_t<index::index_record_t>>(record_generator);
}

int scan_generator_t::get_record_cursor_socket_for_index(
    common::gaia_id_t index_id,
    gaia_txn_id_t txn_id,
    std::shared_ptr<query_processor::scan::index_predicate_t> predicate)
{
    FlatBufferBuilder builder;

    flatbuffers::Offset<void> serialized_predicate = 0;

    if (predicate)
    {
        serialized_predicate = predicate->as_query(builder);
    }

    auto info_builder = index_scan_info_tBuilder(builder);
    info_builder.add_txn_id(txn_id);
    info_builder.add_index_id(index_id);

    if (predicate)
    {
        info_builder.add_query(serialized_predicate);
        info_builder.add_query_type(predicate->query_type());
    }

    auto index_scan_info = info_builder.Finish();

    auto client_request = Createclient_request_t(
        builder,
        session_event_t::REQUEST_STREAM,
        request_data_t::index_scan,
        index_scan_info.Union());
    auto message = Createmessage_t(builder, any_message_t::request, client_request.Union());
    builder.Finish(message);
    client_messenger_t client_messenger;
    client_messenger.send_and_receive(client_t::get_session_socket_for_txn(), nullptr, 0, builder, 1);

    int stream_socket = client_messenger.received_fd(client_messenger_t::c_index_stream_socket);
    auto cleanup_stream_socket = make_scope_guard([&]() {
        close_fd(stream_socket);
    });

    const session_event_t event = client_messenger.server_reply()->event();
    ASSERT_INVARIANT(event == session_event_t::REQUEST_STREAM, c_message_unexpected_event_received);

    // Check that our stream socket is blocking (because we need to perform blocking reads).
    ASSERT_INVARIANT(!is_non_blocking(stream_socket), "Stream socket is not set to blocking!");

    cleanup_stream_socket.dismiss();
    return stream_socket;
}
} // namespace scan
} // namespace query_processor
} // namespace db
} // namespace gaia
