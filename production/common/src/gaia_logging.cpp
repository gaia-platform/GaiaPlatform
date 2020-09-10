/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <memory>
#include <filesystem>

#include "gaia_logging.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "spdlog_setup/conf.h"

using std::cerr;
using std::endl;
using std::make_shared;
using std::shared_ptr;
using std::string_view;

namespace fs = std::filesystem;

namespace gaia::common::logging {

/**
 * Example: [2020-09-11T16:21:54-04:00] [critical] p:201405 t:201405 <gaia-root>: message
 *
 * where
 * - critical: level
 * - p: process
 * - t: thread
 * - gaia-root: logger name
 */
constexpr const char* c_default_spdlog_pattern = "[%Y-%m-%dT%T%z] [%l] p:%P t:%t <%n>: %v";

constexpr const size_t c_default_spdlog_queue_size = 8192;
constexpr const size_t c_default_spdlog_thread_count = 2;

/**
 * Configures spdlog to a default configuration. This function is called if
 * the configuration from file fails.
 */
void configure_spdlog_default();

/**
 * Creates a clone of the root_logger with the given name.
 */
shared_ptr<spdlog::logger> create_spdlog_logger(const string& logger_name);

spdlog::level::level_enum to_spdlog_level(gaia::common::logging::log_level_t gaia_level);

void init_logging(const string& config_path) {
    try {
        spdlog_setup::from_file(config_path);
    } catch (spdlog_setup::setup_error& e) {
        cerr << "An error occurred while configuring logger from file '" << config_path << "': " << e.what() << endl;
    }

    if (!spdlog::get(c_gaia_root_logger)) {
        configure_spdlog_default();
    }
}

class log_impl_t {
  public:
    explicit log_impl_t(const string& logger_name) {
        auto logger = spdlog::get(logger_name);

        if (!logger) {
            throw logger_exception_t("Logger not found: " + logger_name);
        }
        m_logger = create_spdlog_logger(logger_name);
    }

    spdlog::logger& get_logger() {
        return *m_logger;
    }

  private:
    shared_ptr<spdlog::logger> m_logger;
};

gaia_logger_t::gaia_logger_t(const string& logger_name) : m_logger_name(logger_name) {
    p_impl = make_unique<log_impl_t>(logger_name);
}

gaia_logger_t::~gaia_logger_t() = default;

const std::string& gaia_logger_t::get_name() {
    return m_logger_name;
}

// TODO I know C++ doesn't like templates to be in the CPP file. Not sure about the implications though.

template <typename... Args>
void gaia_logger_t::log(log_level_t level, string_view format, Args&... args) {
    p_impl->get_logger().log(to_spdlog_level(level), format, args...);
}

void gaia_logger_t::log(log_level_t level, std::string_view msg) {
    p_impl->get_logger().log(to_spdlog_level(level), msg);
}

template <typename... Args>
void gaia_logger_t::trace(std::string_view format, Args&... args) {
    p_impl->get_logger().trace(format, args...);
}

void gaia_logger_t::trace(std::string_view msg) {
    p_impl->get_logger().trace(msg);
}

template <typename... Args>
void gaia_logger_t::debug(std::string_view format, Args&... args) {
    p_impl->get_logger().debug(format, args...);
}

void gaia_logger_t::debug(std::string_view msg) {
    p_impl->get_logger().debug(msg);
}

template <typename... Args>
void gaia_logger_t::info(std::string_view format, Args&... args) {
    p_impl->get_logger().info(format, args...);
}

void gaia_logger_t::info(std::string_view msg) {
    p_impl->get_logger().info(msg);
}

template <typename... Args>
void gaia_logger_t::warn(std::string_view format, Args&... args) {
    p_impl->get_logger().warn(format, args...);
}

void gaia_logger_t::warn(std::string_view msg) {
    p_impl->get_logger().warn(msg);
}

template <typename... Args>
void gaia_logger_t::error(std::string_view format, Args&... args) {
    p_impl->get_logger().error(format, args...);
}

void gaia_logger_t::error(std::string_view msg) {
    p_impl->get_logger().error(msg);
}

template <typename... Args>
void gaia_logger_t::critical(std::string_view format, Args&... args) {
    p_impl->get_logger().critical(format, args...);
}

void gaia_logger_t::critical(std::string_view msg) {
    p_impl->get_logger().critical(msg);
}

void logger_registry_t::register_logger(std::shared_ptr<gaia_logger_t> logger) {
    scoped_lock lock(m_lock);

    if (m_loggers.find(logger->get_name()) != m_loggers.end()) {
        throw logger_exception_t("Logger already exists: " + logger->get_name());
    }

    m_loggers.insert({logger->get_name(), logger});
}

std::shared_ptr<gaia_logger_t> logger_registry_t::get(const string& logger_name) {
    shared_lock lock(m_lock);
    auto logger = m_loggers.find(logger_name);

    if (logger != m_loggers.end()) {
        return logger->second;
    }

    return nullptr;
}

std::shared_ptr<gaia_logger_t> logger_registry_t::default_logger() {
    return get(c_gaia_root_logger);
}

void create_log_dir_if_not_exists(std::string_view log_file) {
    fs::path path(log_file);
    fs::path parent = path.parent_path();

    if (fs::exists(parent)) {
        return;
    }

    fs::create_directories(parent);
}

void configure_spdlog_default() {
    cerr << c_gaia_root_logger << " logger not found, creating it with default configuration" << endl;

    spdlog::init_thread_pool(c_default_spdlog_queue_size, c_default_spdlog_thread_count);

    cerr << " -queue_size: " << c_default_spdlog_queue_size << endl;
    cerr << " -thread_pool: " << c_default_spdlog_thread_count << endl;

    auto console_sink = make_shared<spdlog::sinks::stderr_color_sink_mt>();
    cerr << " -console_sink: stderr" << endl;

    create_log_dir_if_not_exists(c_default_log_path);
    auto file_sink = make_shared<spdlog::sinks::basic_file_sink_mt>(c_default_log_path, true);

    string syslog_ident = "gaia";
    int syslog_option = 0;
    int syslog_facility = LOG_USER;
    //    bool enable_formatting = false;
    auto syslog_sink = make_shared<spdlog::sinks::syslog_sink_mt>(syslog_ident, syslog_option, syslog_facility);

    spdlog::sinks_init_list sink_list{/*file_sink,*/ console_sink, syslog_sink};

    // It has to be a shared pointer. This is by design because the logger is shared with the logging thread.
    auto logger = make_shared<spdlog::async_logger>(c_gaia_root_logger, sink_list.begin(), sink_list.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    logger->set_level(spdlog::level::info);
    logger->set_pattern(c_default_spdlog_pattern);

    spdlog::register_logger(logger);
}

shared_ptr<spdlog::logger> create_spdlog_logger(const string& logger_name) {
    auto root_logger = spdlog::get(c_gaia_root_logger);
    return root_logger->clone(logger_name);
}

spdlog::level::level_enum to_spdlog_level(gaia::common::logging::log_level_t gaia_level) {
    switch (gaia_level) {
    case log_level_t::trace:
        return spdlog::level::trace;
    case log_level_t::debug:
        return spdlog::level::debug;
    case log_level_t::info:
        return spdlog::level::info;
    case log_level_t::warn:
        return spdlog::level::warn;
    case log_level_t::err:
        return spdlog::level::err;
    case log_level_t::critical:
        return spdlog::level::critical;
    case log_level_t::off:
        return spdlog::level::off;
    default:
        throw logger_exception_t("Unsupported logging level: " + std::to_string(static_cast<std::underlying_type_t<log_level_t>>(gaia_level)));
    }
}

} // namespace gaia::common::logging