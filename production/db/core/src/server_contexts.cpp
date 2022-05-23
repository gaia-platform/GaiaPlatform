////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include "server_contexts.hpp"

#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/common/fd_helpers.hpp"
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
    if (session_shutdown_eventfd != -1)
    {
        // Signal all session-owned threads to terminate.
        common::signal_eventfd_multiple_threads(session_shutdown_eventfd);

        // Wait for all session-owned threads to terminate.
        for (auto& thread : session_owned_threads)
        {
            ASSERT_INVARIANT(thread.joinable(), "Thread must be joinable!");
            thread.join();
        }

        // All session-owned threads have received the session shutdown
        // notification, so we can close the eventfd.
        common::close_fd(session_shutdown_eventfd);
    }

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
