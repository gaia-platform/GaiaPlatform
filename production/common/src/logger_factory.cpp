/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "logger_factory.hpp"

#include <filesystem>
#include <iostream>

#include "spdlog/async.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/syslog_sink.h"
#include "spdlog_setup/conf.h"
#include "logger_spdlog.hpp"

namespace fs = std::filesystem;

namespace gaia::common::logging {

//
// Implementation of logger_factory_t
//
logger_factory_t& logger_factory_t::get() {
        static logger_factory_t instance;
        return instance;
}

bool logger_factory_t::init_logging(const string& config_path) {
    unique_lock lock(m_log_init_mutex);

    if (m_is_log_initialized) {
        return false;
    }


    if (!config_path.empty()) {
        try {
            spdlog_setup::from_file(config_path);
        } catch (spdlog_setup::setup_error& e) {
            // exception not thrown because we want ot fallback to the default configuration
            cerr << "An error occurred while configuring logger from file '" << config_path << "': " << e.what() << endl;
        }
    }

    if (!spdlog::get(c_gaia_root_logger)) {
        configure_spdlog_default();
    }
    

    // DAX:  here we would to validate that the configuration file 
    // has the well-known loggers.
    // Also, produce an internal map of logger_name to shared_ptr logger instance

    //std::vector<std::string> logger_names;
    //spdlog::apply_all([&](const std::shared_ptr<spdlog::logger>& l) {
    //    logger_names.push_back(l->name());
    //});

    m_sys = make_shared<logger_t>(c_gaia_root_logger);
    g_sys = *m_sys;

    m_is_log_initialized = true;

    return true;
}

bool logger_factory_t::is_logging_initialized() const {
    return m_is_log_initialized;
}

bool logger_factory_t::stop_logging() {
    unique_lock lock(m_log_init_mutex);

    if (!m_is_log_initialized) {
        return false;
    }

    spdlog::shutdown();
    m_is_log_initialized = false;
    return true;
}

void logger_factory_t::create_log_dir_if_not_exists(const char* log_file_path) {
    fs::path path(log_file_path);
    fs::path parent = path.parent_path();
    fs::create_directories(parent);
}

// TODO decide the default configuration. ATM we configure 3 sinks, that is probably overkilling.
//   https://gaiaplatform.atlassian.net/browse/GAIAPLAT-358
void logger_factory_t::configure_spdlog_default() {
    std::cerr << "Using default log configuration" << endl
              << flush;

    spdlog::init_thread_pool(spdlog_defaults::c_default_queue_size, spdlog_defaults::c_default_thread_count);

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
    logger->set_level(spdlog_defaults::c_default_level);
    logger->set_pattern(spdlog_defaults::c_default_pattern);

    spdlog::register_logger(logger);
}

shared_ptr<spdlog::logger> logger_factory_t::create_spdlog_logger(const string& logger_name) {
    auto root_logger = spdlog::get(c_gaia_root_logger);
    return root_logger->clone(logger_name);
}

} // namespace gaia::common::logging
