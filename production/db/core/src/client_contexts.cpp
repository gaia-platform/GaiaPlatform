////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

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

client_session_context_t::client_session_context_t()
{
    data_mappings.push_back({data_mapping_t::index_t::locators, &private_locators, c_gaia_mem_locators_prefix});
    data_mappings.push_back({data_mapping_t::index_t::counters, &shared_counters, c_gaia_mem_counters_prefix});
    data_mappings.push_back({data_mapping_t::index_t::data, &shared_data, c_gaia_mem_data_prefix});
    data_mappings.push_back({data_mapping_t::index_t::logs, &shared_logs, c_gaia_mem_logs_prefix});
    data_mappings.push_back({data_mapping_t::index_t::id_index, &shared_id_index, c_gaia_mem_id_index_prefix});
    data_mappings.push_back({data_mapping_t::index_t::type_index, &shared_type_index, c_gaia_mem_type_index_prefix});
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

    // We closed our original fds for these data segments, so we only need to unmap them.
    data_mapping_t::close(data_mappings);
}

} // namespace db
} // namespace gaia
