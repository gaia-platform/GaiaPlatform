/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/gaia_ptr.hpp"

#include <cstring>

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/triggers.hpp"
#include "gaia_internal/db/type_metadata.hpp"

#include "db_client.hpp"
#include "db_hash_map.hpp"
#include "db_helpers.hpp"
#include "memory_types.hpp"
#include "payload_diff.hpp"

using namespace gaia::common;
using namespace gaia::common::iterators;
using namespace gaia::db::memory_manager;
using namespace gaia::db::triggers;

namespace gaia
{
namespace db
{

gaia_ptr_t::gaia_ptr_t(gaia_id_t id)
{
    m_locator = db_hash_map::find(id);
}

gaia_ptr_t::gaia_ptr_t(gaia_locator_t locator, address_offset_t offset)
{
    m_locator = locator;
    client_t::txn_log(m_locator, c_invalid_gaia_offset, get_gaia_offset(offset), gaia_operation_t::create);
}

gaia_id_t gaia_ptr_t::generate_id()
{
    return allocate_id();
}

db_object_t* gaia_ptr_t::to_ptr() const
{
    client_t::verify_txn_active();
    return locator_to_ptr(m_locator);
}

gaia_offset_t gaia_ptr_t::to_offset() const
{
    client_t::verify_txn_active();
    return locator_to_offset(m_locator);
}

void gaia_ptr_t::create_insert_trigger(gaia_type_t type, gaia_id_t id)
{
    if (client_t::is_valid_event(type))
    {
        client_t::s_events.emplace_back(event_type_t::row_insert, type, id, empty_position_list, get_txn_id());
    }
}

gaia_ptr_t gaia_ptr_t::create(gaia_type_t type, size_t data_size, const void* data)
{
    gaia_id_t id = gaia_ptr_t::generate_id();

    auto& metadata = type_registry_t::instance().get(type);
    size_t num_references = metadata.num_references();

    return create(id, type, num_references, data_size, data);
}

gaia_ptr_t gaia_ptr_t::create(gaia_id_t id, gaia_type_t type, size_t data_size, const void* data)
{
    auto& metadata = type_registry_t::instance().get(type);
    size_t num_references = metadata.num_references();

    return create(id, type, num_references, data_size, data);
}

gaia_ptr_t gaia_ptr_t::create(gaia_id_t id, gaia_type_t type, size_t num_references, size_t data_size, const void* data)
{
    size_t references_size = num_references * sizeof(gaia_id_t);
    size_t total_payload_size = data_size + references_size;
    if (total_payload_size > db_object_t::c_max_payload_size)
    {
        throw object_too_large(total_payload_size, db_object_t::c_max_payload_size);
    }

    // TODO: this constructor allows creating a gaia_ptr_t in an invalid state;
    //  the db_object_t should either be initialized before and passed in
    //  or it should be initialized inside the constructor.
    hash_node_t* hash_node = db_hash_map::insert(id);
    size_t object_size = total_payload_size + sizeof(db_object_t);
    hash_node->locator = allocate_locator();
    address_offset_t offset = client_t::allocate_object(hash_node->locator, object_size);
    gaia_ptr_t obj(hash_node->locator, offset);
    db_object_t* obj_ptr = obj.to_ptr();
    obj_ptr->id = id;
    obj_ptr->type = type;
    obj_ptr->num_references = num_references;
    if (num_references)
    {
        memset(obj_ptr->payload, 0, references_size);
    }
    obj_ptr->payload_size = total_payload_size;
    if (data)
    {
        memcpy(obj_ptr->payload + references_size, data, data_size);
    }
    else
    {
        ASSERT_INVARIANT(data_size == 0, "Null payload with non-zero payload size!");
    }

    obj.create_insert_trigger(type, id);
    return obj;
}

void gaia_ptr_t::remove(gaia_ptr_t& node)
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
            throw object_still_referenced(node.id(), node.type());
        }
    }
    node.reset();
}

void gaia_ptr_t::clone_no_txn()
{
    db_object_t* old_this = to_ptr();
    size_t new_size = sizeof(db_object_t) + old_this->payload_size;
    client_t::allocate_object(m_locator, new_size);
    db_object_t* new_this = to_ptr();
    memcpy(new_this, old_this, new_size);
}

gaia_ptr_t& gaia_ptr_t::clone()
{
    gaia_offset_t old_offset = to_offset();
    clone_no_txn();

    client_t::txn_log(m_locator, old_offset, to_offset(), gaia_operation_t::clone);

    db_object_t* new_this = to_ptr();
    if (client_t::is_valid_event(new_this->type))
    {
        client_t::s_events.emplace_back(event_type_t::row_insert, new_this->type, new_this->id, empty_position_list, get_txn_id());
    }

    return *this;
}

gaia_ptr_t& gaia_ptr_t::update_payload(size_t data_size, const void* data)
{
    db_object_t* old_this = to_ptr();
    gaia_offset_t old_offset = to_offset();

    size_t references_size = old_this->num_references * sizeof(gaia_id_t);
    size_t total_payload_size = data_size + references_size;
    if (total_payload_size > db_object_t::c_max_payload_size)
    {
        throw object_too_large(total_payload_size, db_object_t::c_max_payload_size);
    }

    // Updates m_locator to point to the new object.
    client_t::allocate_object(m_locator, sizeof(db_object_t) + total_payload_size);

    db_object_t* new_this = to_ptr();

    memcpy(new_this, old_this, sizeof(db_object_t));
    new_this->payload_size = total_payload_size;
    if (old_this->num_references > 0)
    {
        memcpy(new_this->payload, old_this->payload, references_size);
    }
    new_this->num_references = old_this->num_references;
    memcpy(new_this->payload + references_size, data, data_size);

    client_t::txn_log(m_locator, old_offset, to_offset(), gaia_operation_t::update);

    if (client_t::is_valid_event(new_this->type))
    {
        auto new_data = reinterpret_cast<const uint8_t*>(data);
        auto old_data = reinterpret_cast<const uint8_t*>(old_this->payload);
        const uint8_t* old_data_payload = old_data + references_size;

        // Compute field difference.
        field_position_list_t position_list;
        compute_payload_diff(new_this->type, old_data_payload, new_data, &position_list);
        client_t::s_events.emplace_back(event_type_t::row_update, new_this->type, new_this->id, position_list, get_txn_id());
    }

    return *this;
}

gaia_ptr_t gaia_ptr_t::find_first(common::gaia_type_t type)
{
    gaia_ptr_t ptr;
    ptr.m_locator = c_first_gaia_locator;

    if (!ptr.is(type))
    {
        ptr = ptr.find_next(type);
    }

    return ptr;
}

gaia_ptr_t gaia_ptr_t::find_next() const
{
    if (m_locator)
    {
        return find_next(to_ptr()->type);
    }

    return *this;
}

gaia_ptr_t gaia_ptr_t::find_next(gaia_type_t type) const
{
    gaia::db::counters_t* counters = gaia::db::get_counters();
    gaia_ptr_t next_ptr = *this;

    // We need an acquire barrier before reading `last_locator`. We can
    // change this full barrier to an acquire barrier when we change to proper
    // C++ atomic types.
    __sync_synchronize();

    // Search for objects of this type within the range of used locators.
    while (++next_ptr.m_locator && next_ptr.m_locator <= counters->last_locator)
    {
        if (next_ptr.is(type))
        {
            return next_ptr;
        }
        __sync_synchronize();
    }

    // Mark end of search.
    next_ptr.m_locator = c_invalid_gaia_locator;
    return next_ptr;
}

// This trivial implementation is necessary to avoid calling into client_t code from the header file.
std::shared_ptr<generator_t<gaia_id_t>>
gaia_ptr_t::get_id_generator_for_type(gaia_type_t type)
{
    return client_t::get_id_generator_for_type(type);
}

gaia_ptr_generator_t::gaia_ptr_generator_t(std::shared_ptr<generator_t<gaia_id_t>> id_generator)
    : m_id_generator(std::move(id_generator))
{
}

std::optional<gaia_ptr_t> gaia_ptr_generator_t::operator()()
{
    std::optional<gaia_id_t> id_opt;
    while ((id_opt = (*m_id_generator)()))
    {
        gaia_ptr_t gaia_ptr = gaia_ptr_t::open(*id_opt);
        if (gaia_ptr)
        {
            return gaia_ptr;
        }
    }
    return std::nullopt;
}

generator_iterator_t<gaia_ptr_t>
gaia_ptr_t::find_all_iterator(
    gaia_type_t type)
{
    // Get the gaia_id generator and wrap it in a gaia_ptr_t generator.
    gaia_ptr_generator_t gaia_ptr_generator(get_id_generator_for_type(type));
    return generator_iterator_t<gaia_ptr_t>(std::move(gaia_ptr_generator));
}

generator_range_t<gaia_ptr_t> gaia_ptr_t::find_all_range(
    gaia_type_t type)
{
    return range_from_generator_iterator(find_all_iterator(type));
}

void gaia_ptr_t::reset()
{
    gaia::db::locators_t* locators = gaia::db::get_locators();
    client_t::txn_log(m_locator, to_offset(), c_invalid_gaia_offset, gaia_operation_t::remove, to_ptr()->id);

    if (client_t::is_valid_event(to_ptr()->type))
    {
        client_t::s_events.emplace_back(event_type_t::row_delete, to_ptr()->type, to_ptr()->id, empty_position_list, get_txn_id());
    }
    (*locators)[m_locator] = c_invalid_gaia_offset;
    m_locator = c_invalid_gaia_locator;
}

void gaia_ptr_t::add_child_reference(gaia_id_t child_id, reference_offset_t first_child_offset)
{
    gaia_type_t parent_type = type();
    const type_metadata_t& parent_metadata = type_registry_t::instance().get(parent_type);
    std::optional<relationship_t> relationship = parent_metadata.find_parent_relationship(first_child_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(parent_type, first_child_offset);
    }

    // Check types.

    auto child_ptr = gaia_ptr_t(child_id);

    if (!child_ptr)
    {
        throw invalid_object_id(child_id);
    }

    if (relationship->parent_type != parent_type)
    {
        throw invalid_relationship_type(first_child_offset, relationship->parent_type, parent_type);
    }

    if (relationship->child_type != child_ptr.type())
    {
        throw invalid_relationship_type(first_child_offset, relationship->child_type, child_ptr.type());
    }

    // Check cardinality.

    if (references()[first_child_offset] != c_invalid_gaia_id)
    {
        // This parent already has a child for this relationship.
        // If the relationship is one-to-one, we fail.
        if (relationship->cardinality == cardinality_t::one)
        {
            throw single_cardinality_violation(parent_type, first_child_offset);
        }
    }

    // Note: we check only for parent under the assumption that the relational integrity
    // is preserved, thus if there are no parent references there are no next_child_offset values either.
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
    // TODO (Mihir): if the parent/child have been created in the same txn, the clone may not be necessary.
    gaia_offset_t old_parent_offset = to_offset();
    clone_no_txn();

    gaia_offset_t old_child_offset = child_ptr.to_offset();
    child_ptr.clone_no_txn();

    child_ptr.references()[relationship->next_child_offset] = references()[first_child_offset];
    references()[first_child_offset] = child_ptr.id();
    child_ptr.references()[relationship->parent_offset] = id();

    client_t::txn_log(m_locator, old_parent_offset, to_offset(), gaia_operation_t::update);
    client_t::txn_log(child_ptr.m_locator, old_child_offset, child_ptr.to_offset(), gaia_operation_t::update);
}

void gaia_ptr_t::add_parent_reference(gaia_id_t parent_id, reference_offset_t parent_offset)
{
    gaia_type_t child_type = type();

    const type_metadata_t& child_metadata = type_registry_t::instance().get(child_type);
    std::optional<relationship_t> child_relationship = child_metadata.find_child_relationship(parent_offset);

    if (!child_relationship)
    {
        throw invalid_reference_offset(child_type, parent_offset);
    }

    auto parent_ptr = gaia_ptr_t(parent_id);

    if (!parent_ptr)
    {
        throw invalid_object_id(parent_id);
    }

    parent_ptr.add_child_reference(id(), child_relationship->first_child_offset);
}

void gaia_ptr_t::remove_child_reference(gaia_id_t child_id, reference_offset_t first_child_offset)
{
    gaia_type_t parent_type = type();
    const type_metadata_t& parent_metadata = type_registry_t::instance().get(parent_type);
    std::optional<relationship_t> relationship = parent_metadata.find_parent_relationship(first_child_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(parent_type, first_child_offset);
    }

    // Check types.
    // TODO: Note that this check could be removed or the failure could be gracefully handled.
    //   We prefer to fail because calling this method with wrong arguments means there
    //   is something seriously ill in the caller code.

    auto child_ptr = gaia_ptr_t(child_id);

    if (!child_ptr)
    {
        throw invalid_object_id(child_id);
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

    // Remove reference.
    gaia_offset_t old_parent_offset = to_offset();
    clone_no_txn();
    gaia_offset_t old_child_offset = child_ptr.to_offset();
    child_ptr.clone_no_txn();

    gaia_id_t prev_child = c_invalid_gaia_id;
    gaia_id_t curr_child = references()[first_child_offset];

    while (curr_child != child_id && curr_child != c_invalid_gaia_id)
    {
        prev_child = curr_child;
        curr_child = gaia_ptr_t(prev_child).references()[relationship->next_child_offset];
    }

    // Match found.
    if (curr_child == child_id)
    {
        auto curr_ptr = gaia_ptr_t(curr_child);

        if (!prev_child)
        {
            // First child in the linked list, need to update the parent.
            references()[first_child_offset] = curr_ptr.references()[relationship->next_child_offset];
        }
        else
        {
            // Non-first child in the linked list, update the previous child.
            auto prev_ptr = gaia_ptr_t(prev_child);
            prev_ptr.references()[relationship->next_child_offset]
                = curr_ptr.references()[relationship->next_child_offset];
        }

        curr_ptr.references()[relationship->parent_offset] = c_invalid_gaia_id;
        curr_ptr.references()[relationship->next_child_offset] = c_invalid_gaia_id;
    }

    client_t::txn_log(m_locator, old_parent_offset, to_offset(), gaia_operation_t::update);
    client_t::txn_log(child_ptr.m_locator, old_child_offset, child_ptr.to_offset(), gaia_operation_t::update);
}

void gaia_ptr_t::remove_parent_reference(gaia_id_t parent_id, reference_offset_t parent_offset)
{
    gaia_type_t child_type = type();

    const type_metadata_t& child_metadata = type_registry_t::instance().get(child_type);
    std::optional<relationship_t> relationship = child_metadata.find_child_relationship(parent_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(child_type, parent_offset);
    }

    auto parent_ptr = gaia_ptr_t(parent_id);

    if (!parent_ptr)
    {
        throw invalid_object_id(parent_id);
    }

    // Remove reference.
    parent_ptr.remove_child_reference(id(), relationship->first_child_offset);
}

void gaia_ptr_t::update_parent_reference(gaia_id_t new_parent_id, reference_offset_t parent_offset)
{
    gaia_type_t child_type = type();

    const type_metadata_t& child_metadata = type_registry_t::instance().get(child_type);
    std::optional<relationship_t> relationship = child_metadata.find_child_relationship(parent_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(child_type, parent_offset);
    }

    auto new_parent_ptr = gaia_ptr_t(new_parent_id);

    if (!new_parent_ptr)
    {
        throw invalid_object_id(new_parent_id);
    }

    // TODO: this implementation will produce more garbage than necessary. Also, many of the RI methods
    //  perform redundant checks. Created JIRA to improve RI performance/api:
    //  https://gaiaplatform.atlassian.net/browse/GAIAPLAT-435

    // Check cardinality.
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
        auto old_parent_ptr = gaia_ptr_t(references()[parent_offset]);
        old_parent_ptr.remove_child_reference(id(), relationship->first_child_offset);
    }

    new_parent_ptr.add_child_reference(id(), relationship->first_child_offset);
}

} // namespace db
} // namespace gaia
