/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "serial_group_manager.hpp"

using namespace gaia::common;
using namespace std;
using namespace gaia::rules;

std::shared_ptr<rule_thread_pool_t::serial_group_t>
serial_group_manager_t::acquire_group(const char* group_name)
{
    shared_ptr<rule_thread_pool_t::serial_group_t> serial_group;

    // If a serial group exists, then hand out a new shared
    // pointer to it by promoting the weak_ptr to a shared_ptr.
    // Otherwise, create a new reference.
    {
        shared_lock lock{m_lock};
        auto value = m_groups.find(group_name);
        if (value != m_groups.end())
        {
            serial_group = value->second.lock();
        }
    }

    // Another thread could have created a new group here
    // so check again under an exclusive lock. If it has
    // still not been created, then create one.
    if (serial_group == nullptr)
    {
        unique_lock lock{m_lock};
        auto value = m_groups.find(group_name);
        if (value != m_groups.end())
        {
            serial_group = value->second.lock();
        }

        // Either we have never created the serial group before or
        // we have created it but there is no outstanding shared pointer because
        // the event_binding it was bound to has been deleted.
        if (serial_group == nullptr)
        {
            serial_group = make_shared<rule_thread_pool_t::serial_group_t>();
            m_groups.insert_or_assign(group_name, serial_group);
        }
    }

    return serial_group;
}
