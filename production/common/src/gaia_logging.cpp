/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <filesystem>
#include <iostream>

#include "gaia_logging.hpp"
#include "gaia_logging_spdlog.hpp"

#include "spdlog/async.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/syslog_sink.h"
#include "spdlog_setup/conf.h"

namespace fs = std::filesystem;

namespace gaia::common::logging {

static shared_mutex log_init_mutex;
static bool log_initialized = false;

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

    void register_logger(const std::shared_ptr<gaia_logger_t>& logger) {
        unique_lock lock(m_lock);

        if (m_loggers.find(logger->get_name()) != m_loggers.end()) {
            throw logger_exception_t("Logger already exists: " + logger->get_name());
        }

        m_loggers.insert({logger->get_name(), logger});
    }

    std::shared_ptr<gaia_logger_t> get(const std::string& logger_name) {
        shared_lock lock(m_lock);
        auto logger = m_loggers.find(logger_name);

        if (logger != m_loggers.end()) {
            return logger->second;
        }

        return nullptr;
    }

    void unregister_logger(const std::string& logger_name) {
        unique_lock lock(m_lock);
        m_loggers.erase(logger_name);
    }

    std::shared_ptr<gaia_logger_t> default_logger() {
        return get(c_gaia_root_logger);
    }

    void clear() {
        unique_lock lock(m_lock);
        m_loggers.clear();
    }

  private:
    std::unordered_map<std::string, std::shared_ptr<gaia_logger_t>> m_loggers;

    // Use the lock to ensure exclusive access to loggers.
    mutable shared_mutex m_lock;
};

void init_logging(const string& config_path) {
    unique_lock lock(log_init_mutex);

    if (log_initialized) {
        throw logger_exception_t("Logging already initialized");
    }

    try {
        spdlog_setup::from_file(config_path);
    } catch (spdlog_setup::setup_error& e) {
        cerr << "An error occurred while configuring logger from file '" << config_path << "': " << e.what() << endl;
    }

    if (!spdlog::get(c_gaia_root_logger)) {
        configure_spdlog_default();
    }

    std::vector<std::string> logger_names;
    spdlog::apply_all([&](const std::shared_ptr<spdlog::logger>& l) {
        logger_names.push_back(l->name());
    });

    for (const std::string& logger_name : logger_names) {
        logger_registry_t::instance().register_logger(make_shared<gaia_logger_t>(logger_name));
    }

    log_initialized = true;
}

bool is_logging_initialized() {
    shared_lock lock(log_init_mutex);
    return log_initialized;
}

bool stop_logging() {
    unique_lock lock(log_init_mutex);

    if (!log_initialized) {
        return false;
    }

    logger_registry_t::instance().clear();
    spdlog::shutdown();
    log_initialized = false;
    return true;
}

gaia_logger_t::gaia_logger_t(const string& logger_name) : m_logger_name(logger_name) {
    m_impl_ptr = make_unique<log_impl_t>(logger_name);
}

gaia_logger_t::~gaia_logger_t() = default;

const std::string& gaia_logger_t::get_name() {
    return m_logger_name;
}

void gaia_logger_t::log(log_level_t level, const char* msg) {
    m_impl_ptr->get_logger().log(to_spdlog_level(level), msg);
}

void gaia_logger_t::trace(const char* msg) {
    m_impl_ptr->get_logger().trace(msg);
}

void gaia_logger_t::debug(const char* msg) {
    m_impl_ptr->get_logger().debug(msg);
}

void gaia_logger_t::info(const char* msg) {
    m_impl_ptr->get_logger().info(msg);
}

void gaia_logger_t::warn(const char* msg) {
    m_impl_ptr->get_logger().warn(msg);
}

void gaia_logger_t::error(const char* msg) {
    m_impl_ptr->get_logger().error(msg);
}

void gaia_logger_t::critical(const char* msg) {
    m_impl_ptr->get_logger().critical(msg);
}

void register_logger(const std::shared_ptr<gaia_logger_t>& logger) {
    logger_registry_t::instance().register_logger(logger);
}

std::shared_ptr<gaia_logger_t> get(const std::string& logger_name) {
    return logger_registry_t::instance().get(logger_name);
}

void unregister_logger(const std::string& logger_name) {
    logger_registry_t::instance().unregister_logger(logger_name);
}

std::shared_ptr<gaia_logger_t> default_logger() {
    return logger_registry_t::instance().default_logger();
}

void clear_loggers() {
    logger_registry_t::instance().clear();
}

void create_log_dir_if_not_exists(const char* log_file_path) {
    fs::path path(log_file_path);
    fs::path parent = path.parent_path();
    fs::create_directories(parent);
}

void configure_spdlog_default() {
    std::cerr << c_gaia_root_logger << " logger not found, creating it with default configuration:" << endl;

    spdlog::init_thread_pool(c_default_spdlog_queue_size, c_default_spdlog_thread_count);

    std::cerr << " -queue_size: " << c_default_spdlog_queue_size << endl;
    std::cerr << " -thread_pool_size: " << c_default_spdlog_thread_count << endl;

    auto console_sink = make_shared<spdlog::sinks::stdout_sink_mt>();
    std::cerr << " -console_sink: stdout" << endl;

    create_log_dir_if_not_exists(c_default_log_path);
    auto file_sink = make_shared<spdlog::sinks::basic_file_sink_mt>(c_default_log_path, true);
    std::cerr << " -file_sink: " << c_default_log_path << endl;

    string syslog_ident = "gaia";
    int syslog_option = 0;
    int syslog_facility = LOG_USER;
    auto syslog_sink = make_shared<spdlog::sinks::syslog_sink_mt>(syslog_ident, syslog_option, syslog_facility);
    std::cerr << " -syslog_sink: " << syslog_ident << endl;

    spdlog::sinks_init_list sink_list{file_sink, console_sink, syslog_sink};

    // It has to be a shared pointer. This is by design because the logger is shared with the logging thread.
    auto logger = make_shared<spdlog::async_logger>(c_gaia_root_logger,
        sink_list.begin(), sink_list.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    logger->set_level(c_default_spdlog_level);
    logger->set_pattern(c_default_spdlog_pattern);

    std::cerr << " -level: info " << endl;
    std::cerr << " -pattern: " << c_default_spdlog_pattern << endl;
    std::cerr << " Created async logger." << endl
              << std::flush;

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
