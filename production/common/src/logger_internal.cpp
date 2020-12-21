/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "logger_internal.hpp"

#include <iostream>

#include "spdlog/sinks/basic_file_sink.h"

#include "logger_manager.hpp"
#include "logger_spdlog.hpp"

using namespace std;

namespace gaia::common::logging
{

//
// logger_t implementation.
//
logger_t::logger_t(const string& logger_name)
    : m_logger_name(logger_name)
{
    auto logger = spdlog::get(logger_name);

    if (logger)
    {
        m_spdlogger = logger;
    }
    else
    {
        m_spdlogger = spdlog_defaults::create_logger_with_default_settings(logger_name);
    }
}

spdlog::level::level_enum logger_t::to_spdlog_level(gaia_log::log_level_t gaia_level)
{
    switch (gaia_level)
    {
    case log_level_t::trace:
        return spdlog::level::trace;
    case log_level_t::debug:
        return spdlog::level::debug;
    case log_level_t::info:
        return spdlog::level::info;
    case log_level_t::warn:
        return spdlog::level::warn;
    case log_level_t::error:
        return spdlog::level::err;
    case log_level_t::critical:
        return spdlog::level::critical;
    case log_level_t::off:
        return spdlog::level::off;
    default:
        throw logger_exception_t("Unsupported logging level: " + std::to_string(static_cast<std::underlying_type_t<log_level_t>>(gaia_level)));
    }
}

//
// Top level API implementation
//
void initialize(const string& config_path)
{
    logger_manager_t::get().init_logging(config_path);
}

void shutdown()
{
    logger_manager_t::get().stop_logging();
}

logger_t& sys()
{
    return logger_manager_t::get().sys_logger();
}

logger_t& db()
{
    return logger_manager_t::get().db_logger();
}

logger_t& re()
{
    return logger_manager_t::get().re_logger();
}

logger_t& catalog()
{
    return logger_manager_t::get().catalog_logger();
}

//
// Stats loggers.
//
logger_t& re_stats()
{
    return logger_manager_t::get().re_stats_logger();
}

logger_t& rules()
{
    return logger_manager_t::get().rules_logger();
}

} // namespace gaia::common::logging
