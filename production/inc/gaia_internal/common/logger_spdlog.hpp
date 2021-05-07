/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>
#include <string>

#include <spdlog/spdlog.h>

namespace gaia::common::logging
{

namespace spdlog_defaults
{
/**
 * Example:
 * [2020-12-21T19:25:59.368] [info] [1034644 1034766] <rules>: message 1
 * [2020-12-21T19:25:59.368] [warning] [1034644 1034766] <db>: message 2
 *
 * where
 * - info: level
 * - 1034644: process
 * - 1034766: thread
 * - rules: logger name
 */
constexpr const char* c_default_pattern = "[%Y-%m-%dT%T.%e] [%l] [%P %t] <%n>: %v";

constexpr size_t c_default_queue_size = 8192;
constexpr size_t c_default_thread_count = 1;
constexpr spdlog::level::level_enum c_default_level = spdlog::level::info;

/**
 * Creates an instance of spdlog::logger with some convenient defaults. This function
 * is used to create loggers whenever they are not defined in the log configuration.
 */
std::shared_ptr<spdlog::logger> create_logger_with_default_settings(const std::string& logger_name);
} // namespace spdlog_defaults

} // namespace gaia::common::logging
