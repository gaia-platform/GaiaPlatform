/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/common/logger_manager.hpp"

#include <filesystem>
#include <iostream>

#include "spdlog/async.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog_setup/conf.h"

#include "gaia_internal/common/logger_spdlog.hpp"

namespace fs = std::filesystem;

using namespace std;

namespace gaia::common::logging
{

//
// Implementation of logger_manager_t
//
logger_manager_t& logger_manager_t::get()
{
    static logger_manager_t instance;
    return instance;
}

bool logger_manager_t::init_logging(const string& config_path)
{
    unique_lock lock(m_log_init_mutex);

    if (m_is_log_initialized)
    {
        return false;
    }

    if (!config_path.empty())
    {
        // This may throw if the configuration path is incorrect.
        spdlog_setup::from_file(config_path);
    }

    if (!spdlog::thread_pool())
    {
        spdlog::init_thread_pool(spdlog_defaults::c_default_queue_size, spdlog_defaults::c_default_thread_count);
    }

    m_sys_logger = shared_ptr<logger_t>(new internal_logger_t(c_sys_logger));
    m_db_logger = shared_ptr<logger_t>(new internal_logger_t(c_db_logger));
    m_rules_logger = shared_ptr<logger_t>(new internal_logger_t(c_rules_logger));
    m_catalog_logger = shared_ptr<logger_t>(new internal_logger_t(c_catalog_logger));
    m_rules_stats_logger = shared_ptr<logger_t>(new internal_logger_t(c_rules_stats_logger));
    m_app_logger = shared_ptr<logger_t>(new internal_logger_t(c_app_logger));

    m_is_log_initialized = true;

    return true;
}

bool logger_manager_t::stop_logging()
{
    unique_lock lock(m_log_init_mutex);

    if (!m_is_log_initialized)
    {
        return false;
    }

    // If we see we need more loggers we could re-introduce the logger registry
    // concept (see spdlog registry).
    // For now, considering the limited number of loggers we have, it is ok
    // to do this way.
    m_catalog_logger = nullptr;
    m_db_logger = nullptr;
    m_sys_logger = nullptr;
    m_rules_logger = nullptr;
    m_rules_stats_logger = nullptr;
    m_app_logger = nullptr;

    spdlog::shutdown();

    m_is_log_initialized = false;
    return true;
}

void logger_manager_t::create_log_dir_if_not_exists(const char* log_file_path)
{
    fs::path path(log_file_path);
    fs::path parent = path.parent_path();
    fs::create_directories(parent);
}

shared_ptr<spdlog::logger> spdlog_defaults::create_logger_with_default_settings(const std::string& logger_name)
{

    auto console_sink = make_shared<spdlog::sinks::stderr_sink_mt>();

    // Keeping commented out on purpose. We need to decide what are meaningful default sinks.

    //    create_log_dir_if_not_exists(c_default_log_path);
    //    auto file_sink = make_shared<spdlog::sinks::basic_file_sink_mt>(c_default_log_path, true);
    //
    //    string syslog_identifier = "gaia";
    //    int syslog_option = 0;
    //    int syslog_facility = LOG_USER;
    //    auto syslog_sink = make_shared<spdlog::sinks::syslog_sink_mt>(syslog_identifier, syslog_option, syslog_facility);

    spdlog::sinks_init_list sink_list{console_sink /* ,file_sink, syslog_sink*/};

    // It has to be a shared pointer. This is by design because the logger is shared with the logging thread.
    auto logger = make_shared<spdlog::async_logger>(
        logger_name, sink_list.begin(), sink_list.end(),
        spdlog::thread_pool(), spdlog::async_overflow_policy::block);

    logger->set_level(spdlog_defaults::c_default_level);
    logger->set_pattern(spdlog_defaults::c_default_pattern);

    return logger;
}

} // namespace gaia::common::logging
