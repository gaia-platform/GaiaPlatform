/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unordered_map>
#include <optional>
#include <memory>
#include <sstream>
#include <shared_mutex>

#include "gaia_exception.hpp"
#include "gaia_relationships.hpp"

using namespace std;
using namespace gaia::common;

namespace gaia::db {

/**
 * Contains metadata about a specific gaia type.
 */
class type_metadata_t {
public:
    explicit type_metadata_t(gaia_type_t type) : m_type(type) {}

    gaia_type_t get_type() const;
    relationship_t* find_parent_relationship(reference_offset_t first_child_offset) const;
    relationship_t* find_child_relationship(reference_offset_t parent_offset) const;

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

    /**
     * Counts the number of reference this type has both as parent and child.
     * Note: child references count 2X, since 2 pointers are necessary to express them.
     */
    size_t num_references();

private:
    gaia_type_t m_type;

    // the relationship_t objects are shared between the parent and the child side of the relationship.
    unordered_map<reference_offset_t, shared_ptr<relationship_t>> m_parent_relationships;
    unordered_map<reference_offset_t, shared_ptr<relationship_t>> m_child_relationships;
};

class duplicate_metadata : public gaia_exception {
public:
    explicit duplicate_metadata(const gaia_type_t type) {
        stringstream message;
        message << "Metadata already existent for Gaia type \"" << type << "\"";
        m_message = message.str();
    }
};

/**
 * Maintain the instances of type_metadata_t. This class creates and owns
 * the metadata.
 */
class type_registry_t {
public:
    type_registry_t(const type_registry_t&) = delete;
    type_registry_t& operator=(const type_registry_t&) = delete;
    type_registry_t(type_registry_t&&) = delete;
    type_registry_t& operator=(type_registry_t&&) = delete;

    static type_registry_t& instance() {
        static type_registry_t type_registry;
        return type_registry;
    }

    /**
     * FOR TESTING. There are situation where its not possible
     * to use a mocked version of this class (gaia_ptr). This
     * method allow cleaning the registry between tests.
     */
    void clear();

    /**
     * Returns an instance of type_metadata_t. If no metadata exists for the
     * given type, a new instance is created and returned. Clients are allowed
     * to modify the returned metadata, although the registry keeps ownership.
     */
    type_metadata_t& get_or_create(gaia_type_t type);

private:
    type_registry_t() = default;

    unordered_map<gaia_type_t, type_metadata_t> m_metadata_registry;

    //ensures exclusive access to the registry
    shared_mutex m_registry_lock;
};

} // namespace gaia::db
