/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <spdlog/spdlog.h>

#include "gaia/exception.hpp"

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace logging
{

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
     * Fine-grained informational events that are most
     * useful to debug an application.
     */
    template <typename... T_args>
    void debug(const char* format, const T_args&... args)
    {
        m_spdlogger->debug(format, args...);
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
     * Informational messages that highlight the progress
     * of the application at coarse-grained level.
     */
    template <typename... T_args>
    void warn(const char* format, const T_args&... args)
    {
        m_spdlogger->warn(format, args...);
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
     * Very severe error events that will presumably lead the
     * application to abort.
     */
    template <typename... T_args>
    void critical(const char* format, const T_args&... args)
    {
        m_spdlogger->critical(format, args...);
    }

protected:
    std::shared_ptr<spdlog::logger> m_spdlogger;

private:
    /**
     * Initialize a logger with the given name. If the logger already
     * exists it is reused. Otherwise a new logger is created.
     */
    explicit logger_t(const std::string& logger_name);
};

class logger_exception_t : public gaia_exception
{
public:
    explicit logger_exception_t(const std::string& message)
    {
        m_message = message;
    }
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
