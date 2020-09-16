/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_logging.hpp"

#include <filesystem>
#include <iostream>

#include "spdlog/async.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/syslog_sink.h"
#include "spdlog_setup/conf.h"
#include "gaia_logging_spdlog.hpp"

namespace fs = std::filesystem;

namespace gaia::common::logging {

/**
 * Inits the logging system. This is invoked once when the module is included.
 *
 * TODO currently the configuration logic is pretty naive, it checks
 *   for the existence of one file and fallback to the default configuration.
 *   There is a JIRA to track improvement of this logic:
 *   https://gaiaplatform.atlassian.net/browse/GAIAPLAT-358
*/
bool init_logging();
static bool init = init_logging();

class logger_registry_t {
public:
    logger_registry_t(const logger_registry_t&) = delete;
    logger_registry_t& operator=(const logger_registry_t&) = delete;
    logger_registry_t(logger_registry_t&&) = delete;
    logger_registry_t& operator=(logger_registry_t&&) = delete;

    static logger_registry_t& get() {
        static logger_registry_t instance;
        return instance;
    }

    logger_registry_t() = default;

    void register_logger(const std::shared_ptr<gaia_logger_t>& logger) {
        unique_lock lock(m_lock);

        if (m_loggers.find(logger->get_name()) != m_loggers.end()) {
            throw logger_exception_t("Logger already exists: " + logger->get_name());
        }

        m_loggers.insert({logger->get_name(), logger});
    }

    std::shared_ptr<gaia_logger_t> get(const std::string& logger_name) const {
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

    std::shared_ptr<gaia_logger_t> default_logger() const {
        auto logger = get(c_gaia_root_logger);

        if (!logger) {
            // TODO instead of failing should we setup the default config?
            throw logger_exception_t("Logging not initialized, call gaia_log::init_logging()");
        }

        return logger;
    }

    void clear() {
        unique_lock lock(m_lock);
        m_loggers.clear();
    }

private:
    std::unordered_map<std::string, std::shared_ptr<gaia_logger_t>> m_loggers;
    mutable shared_mutex m_lock;
};

class logger_initializer_t {
public:
    logger_initializer_t(const logger_initializer_t&) = delete;
    logger_initializer_t& operator=(const logger_initializer_t&) = delete;
    logger_initializer_t(logger_initializer_t&&) = delete;
    logger_initializer_t& operator=(logger_initializer_t&&) = delete;

    static logger_initializer_t& get() {
        static logger_initializer_t instance;
        return instance;
    }

    bool init_logging(const string& config_path) {
        unique_lock lock(m_log_init_mutex);

        if (m_is_log_initialized) {
            return false;
        }

        try {
            spdlog_setup::from_file(config_path);
        } catch (spdlog_setup::setup_error& e) {
            // exception not thrown because we want ot fallback to the default configuration
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
            logger_registry_t::get().register_logger(make_shared<gaia_logger_t>(logger_name));
        }

        m_is_log_initialized = true;
        return true;
    }

    bool is_logging_initialized() const {
        return m_is_log_initialized;
    }

    bool stop_logging() {
        unique_lock lock(m_log_init_mutex);

        if (!m_is_log_initialized) {
            return false;
        }

        logger_registry_t::get().clear();
        spdlog::shutdown();
        m_is_log_initialized = false;
        return true;
    }

private:
    logger_initializer_t() = default;
    shared_mutex m_log_init_mutex;
    bool m_is_log_initialized = false;
};

bool init_logging() {
    return logger_initializer_t::get().init_logging(c_default_log_conf_path);
}

bool is_logging_initialized() {
    return logger_initializer_t::get().is_logging_initialized();
}

gaia_logger_t::gaia_logger_t(const string& logger_name) : m_logger_name(logger_name) {
    m_pimpl = make_unique<log_impl_t>(logger_name);
}

gaia_logger_t::~gaia_logger_t() = default;

const std::string& gaia_logger_t::get_name() const {
    return m_logger_name;
}

void gaia_logger_t::log(log_level_t level, const char* msg) {
    m_pimpl->get_logger().log(to_spdlog_level(level), msg);
}

void gaia_logger_t::trace(const char* msg) {
    m_pimpl->get_logger().trace(msg);
}

void gaia_logger_t::debug(const char* msg) {
    m_pimpl->get_logger().debug(msg);
}

void gaia_logger_t::info(const char* msg) {
    m_pimpl->get_logger().info(msg);
}

void gaia_logger_t::warn(const char* msg) {
    m_pimpl->get_logger().warn(msg);
}

void gaia_logger_t::error(const char* msg) {
    m_pimpl->get_logger().error(msg);
}

void gaia_logger_t::critical(const char* msg) {
    m_pimpl->get_logger().critical(msg);
}

void register_logger(const std::shared_ptr<gaia_logger_t>& logger) {
    logger_registry_t::get().register_logger(logger);
}

std::shared_ptr<gaia_logger_t> get(const std::string& logger_name) {
    return logger_registry_t::get().get(logger_name);
}

void unregister_logger(const std::string& logger_name) {
    logger_registry_t::get().unregister_logger(logger_name);
}

std::shared_ptr<gaia_logger_t> default_logger() {
    return logger_registry_t::get().default_logger();
}

void clear_loggers() {
    logger_registry_t::get().clear();
}

void create_log_dir_if_not_exists(const char* log_file_path) {
    fs::path path(log_file_path);
    fs::path parent = path.parent_path();
    fs::create_directories(parent);
}

// TODO decide the default configuration. ATM we configure 3 sinks, that is probably overkilling.
//   https://gaiaplatform.atlassian.net/browse/GAIAPLAT-358
void configure_spdlog_default() {
    std::cerr << "Using default log configuration" << endl
              << flush;

    spdlog::init_thread_pool(c_default_spdlog_queue_size, c_default_spdlog_thread_count);

    auto console_sink = make_shared<spdlog::sinks::stdout_sink_mt>();

    create_log_dir_if_not_exists(c_default_log_path);
    auto file_sink = make_shared<spdlog::sinks::basic_file_sink_mt>(c_default_log_path, true);

    string syslog_identifier = "gaia";
    int syslog_option = 0;
    int syslog_facility = LOG_USER;
    auto syslog_sink = make_shared<spdlog::sinks::syslog_sink_mt>(syslog_identifier, syslog_option, syslog_facility);

    spdlog::sinks_init_list sink_list{file_sink, console_sink, syslog_sink};

    // It has to be a shared pointer. This is by design because the logger is shared with the logging thread.
    auto logger = make_shared<spdlog::async_logger>(c_gaia_root_logger,
        sink_list.begin(), sink_list.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    logger->set_level(c_default_spdlog_level);
    logger->set_pattern(c_default_spdlog_pattern);

    spdlog::register_logger(logger);
}

shared_ptr<spdlog::logger> create_spdlog_logger(const string& logger_name) {
    auto root_logger = spdlog::get(c_gaia_root_logger);
    return root_logger->clone(logger_name);
}

spdlog::level::level_enum to_spdlog_level(gaia_log::log_level_t gaia_level) {
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
