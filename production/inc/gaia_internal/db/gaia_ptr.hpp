/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia/common.hpp"

#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/type_metadata.hpp"

#include "memory_types.hpp"

namespace gaia
{
namespace db
{

/**
* gaia_ptr_t is implemented differently on the server and client-side.
* See their respective cpp files for differences.
*/
class gaia_ptr_t
{
public:
    gaia_ptr_t() = default;

    inline explicit gaia_ptr_t(gaia_locator_t locator)
        : m_locator(locator)
    {
    }

    inline bool operator==(const gaia_ptr_t& other) const;
    inline bool operator!=(const gaia_ptr_t& other) const;
    inline bool operator==(const std::nullptr_t) const;
    inline bool operator!=(const std::nullptr_t) const;
    inline explicit operator bool() const;

    static common::gaia_id_t generate_id();

    static gaia_ptr_t create(
        common::gaia_type_t type,
        size_t data_size,
        const void* data);

    static gaia_ptr_t create(
        common::gaia_id_t id,
        common::gaia_type_t type,
        size_t data_size,
        const void* data);

    // TODO This should be private but it is still used in some code paths
    //  that could be painful to update.
    static gaia_ptr_t create(
        common::gaia_id_t id,
        common::gaia_type_t type,
        common::reference_offset_t num_refs,
        size_t data_size,
        const void* data);

    static gaia_ptr_t open(
        common::gaia_id_t id);

    // Removes the database record at the given pointer object. Throws
    // exceptions in case of referential integrity violation.
    static void remove(gaia_ptr_t& object);

    gaia_ptr_t& update_payload(size_t data_size, const void* data);

    static gaia_ptr_t find_first(common::gaia_type_t type);
    gaia_ptr_t find_next() const;

    inline bool is_null() const;

    inline common::gaia_id_t id() const;
    inline common::gaia_type_t type() const;
    inline char* data() const;
    inline size_t data_size() const;
    inline common::gaia_id_t* references() const;
    inline common::reference_offset_t num_references() const;

    db_object_t* to_ptr() const;
    gaia_offset_t to_offset() const;

    /**
     * Returns an iterator representing a server-side cursor over all objects of the given type.
     */
    static common::iterators::generator_iterator_t<gaia_ptr_t> find_all_iterator(
        common::gaia_type_t type);

    /**
     * Returns a range representing a server-side cursor over all objects of the given type.
     */
    static common::iterators::generator_range_t<gaia_ptr_t> find_all_range(
        common::gaia_type_t type);

    /**
     * Adds a child reference to a parent object. All the pointers involved in the relationship
     * will be updated, not only first_child_offset.
     *
     * Calling `parent.add_child_reference(child)` is the same as calling
     * `child.add_parent_reference(parent)`.
     *
     * @param child_id The id of the children.
     * @param first_child_offset The offset in the references array where the pointer to the child is placed.
     * @throws Exceptions there is no relationship between the parent and the child or if other
     *         integrity constraints are violated.
     */
    bool add_child_reference(common::gaia_id_t child_id, common::reference_offset_t first_child_offset);

    /**
     * Add a parent reference to a child object. All the pointers involved in the relationship
     * will be updated, not only parent_offset.
     *
     * Note: Children objects have 2 pointers per relationship (next_child_offset, parent_offset)
     * only one (parent_offset) is used to denote the relationship with parent.
     *
     * @param parent_id The id of the parent
     * @param parent_offset The offset in the references array where the pointer to the parent is placed.
     * @throws Exceptions there is no relationship between the parent and the child  or if other
     *         integrity constraints are violated.
     */
    bool add_parent_reference(common::gaia_id_t parent_id, common::reference_offset_t parent_offset);

    /**
     * Removes a child reference from a parent object. Without an index this operation
     * could have O(n) time complexity where n is the number of children.
     *
     * All the pointers involved in the relationship will be updated, not only first_child_offset.
     *
     * @param child_id The id of the children to be removed.
     * @param first_child_offset The offset, in the references array, of the pointer to the first child.
     */
    bool remove_child_reference(common::gaia_id_t child_id, common::reference_offset_t first_child_offset, bool for_drop = false);

    /**
     * Removes a parent reference from a child object at the given parent offset.
     *
     * @param parent_offset The offset, in the references array, of the pointer to the parent.
     */
    bool remove_parent_reference(common::reference_offset_t parent_offset, bool for_drop = false);

    /**
     * Update the parent reference with the given new_parent_id. If the this object does not
     * have a parent for the relationship denoted by parent_offset, it will just create the
     * relationship.
     *
     * All the pointers involved in the relationship will be updated, not only parent_offset.
     *
     * @param new_parent_id The id of the new parent.
     * @param parent_offset The offset, in the references array, of the pointer to the parent.
     */
    bool update_parent_reference(common::gaia_id_t new_parent_id, common::reference_offset_t parent_offset);

    // Delete the database record at the pointer. This method will not check
    // referential integrity violation for the deletion. Use 'remove()' instead
    // if you want referential integrity to be respected.
    void reset(bool for_drop = false);

protected:
    void allocate(size_t size);

    inline bool is(common::gaia_type_t type) const;

    gaia_ptr_t find_next(common::gaia_type_t type) const;

private:
    void clone_no_txn();

    void create_insert_trigger(common::gaia_type_t type, common::gaia_id_t id);

    static std::shared_ptr<common::iterators::generator_t<common::gaia_id_t>> get_id_generator_for_type(common::gaia_type_t type);

    static bool update_parent_reference(
        common::gaia_id_t child_id,
        common::gaia_type_t child_type,
        common::gaia_id_t* child_references,
        common::gaia_id_t new_parent_id,
        common::reference_offset_t parent_offset);

    /**
     * Try to auto connect a record to matching parent side record(s).
     *
     * @param child_id The record id
     * @param child_type The record type
     * @param child_type The record id of the child table type
     * @param child_references The record references
     * @param candidate_fields The list of candidate fields' positions.
     */
    static void auto_connect_to_parent(
        common::gaia_id_t child_id,
        common::gaia_type_t child_type,
        common::gaia_id_t child_type_id,
        common::gaia_id_t* child_references,
        const uint8_t* child_payload,
        const common::field_position_list_t& candidate_fields);

    static void auto_connect_to_parent(
        common::gaia_id_t child_id,
        common::gaia_type_t child_type,
        common::gaia_id_t* child_references,
        const uint8_t* child_payload);

private:
    gaia_locator_t m_locator{c_invalid_gaia_locator};
};

class gaia_ptr_generator_t : public common::iterators::generator_t<gaia_ptr_t>
{
public:
    explicit gaia_ptr_generator_t(std::shared_ptr<common::iterators::generator_t<common::gaia_id_t>> id_generator);
    std::optional<gaia_ptr_t> operator()() final;

private:
    std::shared_ptr<common::iterators::generator_t<common::gaia_id_t>> m_id_generator;
};

#include "gaia_ptr.inc"

} // namespace db
} // namespace gaia
