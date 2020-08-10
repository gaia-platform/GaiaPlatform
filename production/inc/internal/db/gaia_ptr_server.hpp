/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>

#include "retail_assert.hpp"
#include "gaia_db.hpp"
#include "gaia_ptr.hpp"

using namespace gaia::common;

namespace gaia {
namespace db {

// Class used for recovery purposes only.
class gaia_ptr_server : public gaia_ptr {
    friend class rdb_object_converter_util;
    friend class rdb_wrapper;
   public:
    static gaia_ptr_server create(
        gaia_id_t id,
        gaia_type_t type,
        size_t num_refs,
        size_t data_size,
        const void* data,
        bool log_updates = true) {
        size_t refs_len = num_refs * sizeof(gaia_id_t);
        size_t total_len = data_size + refs_len;
        object* obj_ptr;
        gaia_ptr_server obj(id, total_len + sizeof(object));
        obj_ptr = obj.to_ptr();
        obj_ptr->id = id;
        obj_ptr->type = type;
        obj_ptr->num_references = num_refs;
        if (num_refs) {
            memset(obj_ptr->payload, 0, refs_len);
        }
        obj_ptr->payload_size = total_len;
        memcpy(obj_ptr->payload + refs_len, data, data_size);
        return obj;
    }
    gaia_ptr_server(const gaia_id_t id, const size_t size);
    object* to_ptr() const;
   private:
    int64_t row_id;
};

}  // namespace db
}  // namespace gaia
