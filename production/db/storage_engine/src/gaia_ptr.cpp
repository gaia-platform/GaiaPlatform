/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_ptr.hpp"

#include <cstring>

#include "memory_types.hpp"
#include "payload_diff.hpp"
#include "se_client.hpp"
#include "se_hash_map.hpp"
#include "se_helpers.hpp"
#include "stack_allocator.hpp"
#include "triggers.hpp"
#include "type_metadata.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::memory_manager;
using namespace gaia::db::triggers;

gaia_id_t gaia_ptr::generate_id()
{
    return allocate_id();
}

gaia_ptr gaia_ptr::create(gaia_type_t type, size_t data_size, const void* data)
{
    gaia_id_t id = gaia_ptr::generate_id();

    auto& metadata = type_registry_t::instance().get(type);
    size_t num_references = metadata.num_references();

    return create(id, type, num_references, data_size, data);
}

gaia_ptr gaia_ptr::create(gaia_id_t id, gaia_type_t type, size_t data_size, const void* data)
{
    auto& metadata = type_registry_t::instance().get(type);
    size_t num_references = metadata.num_references();

    return create(id, type, num_references, data_size, data);
}

gaia_ptr gaia_ptr::create(gaia_id_t id, gaia_type_t type, size_t num_refs, size_t data_size, const void* data)
{
    size_t refs_len = num_refs * sizeof(gaia_id_t);
    size_t total_len = data_size + refs_len;
    if (total_len > se_object_t::c_max_payload_size)
    {
        throw payload_size_too_large(total_len, se_object_t::c_max_payload_size);
    }

    // TODO this constructor allows creating a gaia_ptr in an invalid state
    //  the se_object_t should either be initialized before and passed in
    //  or initialized inside the constructor.
    gaia_ptr obj(id, total_len + sizeof(se_object_t));
    se_object_t* obj_ptr = obj.to_ptr();
    obj_ptr->id = id;
    obj_ptr->type = type;
    obj_ptr->num_references = num_refs;
    if (num_refs)
    {
        memset(obj_ptr->payload, 0, refs_len);
    }
    obj_ptr->payload_size = total_len;
    if (data)
    {
        memcpy(obj_ptr->payload + refs_len, data, data_size);
    }
    else
    {
        retail_assert(data_size == 0, "Null payload with non-zero payload size!");
    }

    obj.create_insert_trigger(type, id);
    return obj;
}

void gaia_ptr::remove(gaia_ptr& node)
{
    if (!node)
    {
        return;
    }

    const gaia_id_t* references = node.references();
    for (size_t i = 0; i < node.num_references(); i++)
    {
        if (references[i] != c_invalid_gaia_id)
        {
            throw node_not_disconnected(node.id(), node.type());
        }
    }
    node.reset();
}

void gaia_ptr::clone_no_txn()
{
    se_object_t* old_this = to_ptr();
    size_t new_size = sizeof(se_object_t) + old_this->payload_size;
    gaia_offset_t old_offset = to_offset();
    stack_allocator_allocate(old_offset, new_size, client::s_stack_allocators, client::s_free_stack_allocators);
    se_object_t* new_this = to_ptr();
    memcpy(new_this, old_this, new_size);
}

gaia_ptr& gaia_ptr::clone()
{
    gaia_offset_t old_offset = to_offset();
    clone_no_txn();

    client::txn_log(m_locator, old_offset, to_offset(), gaia_operation_t::clone);

    se_object_t* new_this = to_ptr();
    if (client::is_valid_event(new_this->type))
    {
        client::s_events.emplace_back(event_type_t::row_insert, new_this->type, new_this->id, empty_position_list);
    }

    return *this;
}

gaia_ptr& gaia_ptr::update_payload(size_t data_size, const void* data)
{
    se_object_t* old_this = to_ptr();
    gaia_offset_t old_offset = to_offset();

    size_t ref_len = old_this->num_references * sizeof(gaia_id_t);
    size_t total_len = data_size + ref_len;
    if (total_len > se_object_t::c_max_payload_size)
    {
        throw payload_size_too_large(total_len, se_object_t::c_max_payload_size);
    }

    // updates m_locator to point to the new object
    stack_allocator_allocate(old_offset, sizeof(se_object_t) + total_len, client::s_stack_allocators, client::s_free_stack_allocators);

    se_object_t* new_this = to_ptr();

    memcpy(new_this, old_this, sizeof(se_object_t));
    new_this->payload_size = total_len;
    if (old_this->num_references > 0)
    {
        memcpy(new_this->payload, old_this->payload, ref_len);
    }
    new_this->num_references = old_this->num_references;
    memcpy(new_this->payload + ref_len, data, data_size);

    client::txn_log(m_locator, old_offset, to_offset(), gaia_operation_t::update);

    if (client::is_valid_event(new_this->type))
    {
        auto new_data = reinterpret_cast<const uint8_t*>(data);
        auto old_data = reinterpret_cast<const uint8_t*>(old_this->payload);
        const uint8_t* old_data_payload = old_data + ref_len;

        // Compute field diff
        field_position_list_t position_list;
        compute_payload_diff(new_this->type, old_data_payload, new_data, &position_list);
        client::s_events.emplace_back(event_type_t::row_update, new_this->type, new_this->id, position_list);
    }

    return *this;
}

void gaia_ptr::create_insert_trigger(gaia_type_t type, gaia_id_t id)
{
    if (client::is_valid_event(type))
    {
        client::s_events.emplace_back(event_type_t::row_insert, type, id, empty_position_list);
    }
}

gaia_ptr::gaia_ptr(gaia_id_t id)
{
    m_locator = se_hash_map::find(id);
}

gaia_ptr::gaia_ptr(gaia_id_t id, size_t size)
{
    hash_node* hash_node = se_hash_map::insert(id);
    hash_node->locator = m_locator = allocate_locator();
    stack_allocator_allocate(0, size, client::s_stack_allocators, client::s_free_stack_allocators);
    client::txn_log(m_locator, 0, to_offset(), gaia_operation_t::create);
}

address_offset_t get_stack_allocator_offset(
    gaia_locator_t locator,
    address_offset_t old_slot_offset,
    size_t size,
    std::vector<stack_allocator_t>& transaction_allocators,
    std::vector<stack_allocator_t>& free_allocators)
{
    // If free list is empty, make an IPC call to request more memory from the server.
    // Note that this call can crash the server in case the server runs out of memory.
    if (free_allocators.size() == 0)
    {
        client::request_memory();
    }

    retail_assert(free_allocators.size() > 0, "Unable to obtain memory from server.");

    // Assign a new stack allocator if none are assigned.
    if (transaction_allocators.size() == 0)
    {
        transaction_allocators.push_back(free_allocators.front());
        free_allocators.erase(free_allocators.begin());
    }

    // Try allocating object. If current stack allocator memory isn't enough, then fetch
    // another allocator from the free list.
    address_offset_t allocated_memory_offset = c_invalid_offset;
    auto result = transaction_allocators.back().allocate(locator, old_slot_offset, size, allocated_memory_offset);

    // The stack allocator size may not be sufficient for the current object.
    if (result == error_code_t::insufficient_memory_size || result == error_code_t::memory_size_too_large)
    {
        if (free_allocators.size() == 0)
        {
            client::request_memory();
        }

        retail_assert(free_allocators.size() > 0, "Unable to obtain memory from server.");

        // Get an allocator from the free list to allocate an object from.
        transaction_allocators.push_back(free_allocators.front());
        free_allocators.erase(free_allocators.begin());

        // Reset the allocator offset and try to allocate an object again.
        allocated_memory_offset = -1;
        result = transaction_allocators.back().allocate(locator, old_slot_offset, size, allocated_memory_offset);
    }

    // Todo - add exception class.
    retail_assert(result == error_code_t::success, "Object Stack allocation failure!");
    // Size 0 indicates a deletion. We never expect memory to be allocated for a delete call.
    if (size == 0)
    {
        retail_assert(allocated_memory_offset == c_invalid_offset, "Memory allocated for a delete operation!");
    }
    else
    {
        retail_assert(allocated_memory_offset != c_invalid_offset, "Allocation failure! Stack allocator returned offset not initialized.");
    }

    return allocated_memory_offset;
}

void gaia_ptr::stack_allocator_allocate(
    address_offset_t old_slot_offset,
    size_t size,
    std::vector<stack_allocator_t>& transaction_allocators,
    std::vector<stack_allocator_t>& free_allocators)
{
    bool deallocate = size == 0;
    address_offset_t allocated_memory_offset = get_stack_allocator_offset(m_locator, old_slot_offset, size, transaction_allocators, free_allocators);

    if (!deallocate)
    {
        retail_assert(allocated_memory_offset != c_invalid_offset, "offset should be valid!");
        // Update locator array to point to the new offset.
        allocate_object(m_locator, allocated_memory_offset);
    }
}

se_object_t* gaia_ptr::to_ptr() const
{
    client::verify_txn_active();
    return locator_to_ptr(m_locator);
}

gaia_offset_t gaia_ptr::to_offset() const
{
    client::verify_txn_active();
    return locator_to_offset(m_locator);
}

void gaia_ptr::find_next(gaia_type_t type)
{
    gaia::db::data* data = gaia::db::get_shared_data();
    // We need an acquire barrier before reading `last_locator`. We can
    // change this full barrier to an acquire barrier when we change to proper
    // C++ atomic types.
    __sync_synchronize();
    // Search for objects of this type within the range of used locators.
    while (++m_locator && m_locator <= data->last_locator)
    {
        if (is(type))
        {
            return;
        }
        __sync_synchronize();
    }
    m_locator = c_invalid_gaia_locator;
}

void gaia_ptr::reset()
{
    gaia::db::locators* locators = gaia::db::get_shared_locators();
    client::txn_log(m_locator, to_offset(), 0, gaia_operation_t::remove, to_ptr()->id);

    if (client::is_valid_event(to_ptr()->type))
    {
        client::s_events.emplace_back(event_type_t::row_delete, to_ptr()->type, to_ptr()->id, empty_position_list);
    }
    (*locators)[m_locator] = c_invalid_gaia_offset;
    m_locator = c_invalid_gaia_locator;
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
    const type_metadata_t& parent_metadata = type_registry_t::instance().get(parent_type);
    std::optional<relationship_t> relationship = parent_metadata.find_parent_relationship(first_child_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(parent_type, first_child_offset);
    }

    // CHECK TYPES

    auto child_ptr = gaia_ptr(child_id);

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

    if (references()[first_child_offset] != c_invalid_gaia_id)
    {
        // This parent already has a child for this relationship.
        // If the relationship is one-to-one we fail.
        if (relationship->cardinality == cardinality_t::one)
        {
            throw single_cardinality_violation(parent_type, first_child_offset);
        }
    }

    // Note: we check only for parent under the assumption that the relational integrity
    // is preserved thus if there are no parent references there are no next_child_offset either
    if (child_ptr.references()[relationship->parent_offset] != c_invalid_gaia_id)
    {
        // If the child already has a reference to this parent, handle gracefully.
        // Otherwise throw an exception.
        if (child_ptr.references()[relationship->parent_offset] == id())
        {
            return;
        }
        throw child_already_referenced(child_ptr.type(), relationship->parent_offset);
    }

    // BUILD THE REFERENCES
    // TODO (Mihir) if the parent/child have been created in the same txn the clone may not be necessary
    gaia_offset_t old_parent_offset = to_offset();
    clone_no_txn();

    gaia_offset_t old_child_offset = child_ptr.to_offset();
    child_ptr.clone_no_txn();

    child_ptr.references()[relationship->next_child_offset] = references()[first_child_offset];
    references()[first_child_offset] = child_ptr.id();
    child_ptr.references()[relationship->parent_offset] = id();

    client::txn_log(m_locator, old_parent_offset, to_offset(), gaia_operation_t::update);
    client::txn_log(child_ptr.m_locator, old_child_offset, child_ptr.to_offset(), gaia_operation_t::update);
}

void gaia_ptr::add_parent_reference(gaia_id_t parent_id, reference_offset_t parent_offset)
{
    gaia_type_t child_type = type();

    const type_metadata_t& child_metadata = type_registry_t::instance().get(child_type);
    std::optional<relationship_t> child_relationship = child_metadata.find_child_relationship(parent_offset);

    if (!child_relationship)
    {
        throw invalid_reference_offset(child_type, parent_offset);
    }

    auto parent_ptr = gaia_ptr(parent_id);

    if (!parent_ptr)
    {
        throw invalid_node_id(parent_id);
    }

    parent_ptr.add_child_reference(id(), child_relationship->first_child_offset);
}

void gaia_ptr::remove_child_reference(gaia_id_t child_id, reference_offset_t first_child_offset)
{
    gaia_type_t parent_type = type();
    const type_metadata_t& parent_metadata = type_registry_t::instance().get(parent_type);
    std::optional<relationship_t> relationship = parent_metadata.find_parent_relationship(first_child_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(parent_type, first_child_offset);
    }

    // CHECK TYPES
    // TODO Note this check could be removed, or the failure could be gracefully handled
    //   I still prefer to fail because calling this method with wrong arguments means there
    //   is something seriously ill in the caller code.

    auto child_ptr = gaia_ptr(child_id);

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

    if (child_ptr.references()[relationship->parent_offset] != id())
    {
        throw invalid_child(child_ptr.type(), child_id, type(), id());
    }

    // REMOVE REFERENCE
    gaia_offset_t old_parent_offset = to_offset();
    clone_no_txn();
    gaia_offset_t old_child_offset = child_ptr.to_offset();
    child_ptr.clone_no_txn();

    gaia_id_t prev_child = c_invalid_gaia_id;
    gaia_id_t curr_child = references()[first_child_offset];

    while (curr_child != child_id && curr_child != c_invalid_gaia_id)
    {
        prev_child = curr_child;
        curr_child = gaia_ptr(prev_child).references()[relationship->next_child_offset];
    }

    // match found
    if (curr_child == child_id)
    {
        auto curr_ptr = gaia_ptr(curr_child);

        if (!prev_child)
        {
            // first child in the linked list, need to update the parent
            references()[first_child_offset] = curr_ptr.references()[relationship->next_child_offset];
        }
        else
        {
            // non-first child in the linked list, update the previous child
            auto prev_ptr = gaia_ptr(prev_child);
            prev_ptr.references()[relationship->next_child_offset]
                = curr_ptr.references()[relationship->next_child_offset];
        }

        curr_ptr.references()[relationship->parent_offset] = c_invalid_gaia_id;
        curr_ptr.references()[relationship->next_child_offset] = c_invalid_gaia_id;
    }

    client::txn_log(m_locator, old_parent_offset, to_offset(), gaia_operation_t::update);
    client::txn_log(child_ptr.m_locator, old_child_offset, child_ptr.to_offset(), gaia_operation_t::update);
}

void gaia_ptr::remove_parent_reference(gaia_id_t parent_id, reference_offset_t parent_offset)
{
    gaia_type_t child_type = type();

    const type_metadata_t& child_metadata = type_registry_t::instance().get(child_type);
    std::optional<relationship_t> relationship = child_metadata.find_child_relationship(parent_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(child_type, parent_offset);
    }

    auto parent_ptr = gaia_ptr(parent_id);

    if (!parent_ptr)
    {
        throw invalid_node_id(parent_id);
    }

    // REMOVE REFERENCE
    parent_ptr.remove_child_reference(id(), relationship->first_child_offset);
}

void gaia_ptr::update_parent_reference(gaia_id_t new_parent_id, reference_offset_t parent_offset)
{
    gaia_type_t child_type = type();

    const type_metadata_t& child_metadata = type_registry_t::instance().get(child_type);
    std::optional<relationship_t> relationship = child_metadata.find_child_relationship(parent_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(child_type, parent_offset);
    }

    auto new_parent_ptr = gaia_ptr(new_parent_id);

    if (!new_parent_ptr)
    {
        throw invalid_node_id(new_parent_id);
    }

    // TODO this implementation will produce more garbage than necessary. Also many of the RI methods
    //  perform redundant checks. Created JIRA to improve RI performance/api:
    //  https://gaiaplatform.atlassian.net/browse/GAIAPLAT-435

    // CHECK CARDINALITY
    if (new_parent_ptr.references()[relationship->first_child_offset] != c_invalid_gaia_id)
    {
        // This parent already has a child for this relationship.
        // If the relationship is one-to-one we fail.
        if (relationship->cardinality == cardinality_t::one)
        {
            throw single_cardinality_violation(new_parent_ptr.type(), relationship->first_child_offset);
        }
    }

    if (references()[parent_offset])
    {
        auto old_parent_ptr = gaia_ptr(references()[parent_offset]);
        old_parent_ptr.remove_child_reference(id(), relationship->first_child_offset);
    }

    new_parent_ptr.add_child_reference(id(), relationship->first_child_offset);
}
