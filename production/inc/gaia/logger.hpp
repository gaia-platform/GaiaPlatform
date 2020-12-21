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
 * Contains Gaia logging API. At the moment it is a wrapper around spdlog logger.
 * This namespace is aliased to gaia_log.
 *
 * The logging system MUST be initialized calling `initialize(const string& config_path)`.
 *
 * The logging is performed via logger_t objects. Each instance of this class represent
 * a separated logger. Different submodules should use different loggers.
 * FUnctions for the different submodules are provided: sys(), db(), scheduler() etc..
 *
 * Calling:
 *
 *  gaia_log::sys().info("I'm the Sys logger")
 *  gaia_log::db().info("I'm the Storage Engine logger")
 *  gaia_log::scheduler().info("I'm the Rules logger")
 *
 * Outputs:
 *
 *  [2020-09-15T11:00:23-04:00] [info] [392311 392311] <sys>: I'm the Sys logger
 *  [2020-09-15T11:00:23-04:00] [info] [392311 392311] <db>: I'm the Storage Engine logger
 *  [2020-09-15T11:00:23-04:00] [info] [392311 392311] <scheduler>: I'm the Rules logger
 *
 * Dynamic creation of logger is not supported and generally discouraged. If you want to
 * add a new logger add a constant in this file.
 *
 * \addtogroup Logging
 * @{
 */

enum class log_level_t
{

    /** Finer-grained informational events than debug.
     *  The enum is initialized to this value if nothing
     *  is specified.*/
    trace = 0,

    /** Fine-grained informational events that are most
     *  useful to debug an application. */
    debug = 1,

    /** Informational messages that highlight the progress
     *  of the application at coarse-grained level. */
    info = 2,

    /** Potentially harmful situations. */
    warn = 3,

    /** Error events that might still allow the application to
     *  continue running */
    error = 4,

    /** Very severe error events that will presumably lead the
     *  application to abort */
    critical = 5,

    /** Turn the logging off */
    off = 6
};

// Forward declarations of internal classes.
class logger_manager_t;
class debug_logger_t;

/**
* Gaia Logger API.
*/
class logger_t
{
    friend class logger_manager_t;
    friend class debug_logger_t;

public:
    const std::string& get_name() const
    {
        return m_logger_name;
    }

    template <typename... T_args>
    void log(log_level_t level, const char* format, const T_args&... args)
    {
        m_spdlogger->log(to_spdlog_level(level), format, args...);
    }

    template <typename... T_args>
    void trace(const char* format, const T_args&... args)
    {
        m_spdlogger->trace(format, args...);
    }

    template <typename... T_args>
    void debug(const char* format, const T_args&... args)
    {
        m_spdlogger->debug(format, args...);
    }

    template <typename... T_args>
    void info(const char* format, const T_args&... args)
    {
        m_spdlogger->info(format, args...);
    }

    template <typename... T_args>
    void warn(const char* format, const T_args&... args)
    {
        m_spdlogger->warn(format, args...);
    }

    template <typename... T_args>
    void error(const char* format, const T_args&... args)
    {
        m_spdlogger->error(format, args...);
    }

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

    static spdlog::level::level_enum to_spdlog_level(log_level_t level);

    std::string m_logger_name;
};

class logger_exception_t : public gaia_exception
{
public:
    explicit logger_exception_t(const std::string& message)
    {
        m_message = message;
    }
};

/** Logger to be use at application layer */
logger_t& rules();

/*@}*/
} // namespace logging
/*@}*/
} // namespace common
/*@}*/
} // namespace gaia

namespace gaia_log = gaia::common::logging;
