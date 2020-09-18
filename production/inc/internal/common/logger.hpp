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
 * The logging system is automatically initialized on the first API call.
 *
 * The logging is performed via logger_t objects. Each instance of this
 * class represent a separated logger. Ideally, different submodules should use
 * different loggers:
 *
 *  [2020-09-15T11:00:23-04:00] [info] [392311 392311] <gaia-root>: I'm the generic root logger
 *  [2020-09-15T11:00:23-04:00] [info] [392311 392311] <gaia-se>: I'm the Storage Engine logger
 *  [2020-09-15T11:00:23-04:00] [info] [392311 392311] <gaia-rules>: I'm the Rules logger
 *
 * All the loggers should be registered in the logger_registry_t to make them available in
 * any point of the application.
 *
 * The framework always provide a default logger named 'gaia-root'. This is used in the
 * static api:
 *
 *  gaia_log::warn("Message");
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

    // Need to explicitly define the destructor because of the
    // usage of the pimpl idiom (log_impl_t)
    ~logger_t() = default;

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
    explicit uninitialized_logger_t()
        : logger_t("uninitialized") {
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
 * Initializes all loggers from the passed in configuration.
 * 
 * The following well-known loggers are created:
 * // Do we mandate that they be in the toml file?
 * gaia_root
 * // gaia_db
 * // gaia_rules
 */
void initialize(const string& config_path);
void shutdown();

/**
 * Exposed loggers. Filled in by initialize and destroyed on shutdown.
 */
extern logger_t& g_sys;
extern logger_t& g_db;
extern logger_t& g_scheduler;


/*@}*/
} // namespace logging
/*@}*/
} // namespace common
/*@}*/
} // namespace gaia

namespace gaia_log = gaia::common::logging;
