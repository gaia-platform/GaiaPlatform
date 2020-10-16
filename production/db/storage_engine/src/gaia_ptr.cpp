/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_ptr.hpp"

#include "field_list.hpp"
#include "gaia_hash_map.hpp"
#include "generator_iterator.hpp"
#include "payload_diff.hpp"
#include "storage_engine.hpp"
#include "storage_engine_client.hpp"
#include "triggers.hpp"
#include "type_metadata.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::triggers;

gaia_id_t gaia_ptr::generate_id()
{
    return client::generate_id(client::s_data);
}

void gaia_ptr::clone_no_txn()
{
    gaia_se_object_t* old_this = to_ptr();
    size_t new_size = sizeof(gaia_se_object_t) + old_this->payload_size;
    allocate(new_size);
    gaia_se_object_t* new_this = to_ptr();
    memcpy(new_this, old_this, new_size);
}

gaia_ptr& gaia_ptr::clone()
{
    gaia_offset_t old_offset = to_offset();
    clone_no_txn();

    client::txn_log(m_locator, old_offset, to_offset(), gaia_operation_t::clone);

    gaia_se_object_t* new_this = to_ptr();
    if (client::is_valid_event(new_this->type))
    {
        client::s_events.push_back(trigger_event_t{event_type_t::row_insert, new_this->type, new_this->id, empty_position_list});
    }

    return *this;
}

gaia_ptr& gaia_ptr::update_payload(size_t data_size, const void* data)
{
    gaia_se_object_t* old_this = to_ptr();
    gaia_offset_t old_offset = to_offset();

    size_t ref_len = old_this->num_references * sizeof(gaia_id_t);
    size_t total_len = data_size + ref_len;
    allocate(sizeof(gaia_se_object_t) + total_len);

    gaia_se_object_t* new_this = to_ptr();

    memcpy(new_this, old_this, sizeof(gaia_se_object_t));
    new_this->payload_size = total_len;
    if (old_this->num_references)
    {
        memcpy(new_this->payload, old_this->payload, ref_len);
    }
    new_this->num_references = old_this->num_references;
    memcpy(new_this->payload + ref_len, data, data_size);

    client::txn_log(m_locator, old_offset, to_offset(), gaia_operation_t::update);

    if (client::is_valid_event(new_this->type))
    {
        const uint8_t* old_data = (const uint8_t*)old_this->payload;

        const uint8_t* old_data_payload;

        if (old_this->num_references)
        {
            old_data_payload = old_data + sizeof(gaia_id_t) * old_this->num_references;
        }
        // Compute field diff
        field_position_list_t position_list;
        gaia::db::payload_types::compute_payload_diff(new_this->type, old_data_payload, (const uint8_t*)data, &position_list);
        client::s_events.push_back(trigger_event_t{event_type_t::row_update, new_this->type, new_this->id, position_list});
    }

    return *this;
}

gaia_ptr& gaia_ptr::update_child_references(
    size_t next_child_slot, gaia_id_t next_child_id, size_t parent_slot, gaia_id_t parent_id)
{
    gaia_offset_t old_offset = to_offset();
    clone_no_txn();

    references()[next_child_slot] = next_child_id;
    references()[parent_slot] = parent_id;

    client::txn_log(m_locator, old_offset, to_offset(), gaia_operation_t::update);
    return *this;
}

gaia_ptr& gaia_ptr::update_child_reference(size_t child_slot, gaia_id_t child_id)
{
    auto old_offset = to_offset();
    clone_no_txn();

    references()[child_slot] = child_id;

    client::txn_log(m_locator, old_offset, to_offset(), gaia_operation_t::update);
    return *this;
}

void gaia_ptr::create_insert_trigger(gaia_type_t type, gaia_id_t id)
{
    if (client::is_valid_event(type))
    {
        client::s_events.push_back(trigger_event_t{event_type_t::row_insert, type, id, empty_position_list});
    }
}

gaia_ptr::gaia_ptr(gaia_id_t id)
{
    m_locator = gaia_hash_map::find(client::s_data, client::s_locators, id);
}

gaia_ptr::gaia_ptr(gaia_id_t id, size_t size)
    : m_locator(0)
{
    se_base::hash_node* hash_node = gaia_hash_map::insert(client::s_data, client::s_locators, id);
    hash_node->locator = m_locator = se_base::allocate_locator(client::s_locators, client::s_data);
    se_base::allocate_object(m_locator, size, client::s_locators, client::s_data);
    client::txn_log(m_locator, 0, to_offset(), gaia_operation_t::create);
}

void gaia_ptr::allocate(size_t size)
{
    se_base::allocate_object(m_locator, size, client::s_locators, client::s_data);
}

gaia_se_object_t* gaia_ptr::to_ptr() const
{
    client::verify_txn_active();

    return m_locator && se_base::locator_exists(client::s_locators, m_locator)
        ? reinterpret_cast<gaia_se_object_t*>(client::s_data->objects + (*client::s_locators)[m_locator])
        : nullptr;
}

gaia_offset_t gaia_ptr::to_offset() const
{
    client::verify_txn_active();

    return m_locator
        ? (*client::s_locators)[m_locator]
        : 0;
}

void gaia_ptr::find_next(gaia_type_t type)
{
    // search for rows of this type within the range of used slots
    while (++m_locator && m_locator < client::s_data->locator_count + 1)
    {
        if (is(type))
        {
            return;
        }
    }
    m_locator = 0;
}

void gaia_ptr::reset()
{
    client::txn_log(m_locator, to_offset(), 0, gaia_operation_t::remove, to_ptr()->id);

    if (client::is_valid_event(to_ptr()->type))
    {
        client::s_events.push_back(trigger_event_t{event_type_t::row_delete, to_ptr()->type, to_ptr()->id, empty_position_list});
    }
    (*client::s_locators)[m_locator] = 0;
    m_locator = 0;
}

// This trivial implementation is necessary to avoid calling into client code from the header file.
std::function<std::optional<gaia_id_t>()>
gaia_ptr::get_id_generator_for_type(gaia_type_t type)
{
    return client::get_id_generator_for_type(type);
}

void gaia_ptr::add_child_reference(gaia_id_t child_id, reference_offset_t first_child_offset)
{
    gaia_type_t parent_type = type();
    auto parent_metadata = type_registry_t::instance().get_or_create(parent_type);
    auto relationship = parent_metadata.find_parent_relationship(first_child_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(parent_type, first_child_offset);
    }

    // CHECK TYPES

    gaia_ptr child_ptr = gaia_ptr(child_id);

    if (!child_ptr)
    {
        throw invalid_node_id(child_id);
    }

    if (relationship->parent_type != parent_type)
    {
        throw invalid_relationship_type(first_child_offset, parent_type, relationship->parent_type);
    }

    if (relationship->child_type != child_ptr.type())
    {
        throw invalid_relationship_type(first_child_offset, child_ptr.type(), relationship->child_type);
    }

    // CHECK CARDINALITY

    if (references()[first_child_offset] != INVALID_GAIA_ID)
    {
        // this parent already has a child for this relationship.
        // If the relationship is one-to-one we fail.
        if (relationship->cardinality == cardinality_t::one)
        {
            throw single_cardinality_violation(parent_type, first_child_offset);
        }
    }

    // Note: we check only for parent under the assumption that the relational integrity
    // is preserved thus if there are no parent references there are no next_child_offset either
    if (child_ptr.references()[relationship->parent_offset] != INVALID_GAIA_ID)
    {
        // ATM we don't allow a reference to be re-assigned on the fly.
        // Users need to explicitly call remove_child_reference() or
        // remove_parent_reference() before replacing an existing reference.
        // In future we may introduce flags/parameters that allow to do so.
        throw child_already_referenced(child_ptr.type(), relationship->parent_offset);
    }

    // BUILD THE REFERENCES

    child_ptr.references()[relationship->next_child_offset] = references()[first_child_offset];
    references()[first_child_offset] = child_ptr.id();
    child_ptr.references()[relationship->parent_offset] = id();
}

void gaia_ptr::add_parent_reference(gaia_id_t parent_id, reference_offset_t parent_offset)
{
    gaia_type_t child_type = type();

    auto child_metadata = type_registry_t::instance().get_or_create(child_type);
    auto child_relationship = child_metadata.find_child_relationship(parent_offset);

    if (!child_relationship)
    {
        throw invalid_reference_offset(child_type, parent_offset);
    }

    gaia_ptr parent_ptr = gaia_ptr(parent_id);

    if (!parent_ptr)
    {
        throw invalid_node_id(parent_ptr);
    }

    parent_ptr.add_child_reference(id(), child_relationship->first_child_offset);
}

void gaia_ptr::remove_child_reference(gaia_id_t child_id, reference_offset_t first_child_offset)
{
    gaia_type_t parent_type = type();
    auto parent_metadata = type_registry_t::instance().get_or_create(parent_type);
    auto relationship = parent_metadata.find_parent_relationship(first_child_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(parent_type, first_child_offset);
    }

    // CHECK TYPES
    // TODO Note this check could be removed, or the failure could be gracefully handled
    //   I still prefer to fail because calling this method with worng arguments means there
    //   is something seriously ill in the caller code.

    gaia_ptr child_ptr = gaia_ptr(child_id);

    if (!child_ptr)
    {
        throw invalid_node_id(child_id);
    }

    if (relationship->parent_type != parent_type)
    {
        throw invalid_relationship_type(first_child_offset, parent_type, relationship->parent_type);
    }

    if (relationship->child_type != child_ptr.type())
    {
        throw invalid_relationship_type(first_child_offset, child_ptr.type(), relationship->child_type);
    }

    // REMOVE REFERENCE
    gaia_id_t prev_child = INVALID_GAIA_ID;
    gaia_id_t curr_child = references()[first_child_offset];

    while (curr_child != child_id)
    {
        prev_child = curr_child;
        curr_child = gaia_ptr(prev_child).references()[relationship->next_child_offset];
    }

    // match found
    if (curr_child == child_id)
    {
        gaia_ptr curr_ptr = gaia_ptr(curr_child);

        if (!prev_child)
        {
            // first child in the linked list, need to update the parent
            references()[first_child_offset] = curr_ptr.references()[relationship->next_child_offset];
        }
        else
        {
            // non-first child in the linked list, update the previous child
            gaia_ptr prev_ptr = gaia_ptr(prev_child);
            prev_ptr.references()[relationship->next_child_offset] = curr_ptr.references()[relationship->next_child_offset];
        }

        curr_ptr.references()[relationship->parent_offset] = INVALID_GAIA_ID;
        curr_ptr.references()[relationship->next_child_offset] = INVALID_GAIA_ID;
    }
}

void gaia_ptr::remove_parent_reference(gaia_id_t parent_id, reference_offset_t parent_offset)
{
    gaia_type_t child_type = type();

    auto child_metadata = type_registry_t::instance().get_or_create(child_type);
    auto relationship = child_metadata.find_child_relationship(parent_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(child_type, parent_offset);
    }

    gaia_ptr parent_ptr = gaia_ptr(parent_id);

    if (!parent_ptr)
    {
        throw invalid_node_id(parent_ptr);
    }

    // REMOVE REFERENCE
    parent_ptr.remove_child_reference(id(), relationship->first_child_offset);
}
