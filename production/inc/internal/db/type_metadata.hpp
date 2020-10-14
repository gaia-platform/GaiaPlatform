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

//#include "catalog_manager.hpp"
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

    /**
     * Find a relationship from the parent side.
     */
    relationship_t* find_parent_relationship(reference_offset_t first_child_offset) const;

    /**
     * Find a relationship from the child side.
     */
    relationship_t* find_child_relationship(reference_offset_t parent_offset) const;

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
 * Maintain the instances of type_metadata_t.
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
     * Returns an instance of type_metadata_t. If no metadata exists for the
     * given type, a new instance is created and returned. Clients are allowed
     * to modify the returned metadata, although the registry keeps ownership.
     */
    type_metadata_t& get(gaia_type_t type);

    // TODO the two following function should be called only by the registry.
    //  Need to figure the best way to do so (friend class?)

    /**
     * Add metadata to the registry
     *
     * @throws metadata_not_found if there is no metadata for the given type.
     */
    void add(type_metadata_t* metadata);

    /**
     * Update the metadata
     *
     * @param type the type that is being updated
     * @param update_op update operation run in a thread safe fashion
     */
    void update(gaia_type_t type, std::function<void(type_metadata_t&)> update_op);

    // TESTING

    /**
     * FOR TESTING. This method allow cleaning the registry between tests.
     */
    void clear();

private:
    type_registry_t() = default;

    unordered_map<gaia_type_t, unique_ptr<type_metadata_t>> m_metadata_registry;

    //ensures exclusive access to the registry
    shared_mutex m_registry_lock;
};

///**
// * Facilitate the creation of relationship objects and their insertion into
// * the registry.
// */
//class relationship_builder_t
//{
//public:
//    static relationship_builder_t one_to_one()
//    {
//        auto metadata = relationship_builder_t();
//        metadata.m_cardinality = cardinality_t::one;
//        return metadata;
//    }
//
//    static relationship_builder_t one_to_many()
//    {
//        auto metadata = relationship_builder_t();
//        metadata.m_cardinality = cardinality_t::many;
//        return metadata;
//    }
//
//    relationship_builder_t& parent(gaia_type_t parent)
//    {
//        this->m_parent_type = parent;
//        return *this;
//    }
//
//    relationship_builder_t& child(gaia_type_t child)
//    {
//        this->m_child_type = child;
//        return *this;
//    }
//
//    relationship_builder_t& first_child_offset(reference_offset_t first_child_offset)
//    {
//        this->m_first_child_offset = first_child_offset;
//        return *this;
//    }
//
//    relationship_builder_t& next_child_offset(reference_offset_t next_child_offset)
//    {
//        this->m_next_child_offset = next_child_offset;
//        return *this;
//    }
//
//    relationship_builder_t& parent_offset(reference_offset_t parent_offset)
//    {
//        this->m_parent_offset = parent_offset;
//        return *this;
//    }
//
//    relationship_builder_t& require_parent(bool parent_required)
//    {
//        this->m_parent_required = parent_required;
//        return *this;
//    }
//
//    // Creates all the object necessary to describe the relationship and
//    // updates the type registry.
//    void create_relationship()
//    {
//        if (m_parent_type == INVALID_GAIA_TYPE)
//        {
//            throw invalid_argument("parent_type must be set");
//        }
//
//        if (m_child_type == INVALID_GAIA_TYPE)
//        {
//            throw invalid_argument("child_type must be set");
//        }
//
//        if (m_cardinality == cardinality_t::not_set)
//        {
//            throw invalid_argument("cardinality must be set");
//        }
//
//        if (m_first_child_offset == INVALID_REFERENCE_OFFSET)
//        {
//            throw invalid_argument("first_child_offset must be set");
//        }
//
//        if (m_next_child_offset == INVALID_REFERENCE_OFFSET)
//        {
//            throw invalid_argument("next_child_offset must be set");
//        }
//
//        if (m_parent_offset == INVALID_REFERENCE_OFFSET)
//        {
//            throw invalid_argument("parent_offset must be set");
//        }
//
//        auto rel = make_shared<relationship_t>(relationship_t{
//            .parent_type = this->m_parent_type,
//            .child_type = this->m_child_type,
//            .first_child_offset = this->m_first_child_offset,
//            .next_child_offset = this->m_next_child_offset,
//            .parent_offset = this->m_parent_offset,
//            .cardinality = this->m_cardinality,
//            .parent_required = this->m_parent_required});
//
//        auto& parent_meta = type_registry_t::instance().get_or_create(m_parent_type);
//        parent_meta.add_parent_relationship(m_first_child_offset, rel);
//
//        auto& child_meta = type_registry_t::instance().get_or_create(m_child_type);
//        child_meta.add_child_relationship(m_parent_offset, rel);
//    }
//
//private:
//    relationship_builder_t() = default;
//
//    // mandatory values
//    cardinality_t m_cardinality = cardinality_t::not_set;
//    gaia_type_t m_parent_type = INVALID_GAIA_TYPE;
//    gaia_type_t m_child_type = INVALID_GAIA_TYPE;
//
//    // default values add methods for those.
//    bool m_parent_required = false;
//    reference_offset_t m_first_child_offset = INVALID_REFERENCE_OFFSET;
//    reference_offset_t m_next_child_offset = INVALID_REFERENCE_OFFSET;
//    reference_offset_t m_parent_offset = INVALID_REFERENCE_OFFSET;
//};

} // namespace gaia::db
