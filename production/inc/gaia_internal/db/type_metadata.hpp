/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

#include "gaia_internal/db/gaia_relationships.hpp"

namespace gaia
{
namespace db
{

/**
 * Contains metadata about a specific gaia type.
 *
 * Notes on concurrency: We do not expect the schema change at runtime yet.
 * This structure though, can change at runtime (possibly by multiple
 * threads) because it is lazy loaded by the type_registry_t.
 */
class type_metadata_t
{
    friend class type_registry_t;

public:
    explicit type_metadata_t(gaia::common::gaia_type_t type)
        : m_type(type), m_initialized(false){};

    gaia::common::gaia_type_t get_type() const;

    /**
     * Find a relationship from the parent side.
     */
    std::optional<relationship_t> find_parent_relationship(gaia::common::reference_offset_t first_child_offset) const;

    /**
     * Find a relationship from the child side.
     */
    std::optional<relationship_t> find_child_relationship(gaia::common::reference_offset_t parent_offset) const;

    /**
     * The total number of references this type has.
     */
    gaia::common::reference_offset_t references_count() const;

    // TODO the two following function should be called only by the registry.
    //  Need to figure the best way to do so since these are used in tests too

    /**
     * Mark this type as the parent side of the relationship.
     * The relationship_t object will be stored in the child metadata as well.
     */
    void add_parent_relationship(const std::shared_ptr<relationship_t>& relationship);

    /**
     * Mark this type as the child side of the relationship.
     * The relationship_t object will be stored in the parent metadata as well.
     */
    void add_child_relationship(const std::shared_ptr<relationship_t>& relationship);

private:
    const gaia::common::gaia_type_t m_type;

    // type_registry_t creates the instances of this object. Instances can be partially created
    // to avoid traversing the entire dependency graph when modeling relationships.
    std::atomic_bool m_initialized;

    mutable std::shared_mutex m_metadata_lock;

    // The relationship_t objects are shared between the parent and the child side of the relationship.
    std::unordered_map<gaia::common::reference_offset_t, std::shared_ptr<relationship_t>> m_parent_relationships;
    std::unordered_map<gaia::common::reference_offset_t, std::shared_ptr<relationship_t>> m_child_relationships;

    // The total number of reference slots this type has.
    common::reference_offset_t m_reference_count{0};

    bool is_initialized();
    void mark_as_initialized();
};

/**
 * Creates and maintains the instances of type_metadata_t and manages their lifecycle.
 * Instances of type_metadata_t are lazily created the first time the corresponding
 * gaia_type_t is accessed.
 *
 * Note: this class assumes that the Catalog cannot change during the execution of
 * a Gaia application. Changing the Catalog leads to a system restart and a recompilation
 * of the DAC classes. This will change after 11/2020 release.
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
     * Checks the existence of a given type in the metadata.
     */
    bool exists(gaia::common::gaia_type_t type) const;

    /**
     * Returns an instance of type_metadata_t. If no metadata exists for the
     * given type, a new instance is created loading the data from the catalog.
     *
     * The registry owns the lifecycle of this object.
     */
    const type_metadata_t& get(gaia::common::gaia_type_t type);

    // TESTING

    /**
     * FOR TESTING. Allows cleaning the registry between tests.
     * Calls init().
     */
    void clear();

    /**
     * Creates an instance of type_metadata_t in the registry skipping the Catalog.
     */
    type_metadata_t& test_get_or_create(gaia::common::gaia_type_t type_id);

private:
    type_registry_t()
    {
        init();
    }

    std::unordered_map<gaia::common::gaia_type_t, std::unique_ptr<type_metadata_t>> m_metadata_registry;

    // Ensures exclusive access to the registry.
    mutable std::shared_mutex m_registry_lock;

    /**
     * Initializes the registry by adding all the system tables (gaia_table, gaia_field, etc..)
     * avoiding failures when such tables are inserted in the catalog.
     */
    void init();

    /**
     * Returns the gaia_id_t of the table that holds the information about the given gaia_type_t
     */
    common::gaia_id_t get_record_id(gaia::common::gaia_type_t);

    /**
     * Creates an instance of type_metadata_t fetching the information from the Catalog.
     */
    type_metadata_t& create(gaia::common::gaia_type_t type);

    type_metadata_t& get_or_create_no_lock(gaia::common::gaia_type_t type);
};

} // namespace db
} // namespace gaia
