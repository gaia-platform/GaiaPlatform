/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <iostream>
#include <string>
#include <spdlog/spdlog.h>


namespace gaia::common::logging {

namespace spdlog_defaults {
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
    constexpr const char* c_default_pattern = "[%Y-%m-%dT%T%z] [%l] p:%P t:%t <%n>: %v";

    constexpr const size_t c_default_queue_size = 8192;
    constexpr const size_t c_default_thread_count = 2;
    constexpr const spdlog::level::level_enum c_default_level = spdlog::level::info;
}
/**
 * Configures spdlog to a default configuration. This function is called if
 * the configuration from file fails.
 */
void configure_spdlog_default();

} // namespace gaia::common::logging
