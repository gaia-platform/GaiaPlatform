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
#include "type_metadata.hpp"

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

void gaia_ptr::add_child_reference(gaia_id_t child_id, relationship_offset_t first_child) {
    auto parent_type = type();
    auto parent_metadata = type_registry_t::instance().get_or_create(parent_type);
    auto relation = parent_metadata.find_parent_relationship(first_child);

    if (!relation) {
        throw invalid_relation_offset_t(parent_type, first_child);
    }

    // CHECK TYPES

    auto child_ptr = gaia_ptr(child_id);

    if (!child_ptr) {
        throw invalid_node_id(child_id);
    }

    if (relation->parent_type != parent_type) {
        throw invalid_relation_type_t(first_child, parent_type, relation->parent_type);
    }

    if (relation->child_type != child_ptr.type()) {
        throw invalid_relation_type_t(first_child, child_ptr.type(), relation->child_type);
    }

    // CHECK CARDINALITY

    if (references()[first_child] != INVALID_GAIA_ID) {
        // this parent already has a child for this relation.
        // If the relation is one-to-one we fail.
        if (relation->cardinality == cardinality_t::one) {
            throw single_cardinality_violation_t(parent_type, first_child);
        }
    }

    // Note: we check only for parent under the assumption that the relational integrity
    // is preserved thus if there are no parent references there are no next_child either
    if (child_ptr.references()[relation->parent] != INVALID_GAIA_ID) {
        // ATM we don't allow a reference to be re-assigned on the fly.
        // Users need to explicitly delete the existing reference.
        // In future we may introduce flags/parameters that allow to do so.
        throw child_already_in_relation_t(child_ptr.type(), relation->parent);
    }

    // BUILD THE REFERENCES

    child_ptr.references()[relation->next_child] = references()[first_child];
    references()[first_child] = child_ptr.id();
    child_ptr.references()[relation->parent] = id();
}

void gaia_ptr::add_parent_reference(gaia_id_t parent_id, relationship_offset_t parent) {
    auto child_type = type();

    auto child_metadata = type_registry_t::instance().get_or_create(child_type);
    auto child_relation = child_metadata.find_child_relationship(parent);

    if (!child_relation) {
        throw invalid_relation_offset_t(child_type, parent);
    }

    auto parent_ptr = gaia_ptr(parent_id);

    if (!parent_ptr) {
        throw invalid_node_id(parent_ptr);
    }

    parent_ptr.add_child_reference(id(), child_relation->first_child);
}

void gaia_ptr::remove_child_reference(gaia_id_t child_id, relationship_offset_t first_child) {
    auto parent_type = type();
    auto parent_metadata = type_registry_t::instance().get_or_create(parent_type);
    auto relation = parent_metadata.find_parent_relationship(first_child);

    if (!relation) {
        throw invalid_relation_offset_t(parent_type, first_child);
    }

    // CHECK TYPES
    // TODO Note this check could be removed, or the failure could be gracefully handled
    //   I still prefer to fail because calling this method with worng arguments means there
    //   is something seriously ill in the caller code.

    auto child_ptr = gaia_ptr(child_id);

    if (!child_ptr) {
        throw invalid_node_id(child_id);
    }

    if (relation->parent_type != parent_type) {
        throw invalid_relation_type_t(first_child, parent_type, relation->parent_type);
    }

    if (relation->child_type != child_ptr.type()) {
        throw invalid_relation_type_t(first_child, child_ptr.type(), relation->child_type);
    }

    // REMOVE REFERENCE
    gaia_id_t prev_child = INVALID_GAIA_ID;
    gaia_id_t curr_child = references()[first_child];

    while (curr_child != child_id) {
        prev_child = curr_child;
        curr_child = gaia_ptr(prev_child).references()[relation->next_child];
    }

    // match found
    if (curr_child == child_id) {
        auto curr_ptr = gaia_ptr(curr_child);

        if (!prev_child) {
            // first child in the linked list, need to update the parent
            references()[first_child] = curr_ptr.references()[relation->next_child];
        } else {
            // non-first child in the linked list, update the previous child
            auto prev_ptr = gaia_ptr(prev_child);
            prev_ptr.references()[relation->next_child] = curr_ptr.references()[relation->next_child];
        }

        curr_ptr.references()[relation->parent] = INVALID_GAIA_ID;
        curr_ptr.references()[relation->next_child] = INVALID_GAIA_ID;
    }
}

void gaia_ptr::remove_parent_reference(gaia_id_t parent_id, relationship_offset_t parent) {
    auto child_type = type();

    auto child_metadata = type_registry_t::instance().get_or_create(child_type);
    auto relationship = child_metadata.find_child_relationship(parent);

    if (!relationship) {
        throw invalid_relation_offset_t(child_type, parent);
    }

    auto parent_ptr = gaia_ptr(parent_id);

    if (!parent_ptr) {
        throw invalid_node_id(parent_ptr);
    }

    // REMOVE REFERENCE
    parent_ptr.remove_child_reference(id(), relationship->first_child);
}
