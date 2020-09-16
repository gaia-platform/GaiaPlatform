/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <iostream>
#include <string>
#include <spdlog/spdlog.h>

#include "gaia_logging.hpp"

namespace gaia::common::logging {

/**
 * Example:
 * [2020-09-11T16:21:54-04:00] [debug   ] p:201405 t:201405 <gaia-root>: message 1
 * [2020-09-11T16:21:54-04:00] [critical] p:201405 t:201405 <gaia-root>: message 2
 *
 * where
 * - critical: level
 * - p: process
 * - t: thread
 * - gaia-root: logger name
 */
constexpr const char* c_default_spdlog_pattern = "[%Y-%m-%dT%T%z] [%l] p:%P t:%t <%n>: %v";

constexpr const size_t c_default_spdlog_queue_size = 8192;
constexpr const size_t c_default_spdlog_thread_count = 2;
constexpr const spdlog::level::level_enum c_default_spdlog_level = spdlog::level::info;

/**
 * Configures spdlog to a default configuration. This function is called if
 * the configuration from file fails.
 */
void configure_spdlog_default();

/**
 * Creates a clone of the root_logger with the given name.
 */
shared_ptr<spdlog::logger> create_spdlog_logger(const std::string& logger_name);

spdlog::level::level_enum to_spdlog_level(gaia_log::log_level_t gaia_level);

class log_impl_t {
  public:
    explicit log_impl_t(const std::string& logger_name) {
        auto logger = spdlog::get(logger_name);

        if (logger) {
            m_logger = logger;
        } else {
            m_logger = create_spdlog_logger(logger_name);
        }
    }

    spdlog::logger& get_logger() {
        return *m_logger;
    }

  private:
    shared_ptr<spdlog::logger> m_logger;
};

} // namespace gaia::common::logging
