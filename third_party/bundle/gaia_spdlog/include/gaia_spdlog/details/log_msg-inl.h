// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef GAIA_SPDLOG_HEADER_ONLY
#include <gaia_spdlog/details/log_msg.h>
#endif

#include <gaia_spdlog/details/os.h>

namespace gaia_spdlog {
namespace details {

GAIA_SPDLOG_INLINE log_msg::log_msg(gaia_spdlog::log_clock::time_point log_time, gaia_spdlog::source_loc loc, string_view_t a_logger_name,
    gaia_spdlog::level::level_enum lvl, gaia_spdlog::string_view_t msg)
    : logger_name(a_logger_name)
    , level(lvl)
    , time(log_time)
#ifndef GAIA_SPDLOG_NO_THREAD_ID
    , thread_id(os::thread_id())
#endif
    , source(loc)
    , payload(msg)
{}

GAIA_SPDLOG_INLINE log_msg::log_msg(
    gaia_spdlog::source_loc loc, string_view_t a_logger_name, gaia_spdlog::level::level_enum lvl, gaia_spdlog::string_view_t msg)
    : log_msg(os::now(), loc, a_logger_name, lvl, msg)
{}

GAIA_SPDLOG_INLINE log_msg::log_msg(string_view_t a_logger_name, gaia_spdlog::level::level_enum lvl, gaia_spdlog::string_view_t msg)
    : log_msg(os::now(), source_loc{}, a_logger_name, lvl, msg)
{}

} // namespace details
} // namespace gaia_spdlog
