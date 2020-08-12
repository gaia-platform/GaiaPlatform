/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_hash_map_base.hpp"
#include "storage_engine.hpp"
#include "storage_engine_client.hpp"

namespace gaia {
namespace db {

using namespace common;

class gaia_hash_map : public gaia_hash_map_base<gaia_hash_map> {
    friend class client;
   public:  
    static se_base::hash_node* get_hash_node(int64_t offset) {
        return client::s_data->hash_nodes + offset;
    }

    static bool locator_exists(int64_t offset) {
        return (*client::s_offsets)[offset];
    }

    static bool check_no_active_transaction() {
        return *client::s_offsets == nullptr;
    }

    static int64_t* get_hash_node_count() {
        return &client::s_data->hash_node_count;
    }
};

}  // namespace db
}  // namespace gaiaF
