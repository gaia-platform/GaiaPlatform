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
 * Allow overriding internal logger instances explicitly.
 */
void set_sys_logger(logger_t* logger_ptr);
void set_db_logger(logger_t* logger_ptr);
void set_rules_logger(logger_t* logger_ptr);
void set_rules_stats_logger(logger_t* logger_ptr);
void set_catalog_logger(logger_t* logger_ptr);
void set_app_logger(logger_t* logger_ptr);

} // namespace logging
} // namespace common
} // namespace gaia
