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

#include "gaia_common.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace payload_types
{

typedef std::unordered_map<field_position_t, const reflection::Field*> field_map_t;

// A type information instance stores all information needed
// to deserialize or serialize data of that type.
// It stores all Field descriptions for a given type,
// indexed by the corresponding field id values.
class type_information_t
{
public:
    type_information_t() = default;

    // Return a direct pointer to our copy of the binary schema.
    const uint8_t* get_raw_binary_schema() const;

    // Return a copy of our serialization template.
    std::vector<uint8_t> get_serialization_template() const;

    // Return field information if the field could be found or nullptr otherwise.
    //
    // Because the field information is retrieved by direct access to the binary schema,
    // we need to maintain a read lock while it is being used,
    // to prevent it from being changed.
    const reflection::Field* get_field(field_position_t field_position) const;

    // Set the binary schema for our type.
    void set_binary_schema(const std::vector<uint8_t>& binary_schema);

    // Set the serialization template for our type.
    void set_serialization_template(const std::vector<uint8_t>& serialization_template);

    // Insert information about a field in the field map.
    // This is used during construction of the field map.
    void set_field(field_position_t field_position, const reflection::Field* field);

    // Return the size of the internal map.
    size_t size();

protected:
    // The binary schema for this type.
    std::vector<uint8_t> m_binary_schema;

    // The serialization template for this type.
    std::vector<uint8_t> m_serialization_template;

    // A map used to directly reference Field information in the binary schema.
    field_map_t m_field_map;
};

typedef std::unordered_map<gaia_type_t, const type_information_t*> type_information_map_t;

class auto_type_information_t;

// The type cache stores type_information_t instances for all managed types.
// The type_information_t instances are indexed by their corresponding type id.
class type_cache_t
{
    friend class auto_type_information_t;

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
    // this method needs to hold a read lock on the type cache.
    // To ensure the release of that lock once the type information is no longer used,
    // it is returned in an auto_type_information_t wrapper that will release the lock
    // at the time the wrapper gets destroyed.
    void get_type_information(gaia_type_t type_id, auto_type_information_t& auto_type_information) const;

    // This method should be called whenever the information for a type is being changed.
    // It will return true if the entry was found and deleted, and false if it was not found
    // (another thread may have deleted it first or the information may never have been cached at all).
    bool remove_type_information(gaia_type_t type_id);

    // This method should be used to load new type information in the cache.
    // It expects the cache to contain no data for the type.
    // It returns true if the cache was updated and false if an entry for the type was found to exist already.
    bool set_type_information(gaia_type_t type_id, const type_information_t* type_information);

    // Return the size of the internal map.
    size_t size() const;

protected:
    // The singleton instance.
    static type_cache_t s_type_cache;

    // Reads from cache will hold read locks, whereas update operations will request exclusive locks.
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
    auto_type_information_t();
    ~auto_type_information_t();

    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    auto_type_information_t(const auto_type_information_t&) = delete;
    auto_type_information_t& operator=(const auto_type_information_t&) = delete;

    const type_information_t* get();

protected:
    const type_information_t* m_type_information;

    void set(const type_information_t* type_information);
};

} // namespace payload_types
} // namespace db
} // namespace gaia
