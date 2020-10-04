/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>
#include <iostream>

#include "retail_assert.hpp"
#include "gaia_db.hpp"
#include "gaia_se_object.hpp"
#include "type_metadata.hpp"

using namespace gaia::common;

namespace gaia {
namespace db {

class gaia_ptr {
private:
    int64_t row_id;
    void create_insert_trigger(gaia_type_t type, gaia_id_t id);
    void clone_no_tx();

public:
    gaia_ptr(const std::nullptr_t = nullptr)
        : row_id(0) {}

    gaia_ptr(const gaia_ptr& other)
        : row_id(other.row_id) {}

    gaia_ptr& operator=(const gaia_ptr& other) = default;

    bool operator==(const gaia_ptr& other) const {
        return row_id == other.row_id;
    }

    bool operator==(const std::nullptr_t) const {
        return to_ptr() == nullptr;
    }

    bool operator!=(const std::nullptr_t) const {
        return to_ptr() != nullptr;
    }

    operator bool() const {
        return to_ptr() != nullptr;
    }

    static gaia_id_t generate_id();

    static gaia_ptr create(
        gaia_type_t type,
        size_t data_size,
        const void* data) {

        gaia_id_t id = gaia_ptr::generate_id();
        auto metadata = type_registry_t::instance().get_or_create(type);
        size_t num_references = metadata.num_references();

        return create(id, type, num_references, data_size, data);
    }

    static gaia_ptr create(
        gaia_id_t id,
        gaia_type_t type,
        size_t data_size,
        const void* data) {

        auto metadata = type_registry_t::instance().get_or_create(type);
        size_t num_references = metadata.num_references();

        return create(id, type, num_references, data_size, data);
    }

    static gaia_ptr create(
        gaia_id_t id,
        gaia_type_t type,
        size_t num_refs,
        size_t data_size,
        const void* data) {
        size_t refs_len = num_refs * sizeof(gaia_id_t);
        size_t total_len = data_size + refs_len;
        gaia_ptr obj(id, total_len + sizeof(gaia_se_object_t));
        gaia_se_object_t* obj_ptr = obj.to_ptr();
        obj_ptr->id = id;
        obj_ptr->type = type;
        obj_ptr->num_references = num_refs;
        if (num_refs) {
            memset(obj_ptr->payload, 0, refs_len);
        }
        obj_ptr->payload_size = total_len;
        memcpy(obj_ptr->payload + refs_len, data, data_size);
        obj.create_insert_trigger(type, id);
        return obj;
    }

    static gaia_ptr open(
        gaia_id_t id) {
        return gaia_ptr(id);
    }

    static void remove(gaia_ptr& node) {
        if (!node) {
            return;
        }

        const gaia_id_t* references = node.references();
        for (size_t i = 0; i < node.num_references(); i++) {
            if (references[i] != INVALID_GAIA_ID) {
                throw node_not_disconnected(node.id(), node.type());
            }
        }
        node.reset();
    }

    gaia_ptr& clone();

    gaia_ptr& update_payload(size_t data_size, const void* data);

    gaia_ptr& update_parent_references(size_t child_slot, gaia_id_t child_id);

    gaia_ptr& update_child_references(
        size_t next_child_slot, gaia_id_t next_child_id,
        size_t parent_slot, gaia_id_t parent_id);

    gaia_ptr& update_child_references(size_t next_child_slot, gaia_id_t next_child_id);

    static gaia_ptr find_first(gaia_type_t type) {
        gaia_ptr ptr;
        ptr.row_id = 1;

        if (!ptr.is(type)) {
            ptr.find_next(type);
        }

        return ptr;
    }

    gaia_ptr find_next() {
        if (row_id) {
            find_next(to_ptr()->type);
        }

        return *this;
    }

    gaia_ptr operator++() {
        if (row_id) {
            find_next(to_ptr()->type);
        }
        return *this;
    }

    bool is_null() const {
        return row_id == 0;
    }

    gaia_id_t id() const {
        return to_ptr()->id;
    }

    gaia_type_t type() const {
        return to_ptr()->type;
    }

    char* data() const {
        return data_size() ? (char*)(to_ptr()->payload + (to_ptr()->num_references * sizeof(gaia_id_t))) : nullptr;
    }

    size_t data_size() const {
        size_t total_len = to_ptr()->payload_size;
        size_t refs_len = to_ptr()->num_references * sizeof(gaia_id_t);
        size_t data_size = total_len - refs_len;
        return data_size;
    }

    gaia_id_t* references() const {
        return reinterpret_cast<gaia_id_t*>(to_ptr()->payload);
    }

    size_t num_references() const {
        return to_ptr()->num_references;
    }

    /**
     * Adds a child reference to a parent object.
     *
     * Calling `parent.add_child_reference(child)` is the same as calling
     * `child.add_parent_reference(parent)`.
     *
     * @param child_id The id of the children.
     * @param first_child_offset The offset in the references array where the pointer to the child is placed.
     * @throws Exceptions there is no relationship between the parent and the child or if other
     *         integrity constraints are violated.
     */
    void add_child_reference(gaia_id_t child_id, reference_offset_t first_child_offset);

    /**
     * Add a parent reference to a child object.
     *
     * Note: Children objects have 2 pointers per relationship (next_child_offset, parent_offset)
     * only one (parent_offset) is used to denote the relationship with parent.
     *
     * @param parent_id The id of the parent
     * @param parent_offset The offset in the references array where the pointer to the parent is placed.
     * @throws Exceptions there is no relationship between the parent and the child  or if other
     *         integrity constraints are violated.
     */
    void add_parent_reference(gaia_id_t parent_id, reference_offset_t parent_offset);

    /**
     * Removes a child reference from a parent object. Without an index this operation
     * could have O(n) time complexity where n is the number of children.
     *
     * @param child_id The id of the children to be removed.
     * @param first_child_offset The offset, in the references array, of the pointer to the first child.
     */
    void remove_child_reference(gaia_id_t child_id, reference_offset_t first_child_offset);

    /**
     * Removes a parent reference from a child object. Without an index this operation
     * could have O(n) time complexity where n is the number of children.
     *
     * @param parent_id The id of the parent to be removed.
     * @param parent_offset The offset, in the references array, of the pointer to the parent.
     */
    void remove_parent_reference(gaia_id_t parent_id, reference_offset_t parent_offset);

protected:
    gaia_ptr(const gaia_id_t id);

    gaia_ptr(const gaia_id_t id, const size_t size);

    void allocate(const size_t size);

    gaia_se_object_t* to_ptr() const;

    int64_t to_offset() const;

    bool is(gaia_type_t type) const {
        return to_ptr() && to_ptr()->type == type;
    }

    void find_next(gaia_type_t type);

    void reset();
};

} // namespace db
} // namespace gaia
