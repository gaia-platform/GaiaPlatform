///////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////


#include "storage_engine.hpp"

using namespace gaia::db;

class gaia_ptr_mock : public gaia_ptr<gaia_se_node>
{
public:
    gaia_ptr_mock() {}
    gaia_ptr_mock(const gaia_ptr<gaia_se_node>& other) : gaia_ptr(other) {}

    gaia_ptr_mock& update_payload(size_t num_references, gaia_id_t* references, size_t payload_size, const void* payload)
    {
        int32_t total_len = payload_size + (num_references+1)*sizeof(gaia_id_t);
        uint8_t* b = new uint8_t[total_len];
        memcpy(b, &num_references, sizeof(gaia_id_t));
        if (num_references) {
            memcpy(b+sizeof(gaia_id_t), references, num_references*sizeof(gaia_id_t));
        }
        memcpy(b+sizeof(gaia_id_t)*(num_references+1), payload, payload_size);
        gaia_ptr<gaia_se_node>::update_payload(total_len, b);
        delete[] b;
        return *this;
    }

};

struct gaia_se_node_mock : public gaia_se_node
{
    gaia_se_node_mock(gaia_ptr<gaia_se_node> node) : se_node(node) {}

    static gaia_ptr_mock create (
        gaia_id_t id,
        gaia_type_t type,
        size_t num_refs,
        const gaia_id_t* references,
        size_t payload_size,
        const void* payload,
        bool log_updates = true
    )
    {
        // mock storage of references:
        //   number of references : 8 bytes
        //   array of references : (number of references) * 8
        //   original payload
        int32_t total_len = payload_size + (num_refs+1)*sizeof(gaia_id_t);
        uint8_t* b = new uint8_t[total_len];
        memcpy(b, &num_refs, sizeof(gaia_id_t));
        if (references) {
            memcpy(b+sizeof(gaia_id_t), references, num_refs*sizeof(gaia_id_t));
        }
        else {
            memset(b+sizeof(gaia_id_t), 0, num_refs*sizeof(gaia_id_t));
        }
        memcpy(b+sizeof(gaia_id_t)*(num_refs+1), payload, payload_size);
        auto new_node = gaia_se_node::create(id, type, total_len, b, log_updates);
        delete[] b;
        return gaia_ptr_mock(new_node);
    }

    static gaia_ptr_mock open(gaia_id_t id) {
        return gaia_se_node::open(id);
    }

    gaia_ptr<gaia_se_node> se_node;
};
