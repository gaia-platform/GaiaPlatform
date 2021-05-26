// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef GAIA_SPDLOG_HEADER_ONLY
#include <gaia_spdlog/sinks/sink.h>
#endif

#include <gaia_spdlog/common.h>

GAIA_SPDLOG_INLINE bool gaia_spdlog::sinks::sink::should_log(gaia_spdlog::level::level_enum msg_level) const
{
    return msg_level >= level_.load(std::memory_order_relaxed);
}

GAIA_SPDLOG_INLINE void gaia_spdlog::sinks::sink::set_level(level::level_enum log_level)
{
    level_.store(log_level, std::memory_order_relaxed);
}

GAIA_SPDLOG_INLINE gaia_spdlog::level::level_enum gaia_spdlog::sinks::sink::level() const
{
    return static_cast<gaia_spdlog::level::level_enum>(level_.load(std::memory_order_relaxed));
}
