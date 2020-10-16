/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <functional>
#include <memory>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>

#include "gaia_exception.hpp"
#include "gaia_relationships.hpp"

using namespace std;
using namespace gaia::common;

namespace gaia::db
{

/**
 * Contains metadata about a specific gaia type.
 */
class type_metadata_t
{
public:
    explicit type_metadata_t(gaia_type_t type)
        : m_type(type){};

    gaia_type_t get_type() const;

    /**
     * Find a relationship from the parent side.
     */
    relationship_t* find_parent_relationship(reference_offset_t first_child_offset) const;

    /**
     * Find a relationship from the child side.
     */
    relationship_t* find_child_relationship(reference_offset_t parent_offset) const;

    /**
     * Counts the number of reference this type has both as parent and child.
     * Note: child references count 2X, since 2 pointers are necessary to express them.
     */
    size_t num_references();

    // TODO the two following function should be called only by the registry.
    //  Need to figure the best way to do so (friend class?)

    /**
     * Mark this type as the parent side of the relationship.
     * The relationship_t object will be stored in the child metadata as well.
     */
    void add_parent_relationship(reference_offset_t first_child, const shared_ptr<relationship_t>& relationship);

    /**
     * Mark this type as the child side of the relationship.
     * The relationship_t object will be stored in the parent metadata as well.
     */
    void add_child_relationship(reference_offset_t parent, const shared_ptr<relationship_t>& relationship);

private:
    gaia_type_t m_type;

    // the relationship_t objects are shared between the parent and the child side of the relationship.
    unordered_map<reference_offset_t, shared_ptr<relationship_t>> m_parent_relationships;
    unordered_map<reference_offset_t, shared_ptr<relationship_t>> m_child_relationships;
};

class duplicate_metadata : public gaia_exception
{
public:
    explicit duplicate_metadata(const gaia_type_t type)
    {
        stringstream message;
        message << "Metadata already existent for Gaia type \"" << type << "\"";
        m_message = message.str();
    }
};

class metadata_not_found : public gaia_exception
{
public:
    explicit metadata_not_found(const gaia_type_t type)
    {
        stringstream message;
        message << "Metadata does not exist for Gaia type \"" << type << "\"";
        m_message = message.str();
    }
};

/**
 * Maintain the instances of type_metadata_t and manages their lifecycle.
 * type_metadata_t should be created/edited/deleted only by the catalog (with
 * the exception of tests).
 */
class type_registry_t
{
public:
    type_registry_t(const type_registry_t&) = delete;
    type_registry_t& operator=(const type_registry_t&) = delete;
    type_registry_t(type_registry_t&&) = delete;
    type_registry_t& operator=(type_registry_t&&) = delete;

    static type_registry_t& instance()
    {
        static type_registry_t type_registry;
        return type_registry;
    }

    /**
     * Returns an instance of type_metadata_t. If no metadata exists for the
     * given type, throws an exception. It should be used in those when
     * the presence of the metadata is expected (eg. gaia_ptr).
     *
     * This method acquires a shared lock thus should be faster than get_or_create.
     *
     * @throws metadata_not_found
     */
    type_metadata_t& get(gaia_type_t type);

    // TODO the two following function should be called only by the registry.
    //  Need to figure the best way to do so (friend class?)

    /**
     * Returns an instance of type_metadata_t. If no metadata exists for the
     * given type, a new instance is created and returned.
     *
     * Clients are NOT allowed to modify the returned metadata, use update()
     * for this purpose.
     *
     * The registry owns the lifecycle of this object.
     */
    type_metadata_t& get_or_create(gaia_type_t type);

    /**
     * Add metadata to the registry. The registry owns the lifecycle of this object.
     */
    // TODO should this be unique_ptr to communicate the transfer of ownership?
    void add(type_metadata_t* metadata);

    /**
     * Update the metadata.
     *
     * @param type the type that is being updated
     * @param update_op update operation run in a thread safe fashion
     * @throws metadata_not_found
     */
    void update(gaia_type_t type, std::function<void(type_metadata_t&)> update_op);

    /**
     * Removes the medata.
     *
     * @throws metadata_not_found
     */
    void remove(gaia_type_t type);

    // TESTING

    /**
     * FOR TESTING. Allow cleaning the registry between tests.
     */
    void clear();

private:
    type_registry_t() = default;

    unordered_map<gaia_type_t, unique_ptr<type_metadata_t>> m_metadata_registry;

    //ensures exclusive access to the registry
    shared_mutex m_registry_lock;
};

} // namespace gaia::db
