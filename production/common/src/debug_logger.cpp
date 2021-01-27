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
    auto logger = make_shared<spdlog::logger>(logger_name);
    spdlog::register_logger(logger);
    return new debug_logger_t(logger_name);
}

debug_logger_t::debug_logger_t(const std::string& logger_name)
    : internal_logger_t(logger_name){};

// Note that this is a friend function of the logger manager class.
// It will replace the existing rules_stats logger and make a shared
// reference to it.
void set_rules_stats(logger_t* logger_ptr)
{
    logger_manager_t::get().m_rules_stats_logger.reset(logger_ptr);
}

} // namespace gaia::common::logging
