/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine.hpp"
#include "storage_engine_server.hpp"
#include "gaia_hash_map_server.hpp"
#include "gaia_ptr_server.hpp"

using namespace gaia::common;
using namespace gaia::db;

gaia_ptr_server::gaia_ptr_server(const gaia_id_t id, const size_t size)
    : row_id(0) {
    se_base::hash_node* hash_node = gaia_hash_map_server::insert(id);
    hash_node->row_id = row_id = server::allocate_row_id(server::s_shared_offsets);
    server::allocate_object(row_id, size, server::s_shared_offsets);
}

gaia_ptr_server::object* gaia_ptr_server::to_ptr() const {
    if (server::s_shared_offsets == nullptr) {
        // Although we don't open up explicit transactions during recovery.
        // Reuse, exception to indicate that offsets isn't initialized.
        throw transaction_not_open();
    }

    return row_id && (*server::s_shared_offsets)[row_id]
        ? reinterpret_cast<gaia_ptr_server::object*>(server::s_data->objects + (*server::s_shared_offsets)[row_id])
        : nullptr;
}
