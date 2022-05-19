/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "server_contexts.hpp"

#include "gaia_internal/common/socket_helpers.hpp"

namespace gaia
{
namespace db
{

void server_transaction_context_t::clear()
{
    local_snapshot_locators.close();
}

void server_session_context_t::clear()
{
    // We can rely on close_fd() to perform the equivalent of
    // shutdown(SHUT_RDWR), because we hold the only fd pointing to this
    // socket. That should allow the client to read EOF if they're in a
    // blocking read and exit gracefully. (If they try to write to the
    // socket after we've closed our end, they'll receive EPIPE.) We don't
    // want to try to read any pending data from the client, because we're
    // trying to shut down as quickly as possible.
    common::close_fd(session_socket);
}

} // namespace db
} // namespace gaia
