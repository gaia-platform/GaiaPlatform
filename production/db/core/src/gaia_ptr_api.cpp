/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/gaia_ptr_api.hpp"

#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/catalog_core.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"
#include "gaia_internal/db/type_metadata.hpp"
#include "gaia_internal/exceptions.hpp"

#include "db_client.hpp"
#include "db_helpers.hpp"
#include "field_access.hpp"
#include "index_key.hpp"
#include "index_scan.hpp"
#include "payload_diff.hpp"
#include "type_id_mapping.hpp"

using namespace gaia::common;
using namespace gaia::db;

namespace gaia
{
namespace db
{

namespace gaia_ptr
{

namespace
{

gaia_id_t find_using_index(
    const uint8_t* payload,
    field_position_t field_position,
    gaia_type_t type,
    gaia_id_t type_id,
    gaia_id_t indexed_table_id,
    field_position_t indexed_field_position)
{
    gaia_type_t indexed_table_type = catalog_core_t::get_table(indexed_table_id).table_type();
    gaia_id_t indexed_table_type_id = type_id_mapping_t::instance().get_record_id(indexed_table_type);

    gaia_id_t index_id = catalog_core_t::find_index(indexed_table_type_id, indexed_field_position);
    // Callers need to ensure the table has an index on the field to search.
    ASSERT_PRECONDITION(index_id != c_invalid_gaia_id, "Cannot find value index for the table.");

    auto schema = catalog_core_t::get_table(type_id).binary_schema();
    auto field_value = payload_types::get_field_value(
        type,
        payload,
        schema->data(),
        schema->size(),
        field_position);

    index::index_key_t key;
    key.insert(field_value);
    for (const auto& scan : query_processor::scan::index_scan_t(
             index_id,
             std::make_shared<query_processor::scan::index_point_read_predicate_t>(key)))
    {
        return scan.id();
    }
    return c_invalid_gaia_id;
}

void parent_side_auto_connect(
    gaia_id_t id,
    gaia_type_t type,
    gaia_id_t type_id,
    gaia_id_t* references,
    const uint8_t* payload,
    field_position_t field_position)
{
    // Check if the given field is used in establishing a relationship where the
    // field's table is on the parent side.
    for (auto relationship_view : catalog_core_t::list_relationship_from(type_id))
    {
        if (relationship_view.parent_field_positions()->size() != 1
            || relationship_view.parent_field_positions()->Get(0) != field_position)
        {
            continue;
        }

        // At this point, we have found the value-linked relationship the
        // node participated in on the parent side.
        //
        // If the node already has some child nodes, disconnect all existing
        // children by detaching the children's reference anchor.
        if (references[relationship_view.first_child_offset()] != c_invalid_gaia_id)
        {
            auto parent_anchor = gaia_ptr_t::from_gaia_id(references[relationship_view.first_child_offset()]);
            if (parent_anchor.references()[c_ref_anchor_first_child_offset] != c_invalid_gaia_id)
            {
                parent_anchor.set_reference(c_ref_anchor_parent_offset, c_invalid_gaia_id);
                references[relationship_view.first_child_offset()] = c_invalid_gaia_id;
            }
        }

        // Check if the field value exists in any child node using the index on the child table.
        gaia_id_t child_id = find_using_index(
            payload,
            field_position,
            type,
            type_id,
            relationship_view.child_table_id(),
            relationship_view.child_field_positions()->Get(0));

        if (child_id != c_invalid_gaia_id)
        {
            // We have found the child with the field value. Link the child
            // node(s) to the parent node by connecting the child's
            // reference anchor to the parent.
            auto child = gaia_ptr_t::from_gaia_id(child_id);
            gaia_id_t anchor_id = child.references()[relationship_view.parent_offset()];
            references[relationship_view.first_child_offset()] = anchor_id;

            auto anchor = gaia_ptr_t::from_gaia_id(anchor_id);
            anchor.set_reference(c_ref_anchor_parent_offset, id);
        }

        // Create an anchor node for the parent node if it does not already
        // exist for the field's value.
        if (references[relationship_view.first_child_offset()] == c_invalid_gaia_id)
        {
            gaia_ptr_t anchor = gaia_ptr_t::create_ref_anchor(id, c_invalid_gaia_id);
            references[relationship_view.first_child_offset()] = anchor.id();
        }
    }
}

void child_side_auto_connect(
    gaia_id_t id,
    gaia_type_t type,
    gaia_id_t type_id,
    gaia_id_t* references,
    const uint8_t* payload,
    field_position_t field_position)
{

    // Check if the given field is used in establishing a relationship where the
    // field's table is on the child side.
    for (auto relationship_view : catalog_core_t::list_relationship_to(type_id))
    {
        if (relationship_view.child_field_positions()->size() != 1
            || relationship_view.child_field_positions()->Get(0) != field_position)
        {
            continue;
        }

        // At this point, we have found the value-linked relationship the
        // node participated in on the child side.
        //
        // Disconnect the node from its existing reference chain.
        if (references[relationship_view.next_child_offset()] != c_invalid_gaia_id)
        {
            // Update the next child if exists.
            auto next_child = gaia_ptr_t::from_gaia_id(references[relationship_view.next_child_offset()]);
            next_child.set_reference(relationship_view.prev_child_offset(), references[relationship_view.prev_child_offset()]);
        }
        if (references[relationship_view.prev_child_offset()] != c_invalid_gaia_id)
        {
            // Update the previous child if exists.
            auto prev_child = gaia_ptr_t::from_gaia_id(references[relationship_view.prev_child_offset()]);
            prev_child.set_reference(relationship_view.next_child_offset(), references[relationship_view.next_child_offset()]);
        }
        else if (references[relationship_view.parent_offset()] != c_invalid_gaia_id)
        {
            // This is the first child because previous node does not exist.
            bool anchor_deleted = false;
            if (references[relationship_view.next_child_offset()] == c_invalid_gaia_id)
            {
                // This is the only child because next node does not exist.
                auto anchor = gaia_ptr_t::from_gaia_id(references[relationship_view.parent_offset()]);
                if (anchor.references()[c_ref_anchor_parent_offset] == c_invalid_gaia_id)
                {
                    // Delete the anchor node if it is not connected to some parent node.
                    anchor.reset();
                    anchor_deleted = true;
                }
            }

            // Disconnect the (first) child from the anchor if it is not deleted in previous step.
            if (!anchor_deleted)
            {
                auto anchor = gaia_ptr_t::from_gaia_id(references[relationship_view.parent_offset()]);
                anchor.set_reference(c_ref_anchor_first_child_offset, references[relationship_view.next_child_offset()]);
            }
        }
        references[relationship_view.prev_child_offset()] = c_invalid_gaia_id;
        references[relationship_view.next_child_offset()] = c_invalid_gaia_id;
        references[relationship_view.parent_offset()] = c_invalid_gaia_id;

        // Check if the field value exists in any parent node using the index on the parent table.
        gaia_id_t parent_id = find_using_index(
            payload,
            field_position,
            type,
            type_id,
            relationship_view.parent_table_id(),
            relationship_view.parent_field_positions()->Get(0));

        if (parent_id != c_invalid_gaia_id)
        {
            // We have found the parent node with the field value. Link the
            // child node to the parent node by connecting the child to the
            // parent's child anchor node.
            auto parent = gaia_ptr_t::from_gaia_id(parent_id);
            gaia_id_t anchor_id = parent.references()[relationship_view.first_child_offset()];
            auto anchor = gaia_ptr_t::from_gaia_id(anchor_id);
            references[relationship_view.parent_offset()] = anchor_id;
            references[relationship_view.next_child_offset()] = anchor.references()[c_ref_anchor_first_child_offset];

            anchor.set_reference(c_ref_anchor_first_child_offset, id);

            if (references[relationship_view.next_child_offset()] != c_invalid_gaia_id)
            {
                auto next_child = gaia_ptr_t::from_gaia_id(references[relationship_view.next_child_offset()]);
                next_child.set_reference(relationship_view.prev_child_offset(), id);
            }
        }
        else
        {
            // Check if the field value exists in any child node using the index on the child table (this table).
            gaia_id_t child_id = find_using_index(
                payload,
                field_position,
                type,
                type_id,
                relationship_view.child_table_id(),
                relationship_view.child_field_positions()->Get(0));
            if (child_id != c_invalid_gaia_id)
            {
                // We have found some child node with the same linked field
                // value. Insert the node to the existing anchor chain.
                auto child = gaia_ptr_t::from_gaia_id(child_id);
                gaia_id_t anchor_id = child.references()[relationship_view.parent_offset()];
                auto anchor = gaia_ptr_t::from_gaia_id(anchor_id);
                references[relationship_view.parent_offset()] = anchor_id;
                references[relationship_view.next_child_offset()] = anchor.references()[c_ref_anchor_first_child_offset];

                anchor.set_reference(c_ref_anchor_first_child_offset, id);

                if (references[relationship_view.next_child_offset()] != c_invalid_gaia_id)
                {
                    auto next_child = gaia_ptr_t::from_gaia_id(references[relationship_view.next_child_offset()]);
                    next_child.set_reference(relationship_view.prev_child_offset(), id);
                }
            }
            else if (references[relationship_view.parent_offset()] == c_invalid_gaia_id)
            {
                // This child has no matching parent or other child of the
                // same field value. Create an anchor node for the child to
                // form an anchor chain of itself.
                gaia_ptr_t anchor = gaia_ptr_t::create_ref_anchor(c_invalid_gaia_id, id);
                references[relationship_view.parent_offset()] = anchor.id();
            }
        }
    }
}

void auto_connect(
    gaia_id_t id,
    gaia_type_t type,
    gaia_id_t type_id,
    gaia_id_t* references,
    const uint8_t* payload,
    const field_position_list_t& candidate_fields)
{
    // Skip catalog core tables. See notes in the other method of the same name.
    if (is_catalog_core_object(type))
    {
        return;
    }

    for (auto field_position : candidate_fields)
    {
        parent_side_auto_connect(id, type, type_id, references, payload, field_position);
        child_side_auto_connect(id, type, type_id, references, payload, field_position);
    }
}

void auto_connect(
    gaia_id_t id,
    gaia_type_t type,
    gaia_id_t* references,
    const uint8_t* payload)
{
    // Skip catalog core tables.
    //
    // This implementation relies on the following catalog tables in place to work:
    //   - gaia_table
    //   - gaia_field
    //   - gaia_relationship
    //
    // These tables will not be available during bootstrap when they are being
    // populated themselves. We can skip all system tables safely as no system
    // tables use the auto connection feature.
    if (is_catalog_core_object(type))
    {
        return;
    }
    field_position_list_t candidate_fields;
    gaia_id_t table_id = type_id_mapping_t::instance().get_record_id(type);
    for (auto field_view : catalog_core_t::list_fields(table_id))
    {
        candidate_fields.push_back(field_view.position());
    }
    auto_connect(id, type, table_id, references, payload, candidate_fields);
}

} // namespace

gaia_ptr_t create(
    common::gaia_type_t type,
    size_t data_size,
    const void* data)
{
    gaia_id_t id = gaia_ptr_t::generate_id();
    return create(id, type, data_size, data);
}

gaia_ptr_t create(
    common::gaia_id_t id,
    common::gaia_type_t type,
    size_t data_size,
    const void* data)
{
    client_t::verify_txn_active();

    const type_metadata_t& metadata = type_registry_t::instance().get(type);
    reference_offset_t num_references = metadata.num_references();

    gaia_ptr_t obj = gaia_ptr_t::create_no_txn(id, type, num_references, data_size, data);
    db_object_t* obj_ptr = obj.to_ptr();
    auto_connect(
        id,
        type,
        // NOLINTNEXTLINE: cppcoreguidelines-pro-type-const-cast
        const_cast<gaia_id_t*>(obj_ptr->references()),
        reinterpret_cast<const uint8_t*>(obj_ptr->data()));

    obj.finalize_create();

    if (client_t::is_valid_event(type))
    {
        client_t::s_events.emplace_back(triggers::event_type_t::row_insert, type, id, triggers::c_empty_position_list, get_current_txn_id());
    }
    return obj;
}

void update_payload(gaia_id_t id, size_t data_size, const void* data)
{
    gaia_ptr_t obj = gaia_ptr_t::from_gaia_id(id);
    if (!obj)
    {
        throw invalid_object_id_internal(id);
    }
    update_payload(obj, data_size, data);
}

void update_payload(gaia_ptr_t& obj, size_t data_size, const void* data)
{
    db_object_t* old_this = obj.to_ptr();
    gaia_offset_t old_offset = obj.to_offset();
    size_t references_size = old_this->num_references * sizeof(gaia_id_t);

    obj.update_payload_no_txn(data_size, data);

    auto new_data = reinterpret_cast<const uint8_t*>(data);
    auto old_data = reinterpret_cast<const uint8_t*>(old_this->payload);
    const uint8_t* old_data_payload = old_data + references_size;
    field_position_list_t changed_fields = compute_payload_diff(obj.type(), old_data_payload, new_data);

    auto_connect(
        obj.id(),
        obj.type(),
        type_id_mapping_t::instance().get_record_id(obj.type()),
        obj.references(),
        new_data,
        changed_fields);

    obj.finalize_update(old_offset);

    if (client_t::is_valid_event(obj.type()))
    {
        client_t::s_events.emplace_back(triggers::event_type_t::row_update, obj.type(), obj.id(), changed_fields, get_current_txn_id());
    }
}

void remove(gaia_ptr_t& object)
{
    if (!object)
    {
        return;
    }

    const gaia_id_t* references = object.references();

    for (reference_offset_t i = 0; i < object.num_references(); i++)
    {
        if (references[i] == c_invalid_gaia_id)
        {
            continue;
        }

        auto other_obj = gaia_ptr_t::from_gaia_id(references[i]);

        gaia_id_t referenced_obj_id = other_obj.id();

        if (other_obj.is_ref_anchor())
        {
            if (other_obj.references()[c_ref_anchor_parent_offset] == object.id())
            {
                // This is a parent node in the relationship because its
                // anchor's parent reference is pointing to itself.
                if (other_obj.references()[c_ref_anchor_first_child_offset] == c_invalid_gaia_id)
                {
                    // There is no child. We can delete the parent.
                    continue;
                }
                referenced_obj_id = other_obj.references()[c_ref_anchor_first_child_offset];
            }
            else
            {
                // This is a child node in the relationship because its
                // anchor node's parent reference is not pointing to itself.
                if (other_obj.references()[c_ref_anchor_parent_offset] == c_invalid_gaia_id)
                {
                    // There is no parent. We can delete the child.
                    //
                    // Skip checking the next two reference slots.
                    i += 2;
                    continue;
                }
                referenced_obj_id = other_obj.references()[c_ref_anchor_parent_offset];
            }
        }

        throw object_still_referenced_internal(object.id(), object.type(), referenced_obj_id, gaia_ptr_t::from_gaia_id(referenced_obj_id).type());
    }

    // Make necessary changes to the anchor chain before deleting the node.
    for (reference_offset_t i = 0; i < object.num_references(); i++)
    {
        if (references[i] != c_invalid_gaia_id)
        {
            auto anchor = gaia_ptr_t::from_gaia_id(references[i]);
            if (!anchor.is_ref_anchor())
            {
                continue;
            }

            if (anchor.references()[c_ref_anchor_parent_offset] == object.id())
            {
                // The anchor node connected to the parent can be safely deleted.
                anchor.reset();
                continue;
            }

            reference_offset_t next_offset = i + 1;

            if (anchor.references()[c_ref_anchor_first_child_offset] == object.id()
                && object.references()[next_offset] == c_invalid_gaia_id)
            {
                // This is the only child in the anchor chain. We can delete the
                // anchor node safely.
                anchor.reset();
                continue;
            }

            reference_offset_t prev_offset = next_offset + 1;

            // Disconnect the node from its existing anchor chain.
            if (object.references()[next_offset] != c_invalid_gaia_id)
            {
                auto next = gaia_ptr_t::from_gaia_id(object.references()[next_offset]);
                next.set_reference(prev_offset, object.references()[prev_offset]);
            }
            if (object.references()[prev_offset] != c_invalid_gaia_id)
            {
                auto prev = gaia_ptr_t::from_gaia_id(object.references()[prev_offset]);
                prev.set_reference(next_offset, object.references()[next_offset]);
            }
            if (anchor.references()[c_ref_anchor_first_child_offset] == object.id())
            {
                anchor.set_reference(c_ref_anchor_first_child_offset, object.references()[next_offset]);
            }
        }
    }

    object.reset();
}

bool insert_into_reference_container(gaia_ptr_t& parent, gaia_id_t child_id, reference_offset_t parent_anchor_slot)
{
    gaia_type_t parent_type = parent.type();
    const type_metadata_t& parent_metadata = type_registry_t::instance().get(parent_type);
    std::optional<relationship_t> relationship = parent_metadata.find_parent_relationship(parent_anchor_slot);
    if (!relationship)
    {
        throw invalid_reference_offset_internal(parent_type, parent_anchor_slot);
    }

    auto child = gaia_ptr_t::from_gaia_id(child_id);
    if (!child)
    {
        throw invalid_object_id_internal(child_id);
    }

    if (relationship->parent_type != parent_type)
    {
        throw invalid_relationship_type_internal(parent_anchor_slot, relationship->parent_type, parent_type);
    }
    if (relationship->child_type != child.type())
    {
        throw invalid_relationship_type_internal(parent_anchor_slot, relationship->child_type, child.type());
    }

    gaia_ptr_t anchor;
    gaia_id_t anchor_id = parent.references()[parent_anchor_slot];
    if (anchor_id != c_invalid_gaia_id)
    {
        anchor = gaia_ptr_t::from_gaia_id(anchor_id);

        if (relationship->cardinality == cardinality_t::one
            && anchor.references()[c_ref_anchor_first_child_offset] != c_invalid_gaia_id)
        {
            throw single_cardinality_violation_internal(parent_type, parent_anchor_slot);
        }

        if (child.references()[relationship->parent_offset] == anchor_id)
        {
            // Users try to insert a child that is already in the anchor chain. We
            // have nothing to do. Return false as the child is not inserted.
            return false;
        }
    }
    if (child.references()[relationship->parent_offset] != c_invalid_gaia_id)
    {
        // Do not allow inserting a child that is already in some other chain.
        throw child_already_referenced_internal(child.type(), parent_anchor_slot);
    }

    if (anchor_id == c_invalid_gaia_id)
    {
        anchor = gaia_ptr_t::create_ref_anchor(parent.id(), child_id);
        anchor_id = anchor.id();
        parent.set_reference(parent_anchor_slot, anchor_id);
        child.set_references(
            relationship->parent_offset,
            anchor_id,
            relationship->next_child_offset,
            c_invalid_gaia_id,
            relationship->prev_child_offset,
            c_invalid_gaia_id);
    }
    else
    {
        child.set_references(
            relationship->parent_offset,
            anchor_id,
            relationship->next_child_offset,
            anchor.references()[c_ref_anchor_first_child_offset],
            relationship->prev_child_offset,
            c_invalid_gaia_id);
        anchor.set_reference(c_ref_anchor_first_child_offset, child_id);
    }

    if (child.references()[relationship->next_child_offset] != c_invalid_gaia_id)
    {
        auto next_child = gaia_ptr_t::from_gaia_id(child.references()[relationship->next_child_offset]);
        next_child.set_reference(relationship->prev_child_offset, child_id);
    }

    return true;
}

bool insert_into_reference_container(gaia_id_t parent_id, gaia_id_t child_id, reference_offset_t parent_anchor_slot)
{
    gaia_ptr_t parent = gaia_ptr_t::from_gaia_id(parent_id);
    if (!parent)
    {
        throw invalid_object_id_internal(parent_id);
    }
    return insert_into_reference_container(parent, child_id, parent_anchor_slot);
}

bool remove_from_reference_container(gaia_ptr_t& parent, gaia_id_t child_id, reference_offset_t parent_anchor_slot)
{
    gaia_type_t parent_type = parent.type();
    const type_metadata_t& parent_metadata = type_registry_t::instance().get(parent_type);
    std::optional<relationship_t> relationship = parent_metadata.find_parent_relationship(parent_anchor_slot);

    if (!relationship)
    {
        throw invalid_reference_offset_internal(parent_type, parent_anchor_slot);
    }

    // Check types.
    // TODO: Note that this check could be removed or the failure could be gracefully handled.
    //   We prefer to fail because calling this method with wrong arguments means there
    //   is something seriously ill in the caller code.

    auto child = gaia_ptr_t::from_gaia_id(child_id);

    if (!child)
    {
        throw invalid_object_id_internal(child_id);
    }

    if (relationship->parent_type != parent_type)
    {
        throw invalid_relationship_type_internal(parent_anchor_slot, parent_type, relationship->parent_type);
    }

    if (relationship->child_type != child.type())
    {
        throw invalid_relationship_type_internal(parent_anchor_slot, child.type(), relationship->child_type);
    }

    if (child.references()[relationship->parent_offset] == c_invalid_gaia_id)
    {
        return false;
    }

    if (child.references()[relationship->parent_offset] != parent.references()[parent_anchor_slot])
    {
        throw invalid_child_reference_internal(child.type(), child_id, parent_type, parent.id());
    }

    return remove_from_reference_container(child, relationship->parent_offset);
}

bool remove_from_reference_container(gaia_id_t parent_id, gaia_id_t child_id, reference_offset_t parent_anchor_slot)
{
    gaia_ptr_t parent = gaia_ptr_t::from_gaia_id(parent_id);
    if (!parent)
    {
        throw invalid_object_id_internal(parent_id);
    }
    return remove_from_reference_container(parent, child_id, parent_anchor_slot);
}

bool remove_from_reference_container(gaia_ptr_t& child, reference_offset_t child_anchor_slot)
{
    reference_offset_t next_child_offset = child_anchor_slot + 1;
    reference_offset_t prev_child_offset = next_child_offset + 1;

    if (child.references()[next_child_offset] != c_invalid_gaia_id)
    {
        // Update the next child if exists.
        auto next_child = gaia_ptr_t::from_gaia_id(child.references()[next_child_offset]);
        next_child.set_reference(prev_child_offset, child.references()[prev_child_offset]);
    }
    if (child.references()[prev_child_offset] != c_invalid_gaia_id)
    {
        // Update the previous child if exists.
        auto prev_child = gaia_ptr_t::from_gaia_id(child.references()[prev_child_offset]);
        prev_child.set_reference(next_child_offset, child.references()[next_child_offset]);
    }
    else if (child.references()[child_anchor_slot] != c_invalid_gaia_id)
    {
        // This is the first child because previous node does not exist.
        if (child.references()[next_child_offset] == c_invalid_gaia_id)
        {
            // This is the only child because next node does not exist.
            // Delete the anchor node and reset the parent node's anchor slot.
            auto anchor = gaia_ptr_t::from_gaia_id(child.references()[child_anchor_slot]);
            auto parent = gaia_ptr_t::from_gaia_id(anchor.references()[c_ref_anchor_parent_offset]);
            const type_metadata_t& metadata = type_registry_t::instance().get(child.type());
            std::optional<relationship_t> relationship = metadata.find_child_relationship(child_anchor_slot);
            parent.set_reference(relationship->first_child_offset, c_invalid_gaia_id);
            anchor.reset();
        }
        else
        {
            // Disconnect the (first) child from the anchor if it is not the only child.
            auto anchor = gaia_ptr_t::from_gaia_id(child.references()[child_anchor_slot]);
            anchor.set_reference(c_ref_anchor_first_child_offset, child.references()[next_child_offset]);
        }
    }
    child.set_references(
        child_anchor_slot,
        c_invalid_gaia_id,
        next_child_offset,
        c_invalid_gaia_id,
        prev_child_offset,
        c_invalid_gaia_id);
    return true;
}

bool remove_from_reference_container(gaia_id_t child_id, reference_offset_t child_anchor_slot)
{
    gaia_ptr_t child = gaia_ptr_t::from_gaia_id(child_id);
    return remove_from_reference_container(child, child_anchor_slot);
}

bool update_parent_reference(gaia_ptr_t& child, gaia_id_t new_parent_id, reference_offset_t parent_offset)
{
    const type_metadata_t& child_metadata = type_registry_t::instance().get(child.type());
    std::optional<relationship_t> relationship = child_metadata.find_child_relationship(parent_offset);

    if (!relationship)
    {
        throw invalid_reference_offset_internal(child.type(), parent_offset);
    }

    auto new_parent = gaia_ptr_t::from_gaia_id(new_parent_id);

    if (!new_parent)
    {
        throw invalid_object_id_internal(new_parent_id);
    }

    // TODO: this implementation will produce more garbage than necessary. Also, many of the RI methods
    //  perform redundant checks. Created JIRA to improve RI performance/api:
    //  https://gaiaplatform.atlassian.net/browse/GAIAPLAT-435

    // Check cardinality.
    if (new_parent.references()[relationship->first_child_offset] != c_invalid_gaia_id)
    {
        // This parent already has a child for this relationship.
        // If the relationship is one-to-one we fail.
        if (relationship->cardinality == cardinality_t::one)
        {
            throw single_cardinality_violation_internal(new_parent.type(), relationship->first_child_offset);
        }
    }

    if (child.references()[parent_offset] != c_invalid_gaia_id)
    {
        auto anchor = gaia_ptr_t::from_gaia_id(child.references()[parent_offset]);
        auto old_parent = gaia_ptr_t::from_gaia_id(anchor.references()[c_ref_anchor_parent_offset]);
        remove_from_reference_container(old_parent.id(), child.id(), relationship->first_child_offset);
    }

    return insert_into_reference_container(new_parent_id, child.id(), relationship->first_child_offset);
}

bool update_parent_reference(gaia_id_t child_id, gaia_id_t new_parent_id, reference_offset_t parent_offset)
{
    gaia_ptr_t child = gaia_ptr_t::from_gaia_id(child_id);
    return update_parent_reference(child, new_parent_id, parent_offset);
}
} // namespace gaia_ptr

} // namespace db
} // namespace gaia
