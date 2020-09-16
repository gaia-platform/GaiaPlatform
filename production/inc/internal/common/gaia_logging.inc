/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_logging_spdlog.hpp"

namespace gaia::common::logging {

template <typename... Args>
void gaia_logger_t::log(log_level_t level, const char* format, const Args&... args) {
    m_pimpl->get_logger().log(to_spdlog_level(level), format, args...);
}

template <typename... Args>
void gaia_logger_t::trace(const char* format, const Args&... args) {
    m_pimpl->get_logger().trace(format, args...);
}

template <typename... Args>
void gaia_logger_t::debug(const char* format, const Args&... args) {
    m_pimpl->get_logger().debug(format, args...);
}

template <typename... Args>
void gaia_logger_t::info(const char* format, const Args&... args) {
    m_pimpl->get_logger().info(format, args...);
}

template <typename... Args>
void gaia_logger_t::warn(const char* format, const Args&... args) {
    m_pimpl->get_logger().warn(format, args...);
}

template <typename... Args>
void gaia_logger_t::error(const char* format, const Args&... args) {
    m_pimpl->get_logger().error(format, args...);
}

template <typename... Args>
void gaia_logger_t::critical(const char* format, const Args&... args) {
    m_pimpl->get_logger().critical(format, args...);
}

} // namespace gaia::common::logging
