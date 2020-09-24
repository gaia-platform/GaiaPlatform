/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <sstream>
#include <unordered_map>
#include <shared_mutex>

#include "gaia_exception.hpp"
#include "logger_spdlog.hpp"

namespace gaia {
/**
 * \addtogroup Gaia
 * @{
 */
namespace common {
/**
 * \addtogroup Gaia
 * @{
 */
namespace logging {
/**
 * Contains Gaia logging API. At the moment it is a wrapper around spdlog logger.
 * This namespace is aliased to gaia_log.
 *
 * The logging system MUST be initialized calling `initialize(const string& config_path)`.
 *
 * The logging is performed via logger_t objects. Each instance of this class represent
 * a separated logger. Different submodules should use different loggers.
 * Constants for the different submodules are provided: g_sys, g_db, g_scheduler etc..
 *
 * Calling:
 *
 *  gaia_log::g_sys.info("I'm the Sys logger")
 *  gaia_log::g_db.info("I'm the Storage Engine logger")
 *  gaia_log::g_scheduler.info("I'm the Rules logger")
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

enum class log_level_t {

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

class logger_t;
typedef std::shared_ptr<logger_t> logger_ptr_t;

/**
* Gaia Logger API.
*/
class logger_t {
public:
    /**
     * Initialize a logger with the given name. If the logger already
     * exists in the underlying framework it reuses it. Otherwsie a
     * new logger is created cloning `gaia-root' logger.
     */
    explicit logger_t(const std::string& logger_name);

    const std::string& get_name() const {
        return m_logger_name;
    }

    template <typename... T_args>
    void log(log_level_t level, const char* format, const T_args&... args) {
        m_spdlogger->log(to_spdlog_level(level), format, args...);
    }

    template <typename... T_args>
    void trace(const char* format, const T_args&... args) {
        m_spdlogger->trace(format, args...);
    }

    template <typename... T_args>
    void debug(const char* format, const T_args&... args) {
        m_spdlogger->debug(format, args...);
    }

    template <typename... T_args>
    void info(const char* format, const T_args&... args) {
        m_spdlogger->info(format, args...);
    }

    template <typename... T_args>
    void warn(const char* format, const T_args&... args) {
        m_spdlogger->warn(format, args...);
    }

    template <typename... T_args>
    void error(const char* format, const T_args&... args) {
        m_spdlogger->error(format, args...);
    }

    template <typename... T_args>
    void critical(const char* format, const T_args&... args) {
        m_spdlogger->critical(format, args...);
    }

private:
    static spdlog::level::level_enum to_spdlog_level(log_level_t level);

    std::string m_logger_name;
    shared_ptr<spdlog::logger> m_spdlogger;
};

class logger_exception_t : public gaia_exception {
public:
    explicit logger_exception_t(const std::string& message) {
        m_message = message;
    }
};

/**
 * Throws an exception if any of the loggers are used
 *  before the logging system is initialize
 */
class uninitialized_logger_t : public logger_t {
public:
    constexpr static const char* c_uninitialized_logger = "uninitialized";

    explicit uninitialized_logger_t()
        : logger_t(c_uninitialized_logger) {
    }

    template <typename... T_args>
    void log(log_level_t, const char*, const T_args&...) {
        fail();
    }

    template <typename... T_args>
    void trace(const char*, const T_args&...) {
        fail();
    }

    template <typename... T_args>
    void debug(const char*, const T_args&...) {
        fail();
    }

    template <typename... T_args>
    void info(const char*, const T_args&...) {
        fail();
    }

    template <typename... T_args>
    void warn(const char*, const T_args&...) {
        fail();
    }

    template <typename... T_args>
    void error(const char*, const T_args&...) {
        fail();
    }

    template <typename... T_args>
    void critical(const char*, const T_args&...) {
        fail();
    }

private:
    void fail() const {
        throw logger_exception_t("Logger sub-system not initialized!");
    }
};

/**
 * 
 * Initializes all loggers from the passed in configuration. If the configuration
 * does not contain one of the required loggers (g_sys etc..) a default logger is
 * instantiated.
 */
void initialize(const string& config_path);

/**
 * Release the resources used by the logging framework.
 */
void shutdown();

/**
 * Exposed loggers. Filled in by initialize and destroyed on shutdown.
 */
extern logger_t& g_sys;
extern logger_t& g_db;
extern logger_t& g_scheduler;
extern logger_t& g_catalog;

/*@}*/
} // namespace logging
/*@}*/
} // namespace common
/*@}*/
} // namespace gaia

namespace gaia_log = gaia::common::logging;
