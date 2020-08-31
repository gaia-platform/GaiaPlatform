/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine_client.hpp"
#include "gaia_hash_map.hpp"
#include "gaia_ptr.hpp"
#include "payload_diff.hpp"
#include "field_list.hpp"
#include "triggers.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;

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

    if (client::is_valid_event(new_this->type)) {
        client::s_events.push_back(trigger_event_t {event_type_t::row_insert, new_this->type, new_this->id, empty_position_list});
    }

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

    if (client::is_valid_event(new_this->type)) {
        auto old_data = (const uint8_t*) old_this->payload;

        const uint8_t* old_data_payload; 

        if (old_this->num_references) {
            old_data_payload = old_data + sizeof(gaia_id_t) * old_this->num_references;
        }
        // Compute field diff
        field_position_list_t position_list;
        gaia::db::types::compute_payload_diff(new_this->type, old_data_payload, (const uint8_t*) data, &position_list);
        client::s_events.push_back(trigger_event_t {event_type_t::row_update, new_this->type, new_this->id, position_list});
    }

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

    // Writing to log will be skipped for recovery.
    if (log_updates) {
        client::tx_log(row_id, 0, to_offset());
    }
}

void gaia_ptr::allocate(const size_t size) {
    client::allocate_object(row_id, size);
}

void gaia_ptr::create_insert_trigger(gaia_type_t type, gaia_id_t id) {
    if (client::is_valid_event(type)) {
        client::s_events.push_back(trigger_event_t {event_type_t::row_insert, type, id, empty_position_list});
    }
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
    if (client::is_valid_event(to_ptr()->type)) {
        client::s_events.push_back(trigger_event_t {event_type_t::row_delete, to_ptr()->type, to_ptr()->id, empty_position_list});
    }
    (*client::s_offsets)[row_id] = 0;
    row_id = 0;
}
