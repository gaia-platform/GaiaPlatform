/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "storage_engine.hpp"
#include "storage_engine_client.hpp"
#include "gaia_hash_map.hpp"
#include "gaia_ptr.hpp"
#include "types.hpp"
#include "payload_diff.hpp"
#include "field_list.hpp"
#include "triggers.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;

gaia_id_t gaia_ptr::generate_id() {
    return client::generate_id(client::s_data);
}

gaia_ptr& gaia_ptr::clone() {
    auto old_this = to_ptr();
    auto old_offset = to_offset();
    auto new_size = sizeof(gaia_se_object_t) + old_this->payload_size;
    allocate(new_size);
    auto new_this = to_ptr();

    memcpy(new_this, old_this, new_size);

    client::tx_log(row_id, old_offset, to_offset(), gaia_operation_t::clone);

    if (client::is_valid_event(new_this->type)) {
        client::s_events.push_back(trigger_event_t {event_type_t::row_insert, new_this->type, new_this->id, empty_position_list});
    }

    return *this;
}

gaia_ptr& gaia_ptr::update_payload(size_t data_size, const void* data) {
    auto old_this = to_ptr();
    auto old_offset = to_offset();

    size_t ref_len = old_this->num_references * sizeof(gaia_id_t);
    size_t total_len = data_size + ref_len;
    allocate(sizeof(gaia_se_object_t) + total_len);

    auto new_this = to_ptr();

    memcpy(new_this, old_this, sizeof(gaia_se_object_t));
    new_this->payload_size = total_len;
    if (old_this->num_references) {
        memcpy(new_this->payload, old_this->payload, ref_len);
    }
    new_this->num_references = old_this->num_references;
    memcpy(new_this->payload + ref_len, data, data_size);

    client::tx_log(row_id, old_offset, to_offset(), gaia_operation_t::update);

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

gaia_ptr& gaia_ptr::update_references(vector<pair<uint64_t, gaia_id_t>> slot_id_pairs) {
    auto old_this = to_ptr();
    auto old_offset = to_offset();

    size_t total_size = sizeof(gaia_se_object_t) + old_this->payload_size;
    allocate(total_size);
    auto new_this = to_ptr();
    memcpy(new_this, old_this, total_size);

    for (auto slot_id_pair : slot_id_pairs) {
        references()[slot_id_pair.first] = slot_id_pair.second;
    }
    client::tx_log(row_id, old_offset, to_offset(), gaia_operation_t::update);

    return *this;
}

gaia_ptr::gaia_ptr(const gaia_id_t id) {
    row_id = gaia_hash_map::find(client::s_data, client::s_offsets, id);
}

gaia_ptr::gaia_ptr(const gaia_id_t id, const size_t size)
    : row_id(0) {
    hash_node* hash_node = gaia_hash_map::insert(client::s_data, client::s_offsets, id);
    hash_node->row_id = row_id = se_base::allocate_row_id(client::s_offsets, client::s_data);
    se_base::allocate_object(row_id, size, client::s_offsets, client::s_data);
    client::tx_log(row_id, 0, to_offset(), gaia_operation_t::create);
}

void gaia_ptr::allocate(const size_t size) {
    se_base::allocate_object(row_id, size, client::s_offsets, client::s_data);
}

void gaia_ptr::create_insert_trigger(gaia_type_t type, gaia_id_t id) {
    if (client::is_valid_event(type)) {
        client::s_events.push_back(trigger_event_t {event_type_t::row_insert, type, id, empty_position_list});
    }
}

gaia_se_object_t* gaia_ptr::to_ptr() const {
    client::verify_tx_active();

    return row_id && se_base::locator_exists(client::s_offsets, row_id)
        ? reinterpret_cast<gaia_se_object_t*>(client::s_data->objects + (*client::s_offsets)[row_id])
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
    client::tx_log(row_id, to_offset(), 0, gaia_operation_t::remove, to_ptr()->id);

    if (client::is_valid_event(to_ptr()->type)) {
        client::s_events.push_back(trigger_event_t {event_type_t::row_delete, to_ptr()->type, to_ptr()->id, empty_position_list});
    }
    (*client::s_offsets)[row_id] = 0;
    row_id = 0;
}
