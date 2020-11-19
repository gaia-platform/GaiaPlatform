/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "db_types.hpp"
#include "generator_iterator.hpp"
#include "se_object.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{

class gaia_ptr
{
public:
    gaia_ptr() = default;

    bool operator==(const gaia_ptr& other) const
    {
        return m_locator == other.m_locator;
    }

    bool operator==(const std::nullptr_t) const
    {
        return to_ptr() == nullptr;
    }

    bool operator!=(const std::nullptr_t) const
    {
        return to_ptr() != nullptr;
    }

    explicit operator bool() const
    {
        return to_ptr() != nullptr;
    }

    static gaia_id_t generate_id();

    static gaia_ptr create(
        gaia_type_t type,
        size_t data_size,
        const void* data);

    static gaia_ptr create(
        gaia_id_t id,
        gaia_type_t type,
        size_t data_size,
        const void* data);

    static gaia_ptr create(
        gaia_id_t id,
        gaia_type_t type,
        size_t num_refs,
        size_t data_size,
        const void* data);

    static gaia_ptr open(
        gaia_id_t id)
    {
        return gaia_ptr(id);
    }

    // TODO this should either accept a gaia_id_t or be an instance method.
    static void remove(gaia_ptr& node);

    gaia_ptr& clone();

    gaia_ptr& update_payload(size_t data_size, const void* data);

    static gaia_ptr find_first(gaia_type_t type)
    {
        gaia_ptr ptr;
        ptr.m_locator = 1;

        if (!ptr.is(type))
        {
            ptr.find_next(type);
        }

        return ptr;
    }

    gaia_ptr find_next()
    {
        if (m_locator)
        {
            find_next(to_ptr()->type);
        }

        return *this;
    }

    gaia_ptr operator++()
    {
        if (m_locator)
        {
            find_next(to_ptr()->type);
        }
        return *this;
    }

    bool is_null() const
    {
        return m_locator == 0;
    }

    gaia_id_t id() const
    {
        return to_ptr()->id;
    }

    gaia_type_t type() const
    {
        return to_ptr()->type;
    }

    char* data() const
    {
        return data_size() ? const_cast<char*>(to_ptr()->data()) : nullptr;
    }

    size_t data_size() const
    {
        size_t total_len = to_ptr()->payload_size;
        size_t refs_len = to_ptr()->num_references * sizeof(gaia_id_t);
        size_t data_size = total_len - refs_len;
        return data_size;
    }

    gaia_id_t* references() const
    {
        return const_cast<gaia_id_t*>(to_ptr()->references());
    }

    size_t num_references() const
    {
        return to_ptr()->num_references;
    }

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
    void add_child_reference(gaia_id_t child_id, reference_offset_t first_child_offset);

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
    void add_parent_reference(gaia_id_t parent_id, reference_offset_t parent_offset);

    /**
     * Removes a child reference from a parent object. Without an index this operation
     * could have O(n) time complexity where n is the number of children.
     *
     * All the pointers involved in the relationship will be updated, not only first_child_offset.
     *
     * @param child_id The id of the children to be removed.
     * @param first_child_offset The offset, in the references array, of the pointer to the first child.
     */
    void remove_child_reference(gaia_id_t child_id, reference_offset_t first_child_offset);

    /**
     * Removes a parent reference from a child object. Without an index this operation
     * could have O(n) time complexity where n is the number of children.
     *
     * All the pointers involved in the relationship will be updated, not only parent_offset.
     *
     * @param parent_id The id of the parent to be removed.
     * @param parent_offset The offset, in the references array, of the pointer to the parent.
     */
    void remove_parent_reference(gaia_id_t parent_id, reference_offset_t parent_offset);

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
    void update_parent_reference(gaia_id_t new_parent_id, reference_offset_t parent_offset);

    // "function with deduced return type cannot be used before it is defined".
    // The function must be defined in the same translation unit where it is used,
    // and the only way to guarantee that for our clients is to define it in the
    // header file itself.

    /**
     * Returns an iterator representing a server-side cursor over all objects
     * of the given type. This is essentially a proof-of-concept for server-side
     * cursors, which will be extended to support server-side filters.
     */
    static auto find_all_iter(
        gaia_type_t type,
        std::function<bool(gaia_ptr)> user_predicate = [](gaia_ptr) { return true; })
    {
        // Get the gaia_id generator and wrap it in a gaia_ptr generator.
        std::function<std::optional<gaia_id_t>()> id_generator = get_id_generator_for_type(type);
        std::function<std::optional<gaia_ptr>()> gaia_ptr_generator = [id_generator]() -> std::optional<gaia_ptr> {
            std::optional<gaia_id_t> id_opt = id_generator();
            if (id_opt)
            {
                return gaia_ptr::open(*id_opt);
            }
            return std::nullopt;
        };
        // We need to construct an iterator from this generator rather than
        // directly constructing a range from the generator, because we need
        // to filter out values corresponding to deleted objects, and we can
        // do that only by supplying a predicate to the iterator.
        // REVIEW: this can filter out objects that do not exist in the client view,
        // but it cannot return objects that only exist in the client view.
        // That will require merging the client's transaction log.
        std::function<bool(gaia_ptr)> gaia_ptr_predicate = [user_predicate](gaia_ptr ptr) {
            return !ptr.is_null() && user_predicate(ptr);
        };
        auto gaia_ptr_iterator = gaia::common::iterators::generator_iterator_t(
            gaia_ptr_generator,
            gaia_ptr_predicate);
        return gaia_ptr_iterator;
    }

    /**
     * Returns a range representing a server-side cursor over all objects
     * of the given type. This is essentially a proof-of-concept for server-side
     * cursors, which will be extended to support server-side filters.
     */
    static auto find_all_range(
        gaia_type_t type,
        std::function<bool(gaia_ptr)> user_predicate = [](gaia_ptr) { return true; })
    {
        return gaia::common::iterators::range(find_all_iter(type, user_predicate));
    }

protected:
    explicit gaia_ptr(gaia_id_t id);

    gaia_ptr(gaia_id_t id, size_t size);

    void allocate(size_t size);

    se_object_t* to_ptr() const;

    gaia_offset_t to_offset() const;

    bool is(gaia_type_t type) const
    {
        return to_ptr() && to_ptr()->type == type;
    }

    void find_next(gaia_type_t type);

    void reset();

private:
    gaia_locator_t m_locator = {};

    void clone_no_txn();

    void create_insert_trigger(gaia_type_t type, gaia_id_t id);

    // This is just a trivial wrapper for a gaia::db::client API,
    // to avoid calling into SE client code from this header file.
    static std::function<std::optional<gaia_id_t>()> get_id_generator_for_type(gaia_type_t type);
};

} // namespace db
} // namespace gaia
