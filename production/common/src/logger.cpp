/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gaia_spdlog/sinks/basic_file_sink.h"

#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/common/logger_manager.hpp"
#include "gaia_internal/common/logger_spdlog.hpp"

using namespace std;

namespace gaia::common::logging
{

//
// logger_t implementation.
//
logger_t::logger_t(const string& logger_name)
{
    auto logger = gaia_spdlog::get(logger_name);

    if (logger)
    {
        m_spdlogger = logger;
    }
    else
    {
        m_spdlogger = spdlog_defaults::create_logger_with_default_settings(logger_name);
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

logger_t& rules()
{
    return logger_manager_t::get().rules_logger();
}

logger_t& catalog()
{
    return logger_manager_t::get().catalog_logger();
}

//
// Stats loggers.
//
logger_t& rules_stats()
{
    return logger_manager_t::get().rules_stats_logger();
}

logger_t& app()
{
    return logger_manager_t::get().app_logger();
}

} // namespace gaia::common::logging
