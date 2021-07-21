/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/common/debug_logger.hpp"

#include "gaia_internal/common/logger_manager.hpp"
#include "gaia_internal/common/logger_spdlog.hpp"

using namespace std;

namespace gaia::common::logging
{

debug_logger_t* debug_logger_t::create(const char* logger_name)
{
    // Create and register a synchronous logger with no sinks.  Registering the logger
    // is required so that the constructor of the logger_t class doesn't create the default
    // asynchronous one.
    auto logger = make_shared<gaia_spdlog::logger>(logger_name);
    gaia_spdlog::register_logger(logger);
    return new debug_logger_t(logger_name);
}

debug_logger_t::debug_logger_t(const std::string& logger_name)
    : internal_logger_t(logger_name){};

// These are friend functions of the logger_manager_t class.
// They replace existing logger instances and make shared
// references to them.
void set_sys_logger(logger_t* logger_ptr)
{
    logger_manager_t::get().m_sys_logger.reset(logger_ptr);
}

void set_db_logger(logger_t* logger_ptr)
{
    logger_manager_t::get().m_db_logger.reset(logger_ptr);
}

void set_rules_logger(logger_t* logger_ptr)
{
    logger_manager_t::get().m_rules_logger.reset(logger_ptr);
}

void set_rules_stats_logger(logger_t* logger_ptr)
{
    logger_manager_t::get().m_rules_stats_logger.reset(logger_ptr);
}

void set_catalog_logger(logger_t* logger_ptr)
{
    logger_manager_t::get().m_catalog_logger.reset(logger_ptr);
}

void set_app_logger(logger_t* logger_ptr)
{
    logger_manager_t::get().m_app_logger.reset(logger_ptr);
}

} // namespace gaia::common::logging
