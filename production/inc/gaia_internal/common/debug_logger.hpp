/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <shared_mutex>
#include <sstream>
#include <string>
#include <unordered_map>

#include "gaia_internal/common/logger_internal.hpp"

namespace gaia
{
namespace common
{
namespace logging
{

/**
 * Represents a synchronous logger useful for testing. Exposes the underlying
 * spdlogger so that tests can provide their own sinks or custom pattern.
 */
class debug_logger_t : public internal_logger_t
{
public:
    static debug_logger_t* create(const char* logger_name);

    std::shared_ptr<gaia_spdlog::logger> get_spdlogger()
    {
        return m_spdlogger;
    }

private:
    explicit debug_logger_t(const std::string& logger_name);
};

/**
 * Allow overriding the rule_stats logger explicitly.
 */
void set_rules_stats(logger_t* logger);

} // namespace logging
} // namespace common
} // namespace gaia
