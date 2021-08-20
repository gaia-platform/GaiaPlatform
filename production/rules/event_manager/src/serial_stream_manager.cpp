/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "serial_stream_manager.hpp"

using namespace gaia::common;
using namespace std;
using namespace gaia::rules;

std::shared_ptr<rule_thread_pool_t::serial_stream_t>
serial_stream_manager_t::acquire_stream(const char* stream_name)
{
    shared_ptr<rule_thread_pool_t::serial_stream_t> serial_stream;

    // If a serial stream exists, then hand out a new shared
    // pointer to it by promoting the weak_ptr to a shared_ptr.
    // Otherwise, create a new reference.
    {
        shared_lock lock{m_lock};
        auto value = m_streams.find(stream_name);
        if (value != m_streams.end())
        {
            serial_stream = value->second.lock();
        }
    }

    // Another thread could have created a new stream here
    // so check again under an exclusive lock. If it has
    // still not been created, then create one.
    if (serial_stream == nullptr)
    {
        unique_lock lock{m_lock};
        auto value = m_streams.find(stream_name);
        if (value != m_streams.end())
        {
            serial_stream = value->second.lock();
        }
        else
        {
            serial_stream = make_shared<rule_thread_pool_t::serial_stream_t>();
            m_streams.insert({stream_name, serial_stream});
        }
    }

    return serial_stream;
}
