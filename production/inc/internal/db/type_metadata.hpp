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
 * types to provide Referential Integrity, access Index metadata etc..
 */
class type_metadata_t {
  public:
    explicit type_metadata_t(gaia_type_t type) : m_type(type), m_parent_relations(), m_child_relations() {}

    gaia_type_t get_type() const;
    optional<relation_t> find_parent_relation(relation_offset_t offset) const;
    optional<relation_t> find_child_relation(relation_offset_t offset) const;

    void add_child_relation(relation_offset_t offset, shared_ptr<relation_t> relation);
    void add_parent_relation(relation_offset_t offset, shared_ptr<relation_t> relation);

    /**
     * Counts the number of reference this type has both as parent and child.
     * Note: child references count 2X, since 2 pointers are necessary to express them.
     */
    size_t num_references();

  private:
    gaia_type_t m_type;

    // the relation_t objects are shared between the parent and the child side of the relation.
    map<relation_offset_t, shared_ptr<relation_t>> m_parent_relations;
    map<relation_offset_t, shared_ptr<relation_t>> m_child_relations;
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
    // singleton stuff, I really hope to get rid of this.
    type_registry_t(const type_registry_t &) = delete;
    type_registry_t &operator=(const type_registry_t &) = delete;
    type_registry_t(type_registry_t &&) = delete;
    type_registry_t &operator=(type_registry_t &&) = delete;

    static type_registry_t &instance() {
        static type_registry_t type_registry;
        return type_registry;
    }

    /**
     * VISIBLE FOR TESTING. Use instance() in prod.
     */
    type_registry_t() : m_metadata_registry(){};

    /**
     * Returns an instance of type_metadata_t. If no metadata existed for the
     * given type, a new instance is created an returned. Clients are allowed
     * to modify the returned metadata, although the registry keeps ownership.
     */
    type_metadata_t &get_metadata(gaia_type_t type);

    /**
     * FOR TESTING. There are situation where its not possible
     * to use a mocked version of this class (gaia_ptr). This
     * method allow cleaning the registry between tests.
     */
    void clear();

  private:
    map<gaia_type_t, unique_ptr<type_metadata_t>> m_metadata_registry;
};

} // namespace gaia::db
