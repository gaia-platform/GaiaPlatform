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
 * Contains metadata about a gaia container.
 */
class container_metadata_t {
public:
    explicit container_metadata_t(gaia_container_id_t container_id) : m_container_id(container_id) {}

    gaia_container_id_t get_container_id() const;
    relationship_t* find_parent_relationship(reference_offset_t first_child_offset) const;
    relationship_t* find_child_relationship(reference_offset_t parent_offset) const;

    /**
     * Mark this container as the parent side of the relationship.
     * The relationship_t object will be stored in the child metadata as well.
     */
    void add_parent_relationship(reference_offset_t first_child, const shared_ptr<relationship_t>& relationship);

    /**
     * Mark this container as the child side of the relationship.
     * The relationship_t object will be stored in the parent metadata as well.
     */
    void add_child_relationship(reference_offset_t parent, const shared_ptr<relationship_t>& relationship);

    /**
     * Counts the number of reference this container has, both as parent and child.
     * Note: child references count 2X, since 2 pointers are necessary to express them.
     */
    size_t num_references();

private:
    gaia_container_id_t m_container_id;

    // the relationship_t objects are shared between the parent and the child side of the relationship.
    unordered_map<reference_offset_t, shared_ptr<relationship_t>> m_parent_relationships;
    unordered_map<reference_offset_t, shared_ptr<relationship_t>> m_child_relationships;
};

class duplicate_metadata : public gaia_exception {
public:
    explicit duplicate_metadata(const gaia_container_id_t container_id) {
        stringstream message;
        message << "Metadata already existent for Gaia container id \"" << container_id << "\"";
        m_message = message.str();
    }
};

/**
 * Maintain the instances of container_metadata_t. This class creates and owns
 * the metadata.
 */
class container_registry_t {
public:
    container_registry_t(const container_registry_t&) = delete;
    container_registry_t& operator=(const container_registry_t&) = delete;
    container_registry_t(container_registry_t&&) = delete;
    container_registry_t& operator=(container_registry_t&&) = delete;

    static container_registry_t& instance() {
        static container_registry_t container_registry;
        return container_registry;
    }

    /**
     * FOR TESTING. There are situation where its not possible
     * to use a mocked version of this class (gaia_ptr). This
     * method allow cleaning the registry between tests.
     */
    void clear();

    /**
     * Returns an instance of container_metadata_t. If no metadata exists for the
     * given container_id, a new instance is created and returned. Clients are allowed
     * to modify the returned metadata, although the registry keeps ownership.
     */
    container_metadata_t& get_or_create(gaia_container_id_t container_id);

private:
    container_registry_t() = default;

    unordered_map<gaia_container_id_t, container_metadata_t> m_metadata_registry;

    //ensures exclusive access to the registry
    shared_mutex m_registry_lock;
};

} // namespace gaia::db
