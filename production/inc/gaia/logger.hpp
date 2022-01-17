/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/exception.hpp"

#include "gaia_spdlog/spdlog.h"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

namespace gaia
{
/**
 * \addtogroup gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup common
 * @{
 */
namespace logging
{
/**
 * \addtogroup logging
 * @{
 */

/**
 * Gaia logging API. Logging is performed via instances of logger_t.
 * Direct instantiation of logger_t is disabled. You can access predefined instances
 * of logger_t using the methods provided in this namespace (i.e. app()).
 *
 * This namespace is aliased to gaia_log, which means you can call:
 *
 *  gaia_log::app().info("Hello world")
 *  gaia_log::app().info("Formatting '{}' {}", "string", 1.0)
 *  gaia_log::app().warn("Warning message!")
 *
 * Which outputs:
 *
 *  [2020-09-15T11:00:23-04:00] [info] [392311 392311] <app>: Hello world
 *  [2020-09-15T11:00:23-04:00] [info] [392311 392311] <app>: Formatting 'string' 1.0
 *  [2020-09-15T11:00:23-04:00] [warn] [392311 392311] <app>: Warning message!
 *
 * Internally we use spdlog and we support file configuration via spdlog_setup:
 * - https://github.com/gabime/spdlog
 * - https://github.com/guangie88/spdlog_setup
 *
 * The logging configuration file location can be specified in gaia::system::initialize().
 *
 * You can check whether a log level is enabled before running expensive calculations:
 *
 *  if (gaia_log::app().is_debug_enabled())
 *  {
 *      int num = very_slow_function();
 *      gaia_log::app().debug("value: {}", num);
 *  }
 *
 * \addtogroup Logging
 * @{
 */

// Forward declarations of internal classes.
class internal_logger_t;

/**
  * Gaia Logger API.
  */
class logger_t
{
    friend class internal_logger_t;

public:
    /**
     * Finer-grained informational events than debug.
     * The enum is initialized to this value if nothing
     * is specified.
     */
    template <typename... T_args>
    void trace(const char* format, const T_args&... args)
    {
        m_spdlogger->trace(format, args...);
    }

    /**
     * Returns true if the log level is at "trace".
     */
    bool is_trace_enabled()
    {
        return m_spdlogger->should_log(gaia_spdlog::level::level_enum::trace);
    }

    /**
     * Fine-grained informational events that are most
     * useful to debug an application.
     */
    template <typename... T_args>
    void debug(const char* format, const T_args&... args)
    {
        m_spdlogger->debug(format, args...);
    }

    /**
     * Returns true if the log level is at "debug" or higher.
     */
    bool is_debug_enabled()
    {
        return m_spdlogger->should_log(gaia_spdlog::level::level_enum::debug);
    }

    /**
     * Informational messages that highlight the progress
     *  of the application at coarse-grained level.
     */
    template <typename... T_args>
    void info(const char* format, const T_args&... args)
    {
        m_spdlogger->info(format, args...);
    }

    /**
     * Returns true if the log level is at "info" or higher.
     */
    bool is_info_enabled()
    {
        return m_spdlogger->should_log(gaia_spdlog::level::level_enum::info);
    }

    /**
     * Informational messages that highlight the progress
     * of the application at coarse-grained level.
     */
    template <typename... T_args>
    void warn(const char* format, const T_args&... args)
    {
        m_spdlogger->warn(format, args...);
    }

    /**
     * Returns true if the log level is at "warn" or higher.
     */
    bool is_warn_enabled()
    {
        return m_spdlogger->should_log(gaia_spdlog::level::level_enum::warn);
    }

    /**
     * Potentially harmful situations.
     */
    template <typename... T_args>
    void error(const char* format, const T_args&... args)
    {
        m_spdlogger->error(format, args...);
    }

    /**
     * Returns true if the log level is at "error" or higher.
     */
    bool is_error_enabled()
    {
        return m_spdlogger->should_log(gaia_spdlog::level::level_enum::err);
    }

    /**
     * Very severe error events that will presumably lead the
     * application to abort.
     */
    template <typename... T_args>
    void critical(const char* format, const T_args&... args)
    {
        m_spdlogger->critical(format, args...);
    }

    /**
     * Returns true if the log level is at "critical" or higher.
     */
    bool is_critical_enabled()
    {
        return m_spdlogger->should_log(gaia_spdlog::level::level_enum::critical);
    }

protected:
    std::shared_ptr<gaia_spdlog::logger> m_spdlogger;

private:
    /**
     * Initialize a logger with the given name. If the logger already
     * exists it is reused. Otherwise a new logger is created.
     */
    explicit logger_t(const std::string& logger_name);
};

/**
 * Logger to be used in the application layer.
 */
logger_t& app();

/*@}*/
} // namespace logging
/*@}*/
} // namespace common
/*@}*/
} // namespace gaia

namespace gaia_log = gaia::common::logging;

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
