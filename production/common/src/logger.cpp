/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "logger.hpp"

#include <filesystem>
#include <iostream>

#include "spdlog/async.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/syslog_sink.h"
#include "spdlog_setup/conf.h"
#include "logger_spdlog.hpp"
#include "logger_factory.hpp"

namespace fs = std::filesystem;

namespace gaia::common::logging {

//
// logger_t implementation.
//
logger_t::logger_t(const string& logger_name) : m_logger_name(logger_name) {
    auto logger = spdlog::get(logger_name);
    if (logger) {
        m_spdlogger = logger;
        return;
    } 

    // Allow uninitialized instances to be created
    // for bootstrapping.
    if (0 == logger_name.compare(logger_factory_t::c_uninitialized_logger)) {
        return;
    }

    string message = "logger '" + logger_name + "' not found.";
    throw logger_exception_t(message);
}

spdlog::level::level_enum logger_t::to_spdlog_level(gaia_log::log_level_t gaia_level) {
    switch (gaia_level) {
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
uninitialized_logger_t uninitialized_logger;
logger_t& g_sys = uninitialized_logger;

//
// Topl level API implemetnation
//
void initialize(const string& config_path)
{
   logger_factory_t::get().init_logging(config_path);
}

} // namespace gaia::common::logging
