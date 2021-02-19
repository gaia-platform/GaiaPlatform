/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "flatbuffers/reflection.h"

#include "gaia/common.hpp"

namespace gaia
{
namespace db
{
namespace payload_types
{

typedef std::unordered_map<gaia::common::field_position_t, const reflection::Field*> field_map_t;

// A type information instance stores all information needed
// to deserialize or serialize data of that type.
//
// It stores all Field descriptions for a given type,
// indexed by the corresponding field id values.
//
// We store direct pointers of both the binary schema and the serialization
// template in catalog for the type.
class type_information_t
{
    friend class auto_type_information_t;
    friend class type_cache_t;

public:
    type_information_t() = default;

    // Set the binary schema for our type.
    void set_binary_schema(const uint8_t* binary_schema, size_t binary_schema_size);

    // Set the serialization template for our type.
    void set_serialization_template(const uint8_t* serialization_template, size_t serialization_template_size);

    // Insert information about a field in the field map.
    // This is used during construction of the field map.
    void set_field(gaia::common::field_position_t field_position, const reflection::Field* field);

    // Return a direct pointer to the binary schema in catalog.
    //
    // This is only needed during initialization so that the Field information
    // references the data in our copy.
    const uint8_t* get_binary_schema() const;

    // Return the size of the binary schema.
    size_t get_binary_schema_size() const;

    // Return a direct pointer to the serialization template in catalog.
    const uint8_t* get_serialization_template() const;

    // Return the size of the serialization template.
    size_t get_serialization_template_size() const;

    // Return field information if the field could be found or nullptr otherwise.
    //
    // Because the field information is retrieved by direct access to the binary schema,
    // we need to maintain a read lock while it is being used,
    // to prevent it from being changed.
    const reflection::Field* get_field(gaia::common::field_position_t field_position) const;

    // Return the size of the internal map.
    size_t get_field_count() const;

protected:
    // Reading these entries will hold read locks, whereas update operations will request exclusive locks.
    // Operations that require exclusive locking are meant to be rare.
    mutable std::shared_mutex m_lock;

    // Direct pointer to the binary schema in catalog for this type.
    const uint8_t* m_binary_schema;
    size_t m_binary_schema_size;

    // Direct pointer to the serialization template in catalog for this type.
    const uint8_t* m_serialization_template;
    size_t m_serialization_template_size;

    // A map used to directly reference Field information in the binary schema.
    field_map_t m_field_map;
};

typedef std::unordered_map<gaia::common::gaia_type_t, std::shared_ptr<const type_information_t>> type_information_map_t;

class auto_type_information_t;

// The type cache stores type_information_t instances for all managed types.
// The type_information_t instances are indexed by their corresponding type id.
class type_cache_t
{
protected:
    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    type_cache_t(const type_cache_t&) = delete;
    type_cache_t& operator=(const type_cache_t&) = delete;

    // type_cache_t is a singleton, so its constructor is not public.
    type_cache_t() = default;

public:
    // Return a pointer to the singleton instance.
    static type_cache_t* get();

    // To ensure that the returned type information instance continues to be valid,
    // this method needs to hold a read lock on it.
    // To ensure the release of that lock once the type information is no longer used,
    // it is returned in an auto_type_information_t wrapper that will release the lock
    // at the time the wrapper gets destroyed.
    // It returns true if the information was retrieved and false otherwise.
    bool get_type_information(
        gaia::common::gaia_type_t type_id,
        auto_type_information_t& auto_type_information) const;

    // This method should be called whenever the information for a type is being changed.
    // It returns true if the entry was found and deleted, and false if it was not found
    // (another thread may have deleted it first or the information may never have been cached at all).
    bool remove_type_information(gaia::common::gaia_type_t type_id);

    // This method should be used to load new type information in the cache.
    // It expects the cache to contain no data for the type.
    // It returns true if the cache was updated and false if an entry for the type was found to exist already.
    // When data is inserted into the cache, the unique_ptr will lose its ownership.
    bool set_type_information(
        gaia::common::gaia_type_t type_id,
        std::unique_ptr<type_information_t>& type_information);

    // Return the size of the internal map.
    size_t size() const;

protected:
    // The singleton instance.
    static type_cache_t s_type_cache;

    // Reads from cache will initially acquire read locks, whereas update operations will request exclusive locks.
    // Operations that require exclusive locking are meant to be rare.
    // We can further improve implementation by preloading type information at system startup.
    mutable std::shared_mutex m_lock;

    // The map used by the type cache.
    type_information_map_t m_type_map;
};

// A class for automatically releasing the read lock taken while reading from the cache.
class auto_type_information_t
{
    friend class type_cache_t;

public:
    auto_type_information_t() = default;
    ~auto_type_information_t();

    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    auto_type_information_t(const auto_type_information_t&) = delete;
    auto_type_information_t& operator=(const auto_type_information_t&) = delete;

    const type_information_t* get();

protected:
    std::shared_ptr<const type_information_t> m_type_information;

    void set(const std::shared_ptr<const type_information_t>& type_information);
};

} // namespace payload_types
} // namespace db
} // namespace gaia
