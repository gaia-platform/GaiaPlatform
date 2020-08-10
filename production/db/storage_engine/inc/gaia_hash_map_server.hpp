/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "storage_engine.hpp"
#include "gaia_hash_map.hpp"
#include "storage_engine_server.hpp"

namespace gaia {
namespace db {

using namespace common;

// This class interacts with server mmap'd objects for Recovery purposes.
// It doesn't matter if the server or the client process calls these methods as the correct
// data will me mmapped.
class gaia_hash_map_server : public gaia_hash_map {
    friend class server;

    protected:
    static se_base::hash_node* get_hash_node(int64_t offset) {
        return server::s_data->hash_nodes + offset;
    }

    static bool locator_exists(int64_t offset) {
        return (*server::s_shared_offsets)[offset];
    }

    static bool check_no_active_transaction() {
        return *server::s_shared_offsets == nullptr;
    }

    static int64_t* get_hash_node_count() {
        return &server::s_data->hash_node_count;
    }
};

}  // namespace db
}  // namespace gaia
