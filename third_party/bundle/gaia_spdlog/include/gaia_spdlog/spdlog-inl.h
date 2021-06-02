// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef GAIA_SPDLOG_HEADER_ONLY
#include <gaia_spdlog/spdlog.h>
#endif

#include <gaia_spdlog/common.h>
#include <gaia_spdlog/pattern_formatter.h>

namespace gaia_spdlog {

GAIA_SPDLOG_INLINE void initialize_logger(std::shared_ptr<logger> logger)
{
    details::registry::instance().initialize_logger(std::move(logger));
}

GAIA_SPDLOG_INLINE std::shared_ptr<logger> get(const std::string &name)
{
    return details::registry::instance().get(name);
}

GAIA_SPDLOG_INLINE void set_formatter(std::unique_ptr<gaia_spdlog::formatter> formatter)
{
    details::registry::instance().set_formatter(std::move(formatter));
}

GAIA_SPDLOG_INLINE void set_pattern(std::string pattern, pattern_time_type time_type)
{
    set_formatter(std::unique_ptr<gaia_spdlog::formatter>(new pattern_formatter(std::move(pattern), time_type)));
}

GAIA_SPDLOG_INLINE void enable_backtrace(size_t n_messages)
{
    details::registry::instance().enable_backtrace(n_messages);
}

GAIA_SPDLOG_INLINE void disable_backtrace()
{
    details::registry::instance().disable_backtrace();
}

GAIA_SPDLOG_INLINE void dump_backtrace()
{
    default_logger_raw()->dump_backtrace();
}

GAIA_SPDLOG_INLINE level::level_enum get_level()
{
    return default_logger_raw()->level();
}

GAIA_SPDLOG_INLINE bool should_log(level::level_enum log_level)
{
    return default_logger_raw()->should_log(log_level);
}

GAIA_SPDLOG_INLINE void set_level(level::level_enum log_level)
{
    details::registry::instance().set_level(log_level);
}

GAIA_SPDLOG_INLINE void flush_on(level::level_enum log_level)
{
    details::registry::instance().flush_on(log_level);
}

GAIA_SPDLOG_INLINE void flush_every(std::chrono::seconds interval)
{
    details::registry::instance().flush_every(interval);
}

GAIA_SPDLOG_INLINE void set_error_handler(void (*handler)(const std::string &msg))
{
    details::registry::instance().set_error_handler(handler);
}

GAIA_SPDLOG_INLINE void register_logger(std::shared_ptr<logger> logger)
{
    details::registry::instance().register_logger(std::move(logger));
}

GAIA_SPDLOG_INLINE void apply_all(const std::function<void(std::shared_ptr<logger>)> &fun)
{
    details::registry::instance().apply_all(fun);
}

GAIA_SPDLOG_INLINE void drop(const std::string &name)
{
    details::registry::instance().drop(name);
}

GAIA_SPDLOG_INLINE void drop_all()
{
    details::registry::instance().drop_all();
}

GAIA_SPDLOG_INLINE void shutdown()
{
    details::registry::instance().shutdown();
}

GAIA_SPDLOG_INLINE void set_automatic_registration(bool automatic_registration)
{
    details::registry::instance().set_automatic_registration(automatic_registration);
}

GAIA_SPDLOG_INLINE std::shared_ptr<gaia_spdlog::logger> default_logger()
{
    return details::registry::instance().default_logger();
}

GAIA_SPDLOG_INLINE gaia_spdlog::logger *default_logger_raw()
{
    return details::registry::instance().get_default_raw();
}

GAIA_SPDLOG_INLINE void set_default_logger(std::shared_ptr<gaia_spdlog::logger> default_logger)
{
    details::registry::instance().set_default_logger(std::move(default_logger));
}

} // namespace gaia_spdlog
