/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "debug_logger.hpp"

#include "logger_spdlog.hpp"

#include "logger_manager.hpp"

namespace gaia::common::logging
{

unique_ptr<debug_logger_t> debug_logger_t::create(const char* logger_name)
{
    // Create and register a synchronous logger with no sinks.  Registering the logger
    // is required so that the constructor of the logger_t class doesn't create the default
    // asynchronous one.
    auto logger = make_shared<spdlog::logger>(logger_name);
    spdlog::register_logger(logger);
    return unique_ptr<debug_logger_t>(new debug_logger_t(logger_name));
}

debug_logger_t::debug_logger_t(const std::string& logger_name)
    : logger_t(logger_name)
{
}

} // namespace gaia::common::logging
