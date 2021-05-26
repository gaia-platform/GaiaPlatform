// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef GAIA_SPDLOG_HEADER_ONLY
#include <gaia_spdlog/common.h>
#endif

namespace gaia_spdlog {
namespace level {

static string_view_t level_string_views[] GAIA_SPDLOG_LEVEL_NAMES;

static const char *short_level_names[] GAIA_SPDLOG_SHORT_LEVEL_NAMES;

GAIA_SPDLOG_INLINE const string_view_t &to_string_view(gaia_spdlog::level::level_enum l) GAIA_SPDLOG_NOEXCEPT
{
    return level_string_views[l];
}

GAIA_SPDLOG_INLINE void set_string_view(gaia_spdlog::level::level_enum l, const string_view_t &s) GAIA_SPDLOG_NOEXCEPT
{
    level_string_views[l] = s;
}

GAIA_SPDLOG_INLINE const char *to_short_c_str(gaia_spdlog::level::level_enum l) GAIA_SPDLOG_NOEXCEPT
{
    return short_level_names[l];
}

GAIA_SPDLOG_INLINE gaia_spdlog::level::level_enum from_str(const std::string &name) GAIA_SPDLOG_NOEXCEPT
{
    int level = 0;
    for (const auto &level_str : level_string_views)
    {
        if (level_str == name)
        {
            return static_cast<level::level_enum>(level);
        }
        level++;
    }
    // check also for "warn" and "err" before giving up..
    if (name == "warn")
    {
        return level::warn;
    }
    if (name == "err")
    {
        return level::err;
    }
    return level::off;
}
} // namespace level

GAIA_SPDLOG_INLINE gaia_spdlog_ex::gaia_spdlog_ex(std::string msg)
    : msg_(std::move(msg))
{}

GAIA_SPDLOG_INLINE gaia_spdlog_ex::gaia_spdlog_ex(const std::string &msg, int last_errno)
{
    memory_buf_t outbuf;
    fmt::format_system_error(outbuf, last_errno, msg);
    msg_ = fmt::to_string(outbuf);
}

GAIA_SPDLOG_INLINE const char *gaia_spdlog_ex::what() const GAIA_SPDLOG_NOEXCEPT
{
    return msg_.c_str();
}

GAIA_SPDLOG_INLINE void throw_spdlog_ex(const std::string &msg, int last_errno)
{
    GAIA_SPDLOG_THROW(gaia_spdlog_ex(msg, last_errno));
}

GAIA_SPDLOG_INLINE void throw_spdlog_ex(std::string msg)
{
    GAIA_SPDLOG_THROW(gaia_spdlog_ex(std::move(msg)));
}

} // namespace gaia_spdlog
