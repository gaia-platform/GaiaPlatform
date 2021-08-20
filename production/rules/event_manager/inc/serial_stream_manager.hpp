/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <map>
#include <shared_mutex>
#include <thread>

#include "rule_thread_pool.hpp"

namespace gaia
{
namespace rules
{

// This class coordinates invocation of rules inside rulesets
// that have a serial_stream attribute.  All rules that have the
// same serial stream are run sequentially.
class serial_stream_manager_t
{
public:
    serial_stream_manager_t() = default;
    std::shared_ptr<rule_thread_pool_t::serial_stream_t> acquire_stream(const char* stream_name);

private:
    std::shared_mutex m_lock;
    std::map<std::string, std::weak_ptr<rule_thread_pool_t::serial_stream_t>> m_streams;
};

} // namespace rules
} // namespace gaia
