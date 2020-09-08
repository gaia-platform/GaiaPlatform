/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <map>
#include <optional>
#include <memory>
#include <sstream>

#include "gaia_exception.hpp"
#include "gaia_relations.hpp"

using namespace std;
using namespace gaia::common;

namespace gaia::db {

/**
 * Contains metadata about a specific gaia type.
 *
 * This could be counter-intuitive because one of the Gaia design
 * pillars is that the SE is agnostic to the Catalog.
 * The SE though, needs to access some of the information about the
 * types to provide Referential Integrity, access Index metadata etc...
 */
class type_metadata_t {
  public:
    explicit type_metadata_t(gaia_type_t type) : m_type(type) {}

    gaia_type_t get_type() const;
    relationship_t* find_parent_relationship(relationship_offset_t offset) const;
    relationship_t* find_child_relationship(relationship_offset_t offset) const;

    void add_parent_relationship(relationship_offset_t first_child, const shared_ptr<relationship_t>& relationship);
    void add_child_relationship(relationship_offset_t parent, const shared_ptr<relationship_t>& relationship);

    /**
     * Counts the number of reference this type has both as parent and child.
     * Note: child references count 2X, since 2 pointers are necessary to express them.
     */
    size_t num_references();

  private:
    gaia_type_t m_type;

    // the relationship_t objects are shared between the parent and the child side of the relationship.
    map<relationship_offset_t, shared_ptr<relationship_t>> m_parent_relationships;
    map<relationship_offset_t, shared_ptr<relationship_t>> m_child_relationships;
};

class duplicated_metadata : public gaia_exception {
  public:
    explicit duplicated_metadata(const gaia_type_t type) {
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
    // TODO: consider not using singleton. Singleton is currently used to avoid
    //  adding a type_registry_t& reference to gaia_ptr. The downside is that
    //  tests have to remember to "clear()" it every time. If we allow having
    //  a reference to type_registry_t within gaia_ptr we could get rid of
    //  singleton.
    type_registry_t(const type_registry_t&) = delete;
    type_registry_t& operator=(const type_registry_t&) = delete;
    type_registry_t(type_registry_t&&) = delete;
    type_registry_t& operator=(type_registry_t&&) = delete;

    static type_registry_t& instance() {
        static type_registry_t type_registry;
        return type_registry;
    }

    /**
     * Use instance() in prod.
     */
    type_registry_t() = default;

    /**
     * FOR TESTING. There are situation where its not possible
     * to use a mocked version of this class (gaia_ptr). This
     * method allow cleaning the registry between tests.
     */
    void clear();

    /**
     * Returns an instance of type_metadata_t. If no metadata existed for the
     * given type, a new instance is created an returned. Clients are allowed
     * to modify the returned metadata, although the registry keeps ownership.
     *
     * NOTE: This code creates instances of type_metadata_t is not thread-safe.
     * This should be acceptable because all the types are initialized
     * in the Catalog during the startup. This assumption could change though.
     */
    type_metadata_t& get_or_create(gaia_type_t type);

  private:
    map<gaia_type_t, type_metadata_t> m_metadata_registry;

};

} // namespace gaia::db
