/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

#include <shared_mutex>
#include <unordered_map>

#include "gaia/common.hpp"

#include "record_list.hpp"

namespace gaia
{
namespace db
{
namespace storage
{

class record_list_manager_t
{
protected:
    // Do not allow copies to be made;
    // disable copy constructor and assignment operator.
    record_list_manager_t(const record_list_manager_t&) = delete;
    record_list_manager_t& operator=(const record_list_manager_t&) = delete;

    // record_list_manager_t is a singleton, so its constructor is not public.
    record_list_manager_t() = default;

public:
    // Return a pointer to the singleton instance.
    static record_list_manager_t* get();

    // Return a pointer to the list instance.
    std::shared_ptr<record_list_t> get_record_list(
        gaia::common::gaia_type_t type_id);

    // Reset a record list.
    // This method should be used after the type is deleted.
    // It permits the reuse of the gaia_type value.
    // If reuse is not desired, then this can be changed to just delete the list.
    void reset_record_list(
        gaia::common::gaia_type_t type_id);

    // Return the size of the internal map.
    size_t size() const;

    // Clear the record_list data tracked by the manager.
    // This is provided for scenarios in which the server state needs to be cleared.
    void clear();

protected:
    std::shared_ptr<record_list_t> get_record_list_internal(
        gaia::common::gaia_type_t type_id) const;

    void create_record_list(
        gaia::common::gaia_type_t type_id);

protected:
    static constexpr size_t c_record_list_range_size = 256;

    // The singleton instance.
    static record_list_manager_t s_record_list_manager;

    // Reads from the map will initially acquire read locks, whereas update operations will request exclusive locks.
    // Operations that require exclusive locking are meant to be rare.
    mutable std::shared_mutex m_lock;

    // The map used by the record_list manager.
    std::unordered_map<gaia::common::gaia_type_t, std::shared_ptr<record_list_t>> m_record_list_map;
};

} // namespace storage
} // namespace db
} // namespace gaia
