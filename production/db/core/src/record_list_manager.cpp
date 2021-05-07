/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "record_list_manager.hpp"

#include "gaia_internal/common/retail_assert.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;

namespace gaia
{
namespace db
{
namespace storage
{

record_list_manager_t record_list_manager_t::s_record_list_manager;

record_list_manager_t* record_list_manager_t::get()
{
    return &s_record_list_manager;
}

shared_ptr<record_list_t> record_list_manager_t::get_record_list(gaia_type_t type_id)
{
    shared_ptr<record_list_t> list = get_record_list_internal(type_id);

    if (!list)
    {
        create_record_list(type_id);
    }

    list = get_record_list_internal(type_id);

    ASSERT_POSTCONDITION(!!list, "Failed to obtain record list from record list manager!");

    return list;
}

shared_ptr<record_list_t> record_list_manager_t::get_record_list_internal(gaia_type_t type_id) const
{
    // We acquire a shared lock while we retrieve the record_list,
    // to ensure that the map is not being updated by another thread.
    shared_lock shared_lock(m_lock);

    auto iterator = m_record_list_map.find(type_id);

    if (iterator == m_record_list_map.end())
    {
        return nullptr;
    }

    return iterator->second;
}

void record_list_manager_t::create_record_list(gaia_type_t type_id)
{
    // We acquire a unique lock because we need to update the map.
    // Such operations should be rare.
    unique_lock unique_lock(m_lock);

    m_record_list_map.insert(
        make_pair(type_id, make_shared<record_list_t>(c_record_list_range_size)));
}

void record_list_manager_t::reset_record_list(gaia_type_t type_id)
{
    shared_ptr<record_list_t> list = get_record_list_internal(type_id);

    // If the list was never created, then we shouldn't do anything.
    if (!!list)
    {
        list->reset(c_record_list_range_size);
    }
}

size_t record_list_manager_t::size() const
{
    return m_record_list_map.size();
}

} // namespace storage
} // namespace db
} // namespace gaia
