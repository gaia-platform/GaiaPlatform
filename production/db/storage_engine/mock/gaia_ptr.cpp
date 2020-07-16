/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine_client.hpp"
#include "gaia_hash_map.hpp"
#include "gaia_ptr.hpp"

using namespace gaia::common;
using namespace gaia::db;

gaia_id_t gaia_ptr::generate_id() {
    return client::generate_id();
}

gaia_ptr& gaia_ptr::clone() {
    auto old_this = to_ptr();
    auto old_offset = to_offset();
    auto new_size = sizeof(gaia_ptr::object) + old_this->payload_size;

    allocate(new_size);
    auto new_this = to_ptr();

    memcpy(new_this, old_this, new_size);

    client::tx_log(row_id, old_offset, to_offset());

    return *this;
}

gaia_ptr& gaia_ptr::update_payload(size_t data_size, const void* data) {
    auto old_this = to_ptr();
    auto old_offset = to_offset();

    int32_t ref_len = old_this->num_references * sizeof(gaia_id_t);
    int32_t total_len = data_size + ref_len;
    allocate(sizeof(gaia_ptr::object) + total_len);

    auto new_this = to_ptr();

    memcpy(new_this, old_this, sizeof(gaia_ptr::object));
    new_this->payload_size = total_len;
    if (old_this->num_references) {
        memcpy(new_this->payload, old_this->payload, ref_len);
    }
    new_this->num_references = old_this->num_references;
    memcpy(new_this->payload + ref_len, data, data_size);

    client::tx_log(row_id, old_offset, to_offset());

    return *this;
}

gaia_ptr::gaia_ptr(const gaia_id_t id) {
    row_id = gaia_hash_map::find(id);
}

gaia_ptr::gaia_ptr(const gaia_id_t id, const size_t size, bool log_updates)
    : row_id(0) {
    se_base::hash_node* hash_node = gaia_hash_map::insert(id);
    hash_node->row_id = row_id = client::allocate_row_id();
    client::allocate_object(row_id, size);

    // writing to log will be skipped for recovery
    if (log_updates) {
        client::tx_log(row_id, 0, to_offset());
    }
}

void gaia_ptr::allocate(const size_t size) {
    client::allocate_object(row_id, size);
}

gaia_ptr::object* gaia_ptr::to_ptr() const {
    client::verify_tx_active();

    return row_id && (*client::s_offsets)[row_id]
               ? reinterpret_cast<gaia_ptr::object*>(client::s_data->objects + (*client::s_offsets)[row_id])
               : nullptr;
}

int64_t gaia_ptr::to_offset() const {
    client::verify_tx_active();

    return row_id
               ? (*client::s_offsets)[row_id]
               : 0;
}

void gaia_ptr::find_next(gaia_type_t type) {
    // search for rows of this type within the range of used slots
    while (++row_id && row_id < client::s_data->row_id_count + 1) {
        if (is(type)) {
            return;
        }
    }
    row_id = 0;
}

void gaia_ptr::reset() {
    client::tx_log(row_id, to_offset(), 0);
    (*client::s_offsets)[row_id] = 0;
    row_id = 0;
}
