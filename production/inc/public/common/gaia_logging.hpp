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
 * You must call the init_logging(const string& config_path) method before using
 * the API. This method should be called at the Gaia startup (TODO where it is?).
 *
 * The logging is performed via gaia_logger_t objects. Each instance of this
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

/** Name of the root logger. This is the logger used if no specific logger is specified. */
constexpr const char* c_gaia_root_logger = "gaia-root";

/** Default logging path used if none is specified via configuration. */
constexpr const char* c_default_log_path = "logs/gaia.log";

/** Default location of the log configuration file */
// TODO it is unclear to me how we can provide this
constexpr const char* c_default_log_conf_path = "log_conf.toml";

enum class log_level_t {
    trace = 1,
    debug = 2,
    info = 3,
    warn = 4,
    err = 5,
    critical = 6,
    off = 0,
};

/**
* Use the PIMPL idiom to hide the logging implementation details.
*/
class log_impl_t;

/**
* Inits the logging system using the configuration file provided.
* If the config file does not exists or it is invalid the system
* fallback on a default configuration.
*
* @throws logging_exception_t if the logging is already initialized
*/
void init_logging(const string& config_path);

/**
* Returns true if the logging is initialized
*/
bool is_logging_initialized();

/**
* Stops the logging system, cleaning up all the loggers.
 * Returns true if the operation succeed, false otherwise.
*/
bool stop_logging();

/**
* Gaia Logger API.
*/
class gaia_logger_t {
  public:
    /**
     * Initialize a logger with the given name. If the logger already
     * exists in the underlying framework it reuses it. Otherwsie a
     * new logger is created cloning `gaia-root' logger.
     */
    explicit gaia_logger_t(const std::string& logger_name);

    // Need to explicitly define the destructor because of the
    // usage of the pimpl idiom (log_impl_t)
    ~gaia_logger_t();

    const std::string& get_name();

    template <typename... Args>
    void log(log_level_t level, const char* format, const Args&... args);
    void log(log_level_t level, const char* msg);

    template <typename... Args>
    void trace(const char* format, const Args&... args);
    void trace(const char* msg);

    template <typename... Args>
    void debug(const char* format, const Args&... args);
    void debug(const char* msg);

    template <typename... Args>
    void info(const char* format, const Args&... args);
    void info(const char* msg);

    template <typename... Args>
    void warn(const char* format, const Args&... args);
    void warn(const char* msg);

    template <typename... Args>
    void error(const char* format, const Args&... args);
    void error(const char* msg);

    template <typename... Args>
    void critical(const char* format, const Args&... args);
    void critical(const char* msg);

  private:
    std::string m_logger_name;
    std::unique_ptr<log_impl_t> m_pimpl;
};

class logger_exception_t : public gaia_exception {
  public:
    explicit logger_exception_t(const std::string& message) {
        m_message = message;
    }
};

/**
 * Registers a new logger.
 *
 * @throws logger_exception_t if a logger with the same name
 * already exists.
 */
void register_logger(const std::shared_ptr<gaia_logger_t>& logger);

/**
    * Returns the logger with the given name or nullptr.
    */
std::shared_ptr<gaia_logger_t> get(const std::string& logger_name);

/**
 * Unregister the logger with the given name. If the logger
 * does not exists this function is a no-op.
 */
void unregister_logger(const std::string& logger_name);

/**
 * Returns the c_gaia_root_logger. This logger should always be available.
 */
std::shared_ptr<gaia_logger_t> default_logger();

template <typename... Args>
inline void log(log_level_t level, const char* format, const Args&... args) {
    default_logger()->log(level, format, args...);
}

inline void log(log_level_t level, const char* msg) {
    default_logger()->log(level, msg);
}

template <typename... Args>
inline void trace(const char* format, const Args&... args) {
    default_logger()->trace(format, args...);
}

inline void trace(const char* msg) {
    default_logger()->trace(msg);
}

template <typename... Args>
inline void debug(const char* format, const Args&... args) {
    default_logger()->debug(format, args...);
}

inline void debug(const char* msg) {
    default_logger()->debug(msg);
}

template <typename... Args>
inline void info(const char* format, const Args&... args) {
    default_logger()->info(format, args...);
}

inline void info(const char* msg) {
    default_logger()->info(msg);
}

template <typename... Args>
inline void warn(const char* format, const Args&... args) {
    default_logger()->warn(format, args...);
}

inline void warn(const char* msg) {
    default_logger()->warn(msg);
}

template <typename... Args>
inline void error(const char* format, const Args&... args) {
    default_logger()->error(format, args...);
}

inline void error(const char* msg) {
    default_logger()->error(msg);
}

template <typename... Args>
inline void critical(const char* format, const Args&... args) {
    default_logger()->critical(format, args...);
}

inline void critical(const char* msg) {
    default_logger()->critical(msg);
}

/*@}*/
} // namespace logging
/*@}*/
} // namespace common
/*@}*/
} // namespace gaia

namespace gaia_log = gaia::common::logging;

#include "gaia_logging.inc"
