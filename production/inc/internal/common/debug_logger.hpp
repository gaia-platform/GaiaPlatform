/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <shared_mutex>
#include <sstream>
#include <string>
#include <unordered_map>

#include "logger.hpp"

namespace gaia
{
namespace common
{
namespace logging
{

/**
 * Represents a synchronous logger useful for testing.  Exposesd the underlying
 * spdlogger so that tests can provide their own sinks or custom pattern.
 */
class debug_logger_t : public logger_t
{
public:
    static unique_ptr<debug_logger_t> create(const char* logger_name);

    shared_ptr<spdlog::logger> get_spdlogger()
    {
        return m_spdlogger;
    }

private:
    debug_logger_t(const std::string& logger_name);
};

} // namespace logging
} // namespace common
} // namespace gaia
