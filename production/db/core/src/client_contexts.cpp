/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "client_contexts.hpp"

#include "index.hpp"

namespace gaia
{
namespace db
{

void client_transaction_context_t::clear()
{
    // Clear the local indexes.
    for (const auto& index : local_indexes)
    {
        index.second->clear();
    }
}

void client_session_context_t::clear()
{
    // This will gracefully shut down the server-side session thread
    // and all other threads that session thread owns.
    common::close_fd(session_socket);

    // If we own a chunk, we need to release it to the memory manager to be
    // reused when it is empty.
    // NB: The chunk manager could be uninitialized if this session never made
    // any allocations.
    if (chunk_manager.initialized())
    {
        // Get the session's chunk version for safe deallocation.
        chunk_version_t version = chunk_manager.get_version();

        // Now retire the chunk.
        chunk_manager.retire_chunk(version);

        // Release ownership of the chunk.
        chunk_manager.release();
    }
}

} // namespace db
} // namespace gaia
