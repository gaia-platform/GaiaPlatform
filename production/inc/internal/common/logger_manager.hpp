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
#include "logger.hpp"

namespace gaia {
/**
 * \addtogroup Gaia
 * @{
 */
namespace common {
/**
 * \addtogroup Common
 * @{
 */
namespace logging {

class logger_manager_t {

public:
    constexpr static const char* c_sys_logger = "sys";
    constexpr static const char* c_db_logger = "db";
    constexpr static const char* c_scheduler_logger = "scheduler";
    constexpr static const char* c_catalog_logger = "catalog";

    /** Default logging path used if none is specified via configuration. */
    constexpr static const char* c_default_log_path = "logs/gaia.log";

    /** Default location of the log configuration file */
    // REVIEW it is unclear to me how we can provide this
    constexpr static const char* c_default_log_conf_path = "log_conf.toml";

    logger_manager_t(const logger_manager_t&) = delete;
    logger_manager_t& operator=(const logger_manager_t&) = delete;
    logger_manager_t(logger_manager_t&&) = delete;
    logger_manager_t& operator=(logger_manager_t&&) = delete;

    static logger_manager_t& get();

    bool init_logging(const string& config_path);
    bool is_logging_initialized() const;
    bool stop_logging();

private:
    static void create_log_dir_if_not_exists(const char* log_file_path);

private:
    logger_manager_t() = default;
    shared_mutex m_log_init_mutex;
    bool m_is_log_initialized = false;

    // well-known loggers
    friend logger_t& gaia_log::sys();
    friend logger_t& gaia_log::db();
    friend logger_t& gaia_log::scheduler();
    friend logger_t& gaia_log::catalog();

    logger_ptr_t m_sys_logger;
    logger_ptr_t m_db_logger;
    logger_ptr_t m_scheduler_logger;
    logger_ptr_t m_catalog_logger;
};

/*@}*/
} // namespace logging
/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
