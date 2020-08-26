/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>
#include <iostream>

#include "retail_assert.hpp"
#include "gaia_db.hpp"
#include "types.hpp"

using namespace gaia::common;

namespace gaia {
namespace db {

// Class used for recovery purposes only.
class gaia_ptr_server {
    friend class rdb_wrapper;
   public:
    static gaia_ptr_server create(
        gaia_id_t id,
        gaia_type_t type,
        uint64_t num_refs,
        uint64_t data_size,
        const void* data) {
        // cout << "[decode; id; type; num_refs; data_size; total]" << id << ":" << type <<":" << num_refs << ":" << data_size << endl << flush;
        // uint64_t refs_len = num_refs * sizeof(gaia_id_t);
        // uint64_t total_len = data_size + refs_len;
        gaia_ptr_server obj(id, data_size + sizeof(object));
        object* obj_ptr = obj.to_ptr();
        obj_ptr->id = id;
        obj_ptr->type = type;
        obj_ptr->num_references = num_refs;
        // if (num_refs) {
        //     memset(obj_ptr->payload, 0, refs_len);
        // }
        obj_ptr->payload_size = data_size;
        memcpy(obj_ptr->payload, data, data_size);
        return obj;
    }
    gaia_ptr_server(const gaia_id_t id, const uint64_t size);
    object* to_ptr() const;
   private:
    int64_t row_id;
};

}  // namespace db
}  // namespace gaia
