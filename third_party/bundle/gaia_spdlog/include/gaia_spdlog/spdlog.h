// Copyright(c) 2015-present, Gabi Melman & gaia_spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

// gaia_spdlog main header file.
// see example.cpp for usage example

#ifndef GAIA_SPDLOG_H
#define GAIA_SPDLOG_H

#pragma once

#include <gaia_spdlog/common.h>
#include <gaia_spdlog/details/registry.h>
#include <gaia_spdlog/logger.h>
#include <gaia_spdlog/version.h>
#include <gaia_spdlog/details/synchronous_factory.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace gaia_spdlog {

using default_factory = synchronous_factory;

// Create and register a logger with a templated sink type
// The logger's level, formatter and flush level will be set according the
// global settings.
//
// Example:
//   gaia_spdlog::create<daily_file_sink_st>("logger_name", "dailylog_filename", 11, 59);
template<typename Sink, typename... SinkArgs>
inline std::shared_ptr<gaia_spdlog::logger> create(std::string logger_name, SinkArgs &&...sink_args)
{
    return default_factory::create<Sink>(std::move(logger_name), std::forward<SinkArgs>(sink_args)...);
}

// Initialize and register a logger,
// formatter and flush level will be set according the global settings.
//
// Useful for initializing manually created loggers with the global settings.
//
// Example:
//   auto mylogger = std::make_shared<gaia_spdlog::logger>("mylogger", ...);
//   gaia_spdlog::initialize_logger(mylogger);
GAIA_SPDLOG_API void initialize_logger(std::shared_ptr<logger> logger);

// Return an existing logger or nullptr if a logger with such name doesn't
// exist.
// example: gaia_spdlog::get("my_logger")->info("hello {}", "world");
GAIA_SPDLOG_API std::shared_ptr<logger> get(const std::string &name);

// Set global formatter. Each sink in each logger will get a clone of this object
GAIA_SPDLOG_API void set_formatter(std::unique_ptr<gaia_spdlog::formatter> formatter);

// Set global format string.
// example: gaia_spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e %l : %v");
GAIA_SPDLOG_API void set_pattern(std::string pattern, pattern_time_type time_type = pattern_time_type::local);

// enable global backtrace support
GAIA_SPDLOG_API void enable_backtrace(size_t n_messages);

// disable global backtrace support
GAIA_SPDLOG_API void disable_backtrace();

// call dump backtrace on default logger
GAIA_SPDLOG_API void dump_backtrace();

// Get global logging level
GAIA_SPDLOG_API level::level_enum get_level();

// Set global logging level
GAIA_SPDLOG_API void set_level(level::level_enum log_level);

// Determine whether the default logger should log messages with a certain level
GAIA_SPDLOG_API bool should_log(level::level_enum lvl);

// Set global flush level
GAIA_SPDLOG_API void flush_on(level::level_enum log_level);

// Start/Restart a periodic flusher thread
// Warning: Use only if all your loggers are thread safe!
GAIA_SPDLOG_API void flush_every(std::chrono::seconds interval);

// Set global error handler
GAIA_SPDLOG_API void set_error_handler(void (*handler)(const std::string &msg));

// Register the given logger with the given name
GAIA_SPDLOG_API void register_logger(std::shared_ptr<logger> logger);

// Apply a user defined function on all registered loggers
// Example:
// gaia_spdlog::apply_all([&](std::shared_ptr<gaia_spdlog::logger> l) {l->flush();});
GAIA_SPDLOG_API void apply_all(const std::function<void(std::shared_ptr<logger>)> &fun);

// Drop the reference to the given logger
GAIA_SPDLOG_API void drop(const std::string &name);

// Drop all references from the registry
GAIA_SPDLOG_API void drop_all();

// stop any running threads started by gaia_spdlog and clean registry loggers
GAIA_SPDLOG_API void shutdown();

// Automatic registration of loggers when using gaia_spdlog::create() or gaia_spdlog::create_async
GAIA_SPDLOG_API void set_automatic_registration(bool automatic_registration);

// API for using default logger (stdout_color_mt),
// e.g: gaia_spdlog::info("Message {}", 1);
//
// The default logger object can be accessed using the gaia_spdlog::default_logger():
// For example, to add another sink to it:
// gaia_spdlog::default_logger()->sinks().push_back(some_sink);
//
// The default logger can replaced using gaia_spdlog::set_default_logger(new_logger).
// For example, to replace it with a file logger.
//
// IMPORTANT:
// The default API is thread safe (for _mt loggers), but:
// set_default_logger() *should not* be used concurrently with the default API.
// e.g do not call set_default_logger() from one thread while calling gaia_spdlog::info() from another.

GAIA_SPDLOG_API std::shared_ptr<gaia_spdlog::logger> default_logger();

GAIA_SPDLOG_API gaia_spdlog::logger *default_logger_raw();

GAIA_SPDLOG_API void set_default_logger(std::shared_ptr<gaia_spdlog::logger> default_logger);

template<typename FormatString, typename... Args>
inline void log(source_loc source, level::level_enum lvl, const FormatString &gaia_fmt, Args&&...args)
{
    default_logger_raw()->log(source, lvl, gaia_fmt, std::forward<Args>(args)...);
}

template<typename FormatString, typename... Args>
inline void log(level::level_enum lvl, const FormatString &gaia_fmt, Args&&...args)
{
    default_logger_raw()->log(source_loc{}, lvl, gaia_fmt, std::forward<Args>(args)...);
}

template<typename FormatString, typename... Args>
inline void trace(const FormatString &gaia_fmt, Args&&...args)
{
    default_logger_raw()->trace(gaia_fmt, std::forward<Args>(args)...);
}

template<typename FormatString, typename... Args>
inline void debug(const FormatString &gaia_fmt, Args&&...args)
{
    default_logger_raw()->debug(gaia_fmt, std::forward<Args>(args)...);
}

template<typename FormatString, typename... Args>
inline void info(const FormatString &gaia_fmt, Args&&...args)
{
    default_logger_raw()->info(gaia_fmt, std::forward<Args>(args)...);
}

template<typename FormatString, typename... Args>
inline void warn(const FormatString &gaia_fmt, Args&&...args)
{
    default_logger_raw()->warn(gaia_fmt, std::forward<Args>(args)...);
}

template<typename FormatString, typename... Args>
inline void error(const FormatString &gaia_fmt, Args&&...args)
{
    default_logger_raw()->error(gaia_fmt, std::forward<Args>(args)...);
}

template<typename FormatString, typename... Args>
inline void critical(const FormatString &gaia_fmt, Args&&...args)
{
    default_logger_raw()->critical(gaia_fmt, std::forward<Args>(args)...);
}

template<typename T>
inline void log(source_loc source, level::level_enum lvl, const T &msg)
{
    default_logger_raw()->log(source, lvl, msg);
}

template<typename T>
inline void log(level::level_enum lvl, const T &msg)
{
    default_logger_raw()->log(lvl, msg);
}

template<typename T>
inline void trace(const T &msg)
{
    default_logger_raw()->trace(msg);
}

template<typename T>
inline void debug(const T &msg)
{
    default_logger_raw()->debug(msg);
}

template<typename T>
inline void info(const T &msg)
{
    default_logger_raw()->info(msg);
}

template<typename T>
inline void warn(const T &msg)
{
    default_logger_raw()->warn(msg);
}

template<typename T>
inline void error(const T &msg)
{
    default_logger_raw()->error(msg);
}

template<typename T>
inline void critical(const T &msg)
{
    default_logger_raw()->critical(msg);
}

} // namespace gaia_spdlog

//
// enable/disable log calls at compile time according to global level.
//
// define GAIA_SPDLOG_ACTIVE_LEVEL to one of those (before including gaia_spdlog.h):
// GAIA_SPDLOG_LEVEL_TRACE,
// GAIA_SPDLOG_LEVEL_DEBUG,
// GAIA_SPDLOG_LEVEL_INFO,
// GAIA_SPDLOG_LEVEL_WARN,
// GAIA_SPDLOG_LEVEL_ERROR,
// GAIA_SPDLOG_LEVEL_CRITICAL,
// GAIA_SPDLOG_LEVEL_OFF
//

#define GAIA_SPDLOG_LOGGER_CALL(logger, level, ...) (logger)->log(gaia_spdlog::source_loc{__FILE__, __LINE__, GAIA_SPDLOG_FUNCTION}, level, __VA_ARGS__)

#if GAIA_SPDLOG_ACTIVE_LEVEL <= GAIA_SPDLOG_LEVEL_TRACE
#define GAIA_SPDLOG_LOGGER_TRACE(logger, ...) GAIA_SPDLOG_LOGGER_CALL(logger, gaia_spdlog::level::trace, __VA_ARGS__)
#define GAIA_SPDLOG_TRACE(...) GAIA_SPDLOG_LOGGER_TRACE(gaia_spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define GAIA_SPDLOG_LOGGER_TRACE(logger, ...) (void)0
#define GAIA_SPDLOG_TRACE(...) (void)0
#endif

#if GAIA_SPDLOG_ACTIVE_LEVEL <= GAIA_SPDLOG_LEVEL_DEBUG
#define GAIA_SPDLOG_LOGGER_DEBUG(logger, ...) GAIA_SPDLOG_LOGGER_CALL(logger, gaia_spdlog::level::debug, __VA_ARGS__)
#define GAIA_SPDLOG_DEBUG(...) GAIA_SPDLOG_LOGGER_DEBUG(gaia_spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define GAIA_SPDLOG_LOGGER_DEBUG(logger, ...) (void)0
#define GAIA_SPDLOG_DEBUG(...) (void)0
#endif

#if GAIA_SPDLOG_ACTIVE_LEVEL <= GAIA_SPDLOG_LEVEL_INFO
#define GAIA_SPDLOG_LOGGER_INFO(logger, ...) GAIA_SPDLOG_LOGGER_CALL(logger, gaia_spdlog::level::info, __VA_ARGS__)
#define GAIA_SPDLOG_INFO(...) GAIA_SPDLOG_LOGGER_INFO(gaia_spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define GAIA_SPDLOG_LOGGER_INFO(logger, ...) (void)0
#define GAIA_SPDLOG_INFO(...) (void)0
#endif

#if GAIA_SPDLOG_ACTIVE_LEVEL <= GAIA_SPDLOG_LEVEL_WARN
#define GAIA_SPDLOG_LOGGER_WARN(logger, ...) GAIA_SPDLOG_LOGGER_CALL(logger, gaia_spdlog::level::warn, __VA_ARGS__)
#define GAIA_SPDLOG_WARN(...) GAIA_SPDLOG_LOGGER_WARN(gaia_spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define GAIA_SPDLOG_LOGGER_WARN(logger, ...) (void)0
#define GAIA_SPDLOG_WARN(...) (void)0
#endif

#if GAIA_SPDLOG_ACTIVE_LEVEL <= GAIA_SPDLOG_LEVEL_ERROR
#define GAIA_SPDLOG_LOGGER_ERROR(logger, ...) GAIA_SPDLOG_LOGGER_CALL(logger, gaia_spdlog::level::err, __VA_ARGS__)
#define GAIA_SPDLOG_ERROR(...) GAIA_SPDLOG_LOGGER_ERROR(gaia_spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define GAIA_SPDLOG_LOGGER_ERROR(logger, ...) (void)0
#define GAIA_SPDLOG_ERROR(...) (void)0
#endif

#if GAIA_SPDLOG_ACTIVE_LEVEL <= GAIA_SPDLOG_LEVEL_CRITICAL
#define GAIA_SPDLOG_LOGGER_CRITICAL(logger, ...) GAIA_SPDLOG_LOGGER_CALL(logger, gaia_spdlog::level::critical, __VA_ARGS__)
#define GAIA_SPDLOG_CRITICAL(...) GAIA_SPDLOG_LOGGER_CRITICAL(gaia_spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define GAIA_SPDLOG_LOGGER_CRITICAL(logger, ...) (void)0
#define GAIA_SPDLOG_CRITICAL(...) (void)0
#endif

#ifdef GAIA_SPDLOG_HEADER_ONLY
#include "spdlog-inl.h"
#endif

#endif // GAIA_SPDLOG_H
