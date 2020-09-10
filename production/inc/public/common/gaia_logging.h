/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <unordered_map>
#include <shared_mutex>
#include "gaia_exception.hpp"

#define GAIA_LOG_LEVEL_TRACE 0
#define GAIA_LOG_LEVEL_DEBUG 1
#define GAIA_LOG_LEVEL_INFO 2
#define GAIA_LOG_LEVEL_WARN 3
#define GAIA_LOG_LEVEL_ERROR 4
#define GAIA_LOG_LEVEL_CRITICAL 5
#define GAIA_LOG_LEVEL_OFF 6

using std::string;

namespace gaia::common::logging {

/** Name of the root logger. This is the logger used if no specific logger is specified. */
constexpr const char* c_gaia_root_logger = "gaia-root";

/** Default logging path used if none is specified via configuration. */
constexpr const char* c_default_log_path = "logs/gaia.log";

enum class log_level_t {
    trace = GAIA_LOG_LEVEL_TRACE,
    debug = GAIA_LOG_LEVEL_DEBUG,
    info = GAIA_LOG_LEVEL_INFO,
    warn = GAIA_LOG_LEVEL_WARN,
    err = GAIA_LOG_LEVEL_ERROR,
    critical = GAIA_LOG_LEVEL_CRITICAL,
    off = GAIA_LOG_LEVEL_OFF,
};

/**
 * Use the PIMPL idiom to hide the logging implementation details.
 */
class log_impl_t;

/**
 * Inits the logging system using the configuration file provided.
 * If the config file does not exists or it is invalid the system
 * fallback on a default configuration.
 */
void init_logging(const string& config_path);

class gaia_logger_t {
  public:
    explicit gaia_logger_t(const string& logger_name);

    // Need to explicitly define the destructor because of the
    // usage of the pimpl idiom (log_impl_t)
    ~gaia_logger_t();

    const std::string& get_name();

    template <typename... Args>
    void log(log_level_t level, std::string_view format, Args&... args);
    void log(log_level_t level, std::string_view msg);

    template <typename... Args>
    void trace(std::string_view format, Args&... args);
    void trace(std::string_view msg);

    template <typename... Args>
    void debug(std::string_view format, Args&... args);
    void debug(std::string_view msg);

    template <typename... Args>
    void info(std::string_view format, Args&... args);
    void info(std::string_view msg);

    template <typename... Args>
    void warn(std::string_view format, Args&... args);
    void warn(std::string_view msg);

    template <typename... Args>
    void error(std::string_view format, Args&... args);
    void error(std::string_view msg);

    template <typename... Args>
    void critical(std::string_view format, Args&... args);
    void critical(std::string_view msg);

  private:
    std::string m_logger_name;
    std::unique_ptr<log_impl_t> p_impl;
};

class logger_registry_t {
  public:
    logger_registry_t(const logger_registry_t&) = delete;
    logger_registry_t& operator=(const logger_registry_t&) = delete;
    logger_registry_t(logger_registry_t&&) = delete;
    logger_registry_t& operator=(logger_registry_t&&) = delete;

    static logger_registry_t& instance() {
        static logger_registry_t type_registry;
        return type_registry;
    }

    logger_registry_t() = default;

    void register_logger(std::shared_ptr<gaia_logger_t> logger);
    std::shared_ptr<gaia_logger_t> get(const std::string& logger_name);
    std::shared_ptr<gaia_logger_t> default_logger();

  private:
    std::unordered_map<std::string, std::shared_ptr<gaia_logger_t>> m_loggers;

    // Use the lock to ensure exclusive access to loggers.
    mutable shared_mutex m_lock;
};

class logger_exception_t : public gaia_exception {
  public:
    explicit logger_exception_t(const string& message) {
        m_message = message;
    }
};

} // namespace gaia::common::logging