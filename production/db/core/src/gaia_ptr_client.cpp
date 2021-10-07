/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <sys/mman.h>

#include "gaia/common.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/catalog_core.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"
#include "gaia_internal/db/triggers.hpp"

#include "db_client.hpp"
#include "db_hash_map.hpp"
#include "db_helpers.hpp"
#include "field_access.hpp"
#include "index_key.hpp"
#include "index_scan.hpp"
#include "memory_types.hpp"
#include "payload_diff.hpp"
#include "type_id_mapping.hpp"

using namespace gaia::common;
using namespace gaia::common::iterators;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::db::memory_manager;

// This helper is only used for DEBUG allocation mode, where we allocate all
// objects at page granularity and write-protect allocated pages after all
// updates to the allocated object are complete.
static void write_protect_allocation_page_for_offset(gaia_offset_t offset)
{
    // Offset must be aligned to a page in debug mode.
    ASSERT_INVARIANT(
        ((offset * c_slot_size_bytes) % c_page_size_bytes) == 0,
        "Allocations must be page-aligned in debug mode!");
    void* offset_page = page_address_from_offset(offset);
    if (-1 == ::mprotect(offset_page, c_page_size_bytes, PROT_READ))
    {
        throw_system_error("mprotect(PROT_READ) failed!");
    }
}

#ifdef DEBUG
#define WRITE_PROTECT(o) write_protect_allocation_page_for_offset((o))
#else
#define WRITE_PROTECT(o) ((void)0)
#endif

namespace gaia
{
namespace db
{

/*
 * Client-side implementation of gaia_ptr_t here.
 */

void gaia_ptr_t::reset()
{
    locators_t* locators = get_locators();
    client_t::txn_log(m_locator, to_offset(), c_invalid_gaia_offset, gaia_operation_t::remove, to_ptr()->id);

    // TODO[GAIAPLAT-445]:  We don't expose delete events.
    // if (client_t::is_valid_event(to_ptr()->type))
    // {
    //     client_t::s_events.emplace_back(event_type_t::row_delete, to_ptr()->type, to_ptr()->id, empty_position_list, get_txn_id());
    // }

    (*locators)[m_locator] = c_invalid_gaia_offset;
    m_locator = c_invalid_gaia_locator;
}

// This trivial implementation is necessary to avoid calling into client_t code from the header file.
std::shared_ptr<generator_t<gaia_id_t>>
gaia_ptr_t::get_id_generator_for_type(gaia_type_t type)
{
    return client_t::get_id_generator_for_type(type);
}

bool gaia_ptr_t::add_child_reference(gaia_id_t child_id, reference_offset_t first_child_offset)
{
    gaia_type_t parent_type = type();
    const type_metadata_t& parent_metadata = type_registry_t::instance().get(parent_type);
    std::optional<relationship_t> relationship = parent_metadata.find_parent_relationship(first_child_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(parent_type, first_child_offset);
    }

    // Check types.

    auto child_ptr = gaia_ptr_t::open(child_id);

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
            return false;
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

    // Need to handle self-references.
    if (*this != child_ptr)
    {
        WRITE_PROTECT(to_offset());
    }

    child_ptr.references()[relationship->parent_offset] = id();
    WRITE_PROTECT(child_ptr.to_offset());

    client_t::txn_log(m_locator, old_parent_offset, to_offset(), gaia_operation_t::update);
    client_t::txn_log(child_ptr.m_locator, old_child_offset, child_ptr.to_offset(), gaia_operation_t::update);
    return true;
}

bool gaia_ptr_t::add_parent_reference(gaia_id_t parent_id, reference_offset_t parent_offset)
{
    gaia_type_t child_type = type();

    const type_metadata_t& child_metadata = type_registry_t::instance().get(child_type);
    std::optional<relationship_t> child_relationship = child_metadata.find_child_relationship(parent_offset);

    if (!child_relationship)
    {
        throw invalid_reference_offset(child_type, parent_offset);
    }

    auto parent_ptr = gaia_ptr_t::open(parent_id);

    if (!parent_ptr)
    {
        throw invalid_object_id(parent_id);
    }

    return parent_ptr.add_child_reference(id(), child_relationship->first_child_offset);
}

bool gaia_ptr_t::remove_child_reference(gaia_id_t child_id, reference_offset_t first_child_offset)
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

    auto child_ptr = gaia_ptr_t::open(child_id);

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

    if (child_ptr.references()[relationship->parent_offset] == c_invalid_gaia_id)
    {
        return false;
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

    // Need to handle self-references.
    if (*this != child_ptr)
    {
        WRITE_PROTECT(child_ptr.to_offset());
    }

    gaia_id_t prev_child = c_invalid_gaia_id;
    gaia_id_t curr_child = references()[first_child_offset];

    while (curr_child != child_id && curr_child != c_invalid_gaia_id)
    {
        prev_child = curr_child;
        curr_child = gaia_ptr_t::open(prev_child).references()[relationship->next_child_offset];
    }

    // Match found.
    if (curr_child == child_id)
    {
        auto curr_ptr = gaia_ptr_t::open(curr_child);

        if (!prev_child)
        {
            // First child in the linked list, need to update the parent.
            references()[first_child_offset] = curr_ptr.references()[relationship->next_child_offset];
        }
        else
        {
            // Non-first child in the linked list, update the previous child.
            auto prev_ptr = gaia_ptr_t::open(prev_child);
            gaia_offset_t old_prev_offset = prev_ptr.to_offset();
            prev_ptr.clone_no_txn();
            prev_ptr.references()[relationship->next_child_offset]
                = curr_ptr.references()[relationship->next_child_offset];
            WRITE_PROTECT(prev_ptr.to_offset());
            client_t::txn_log(prev_ptr.m_locator, old_prev_offset, prev_ptr.to_offset(), gaia_operation_t::update);
        }

        curr_ptr.clone_no_txn();
        curr_ptr.references()[relationship->parent_offset] = c_invalid_gaia_id;
        curr_ptr.references()[relationship->next_child_offset] = c_invalid_gaia_id;
        WRITE_PROTECT(curr_ptr.to_offset());
    }
    WRITE_PROTECT(to_offset());

    WRITE_PROTECT(to_offset());

    // Need to handle self-references.
    if (*this != child_ptr)
    {
        WRITE_PROTECT(child_ptr.to_offset());
    }

    client_t::txn_log(m_locator, old_parent_offset, to_offset(), gaia_operation_t::update);
    client_t::txn_log(child_ptr.m_locator, old_child_offset, child_ptr.to_offset(), gaia_operation_t::update);
    return true;
}

bool gaia_ptr_t::remove_parent_reference(gaia_id_t parent_id, reference_offset_t parent_offset)
{
    gaia_type_t child_type = type();

    const type_metadata_t& child_metadata = type_registry_t::instance().get(child_type);
    std::optional<relationship_t> relationship = child_metadata.find_child_relationship(parent_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(child_type, parent_offset);
    }

    auto parent_ptr = gaia_ptr_t::open(parent_id);

    if (!parent_ptr)
    {
        throw invalid_object_id(parent_id);
    }

    // Remove reference.
    return parent_ptr.remove_child_reference(id(), relationship->first_child_offset);
}

bool gaia_ptr_t::update_parent_reference(
    gaia_id_t child_id,
    gaia_type_t child_type,
    gaia_id_t* child_references,
    gaia_id_t new_parent_id,
    reference_offset_t parent_offset)
{
    const type_metadata_t& child_metadata = type_registry_t::instance().get(child_type);
    std::optional<relationship_t> relationship = child_metadata.find_child_relationship(parent_offset);

    if (!relationship)
    {
        throw invalid_reference_offset(child_type, parent_offset);
    }

    auto new_parent_ptr = gaia_ptr_t::open(new_parent_id);

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

    if (child_references[parent_offset])
    {
        auto old_parent_ptr = gaia_ptr_t::open(child_references[parent_offset]);
        old_parent_ptr.remove_child_reference(child_id, relationship->first_child_offset);
    }

    return new_parent_ptr.add_child_reference(child_id, relationship->first_child_offset);
}

bool gaia_ptr_t::update_parent_reference(gaia_id_t new_parent_id, reference_offset_t parent_offset)
{
    return update_parent_reference(id(), type(), references(), new_parent_id, parent_offset);
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

    const type_metadata_t& metadata = type_registry_t::instance().get(type);
    reference_offset_t num_references = metadata.num_references();

    return create(id, type, num_references, data_size, data);
}

gaia_ptr_t gaia_ptr_t::create(gaia_id_t id, gaia_type_t type, size_t data_size, const void* data)
{
    const type_metadata_t& metadata = type_registry_t::instance().get(type);
    reference_offset_t num_references = metadata.num_references();

    return create(id, type, num_references, data_size, data);
}

gaia_ptr_t gaia_ptr_t::create(gaia_id_t id, gaia_type_t type, reference_offset_t num_references, size_t data_size, const void* data)
{
    size_t references_size = num_references * sizeof(gaia_id_t);
    size_t total_payload_size = data_size + references_size;
    if (total_payload_size > c_db_object_max_payload_size)
    {
        throw object_too_large(total_payload_size, c_db_object_max_payload_size);
    }

    // TODO: this constructor allows creating a gaia_ptr_t in an invalid state;
    //  the db_object_t should either be initialized before and passed in
    //  or it should be initialized inside the constructor.
    gaia_locator_t locator = allocate_locator();
    hash_node_t* hash_node = db_hash_map::insert(id);
    hash_node->locator = locator;
    allocate_object(locator, total_payload_size);
    gaia_ptr_t obj(locator);
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
    client_t::txn_log(locator, c_invalid_gaia_offset, obj.to_offset(), gaia_operation_t::create);

    auto_connect_to_parent(
        id,
        type,
        // NOLINTNEXTLINE: cppcoreguidelines-pro-type-const-cast
        const_cast<gaia_id_t*>(obj_ptr->references()),
        reinterpret_cast<const uint8_t*>(obj_ptr->data()));

    WRITE_PROTECT(locator_to_offset(locator));

    obj.create_insert_trigger(type, id);
    return obj;
}

void gaia_ptr_t::remove(gaia_ptr_t& object)
{
    if (!object)
    {
        return;
    }

    const gaia_id_t* references = object.references();
    for (reference_offset_t i = 0; i < object.num_references(); i++)
    {
        if (references[i] != c_invalid_gaia_id)
        {
            auto other_obj = gaia_ptr_t::open(references[i]);
            throw object_still_referenced(object.id(), object.type(), other_obj.id(), other_obj.type());
        }
    }
    object.reset();
}

void gaia_ptr_t::clone_no_txn()
{
    db_object_t* old_this = to_ptr();
    size_t total_payload_size = old_this->payload_size;
    size_t total_object_size = c_db_object_header_size + total_payload_size;
    allocate_object(m_locator, total_payload_size);
    db_object_t* new_this = to_ptr();
    memcpy(new_this, old_this, total_object_size);
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

void gaia_ptr_t::auto_connect_to_parent(
    gaia_id_t child_id,
    gaia_type_t child_type,
    gaia_id_t* child_references,
    const uint8_t* child_payload)
{
    // Skip system tables.
    //
    // This implementation relies on the following catalog tables in place to work:
    //   - gaia_table
    //   - gaia_field
    //   - gaia_relationship
    //
    // These tables will not be available during bootstrap when they are being
    // populated themselves. We can skip all system tables safely as no system
    // tables use the auto connection feature.
    if (child_type >= c_system_table_reserved_range_start)
    {
        return;
    }
    field_position_list_t candidate_fields;
    gaia_id_t table_id = type_id_mapping_t::instance().get_record_id(child_type);
    for (auto field_view : catalog_core_t::list_fields(table_id))
    {
        candidate_fields.push_back(field_view.position());
    }
    auto_connect_to_parent(child_id, child_type, table_id, child_references, child_payload, candidate_fields);
}

void gaia_ptr_t::auto_connect_to_parent(
    gaia_id_t child_id,
    gaia_type_t child_type,
    gaia_id_t child_type_id,
    gaia_id_t* child_references,
    const uint8_t* child_payload,
    const field_position_list_t& candidate_fields)
{
    // Skip system tables. See notes in the other method of the same name.
    if (child_type >= c_system_table_reserved_range_start)
    {
        return;
    }

    for (auto field_position : candidate_fields)
    {
        // For every field, check if the field is used in establishing a
        // relationship where the field's table is on the child side.
        for (auto relationship_view : catalog_core_t::list_relationship_to(child_type_id))
        {
            if (relationship_view.child_field_positions()->size() != 1
                || relationship_view.child_field_positions()->Get(0) != field_position)
            {
                continue;
            }

            // At this point, we have found the relationship (that uses the
            // field), check if the new field value exists in any parent record.
            // If yes, link the child record to the parent record.
            auto schema = catalog_core_t::get_table(child_type_id).binary_schema();
            auto field_value = payload_types::get_field_value(
                child_type_id,
                child_payload,
                schema->data(),
                schema->size(),
                field_position);

            gaia_type_t parent_table_type = catalog_core_t::get_table(relationship_view.parent_table_id()).table_type();
            gaia_id_t parent_table_id = type_id_mapping_t::instance().get_record_id(parent_table_type);
            bool has_connected = false;

            gaia_id_t parent_index_id = catalog_core_t::find_index(
                parent_table_id, relationship_view.parent_field_positions()->Get(0));
            ASSERT_INVARIANT(parent_index_id != c_invalid_gaia_id, "Cannot find value index for the parent table.");

            index::index_key_t key;
            key.insert(field_value);
            for (const auto& parent_scan : query_processor::scan::index_scan_t(
                     parent_index_id,
                     std::make_shared<query_processor::scan::index_point_read_predicate_t>(key)))
            {
                update_parent_reference(
                    child_id,
                    child_type,
                    child_references,
                    parent_scan.id(),
                    relationship_view.parent_offset());
                has_connected = true;
                break;
            }

            // If there is no match and the record was connected to some parent
            // record, we need to disconnect them.
            if (!has_connected)
            {
                gaia_ptr_t child_ptr(child_id);
                reference_offset_t parent_offset = relationship_view.parent_offset();
                gaia_id_t parent_id = child_ptr.references()[relationship_view.parent_offset()];
                if (parent_id != c_invalid_gaia_id)
                {
                    child_ptr.remove_parent_reference(parent_id, parent_offset);
                }
            }
        }
    }
}

gaia_ptr_t& gaia_ptr_t::update_payload(size_t data_size, const void* data)
{
    db_object_t* old_this = to_ptr();
    gaia_offset_t old_offset = to_offset();

    size_t references_size = old_this->num_references * sizeof(gaia_id_t);
    size_t total_payload_size = data_size + references_size;
    if (total_payload_size > c_db_object_max_payload_size)
    {
        throw object_too_large(total_payload_size, c_db_object_max_payload_size);
    }

    // Updates m_locator to point to the new object.
    allocate_object(m_locator, total_payload_size);

    db_object_t* new_this = to_ptr();

    memcpy(new_this, old_this, c_db_object_header_size);
    new_this->payload_size = total_payload_size;
    if (old_this->num_references > 0)
    {
        memcpy(new_this->payload, old_this->payload, references_size);
    }
    new_this->num_references = old_this->num_references;
    memcpy(new_this->payload + references_size, data, data_size);

    auto new_data = reinterpret_cast<const uint8_t*>(data);
    auto old_data = reinterpret_cast<const uint8_t*>(old_this->payload);
    const uint8_t* old_data_payload = old_data + references_size;
    field_position_list_t changed_fields = compute_payload_diff(new_this->type, old_data_payload, new_data);

    auto_connect_to_parent(
        id(),
        type(),
        type_id_mapping_t::instance().get_record_id(type()),
        references(),
        new_data,
        changed_fields);

    WRITE_PROTECT(to_offset());

    client_t::txn_log(m_locator, old_offset, to_offset(), gaia_operation_t::update);

    if (client_t::is_valid_event(new_this->type))
    {
        client_t::s_events.emplace_back(event_type_t::row_update, new_this->type, new_this->id, changed_fields, get_txn_id());
    }

    return *this;
}

} // namespace db
} // namespace gaia
