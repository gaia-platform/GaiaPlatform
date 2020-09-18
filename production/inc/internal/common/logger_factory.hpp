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

class logger_factory_t {
public:
    constexpr static const char* c_uninitialized_logger = "uninitialized";

    constexpr static const char* c_sys_logger = "sys";
    constexpr static const char* c_db_logger = "db";
    constexpr static const char* c_scheduler_logger = "scheduler";
    constexpr static const char* c_catalog_logger = "catalog";

    /** Default logging path used if none is specified via configuration. */
    constexpr static const char* c_default_log_path = "logs/gaia.log";

    /** Default location of the log configuration file */
    // TODO it is unclear to me how we can provide this
    constexpr static const char* c_default_log_conf_path = "log_conf.toml";

    logger_factory_t(const logger_factory_t&) = delete;
    logger_factory_t& operator=(const logger_factory_t&) = delete;
    logger_factory_t(logger_factory_t&&) = delete;
    logger_factory_t& operator=(logger_factory_t&&) = delete;

    static logger_factory_t& get();

    bool init_logging(const string& config_path);
    bool is_logging_initialized() const;
    bool stop_logging();

private:
    static void create_log_dir_if_not_exists(const char* log_file_path);

private:
    logger_factory_t() = default;
    shared_mutex m_log_init_mutex;
    bool m_is_log_initialized = false;

    // well-known loggers
    logger_ptr_t m_sys;
    logger_ptr_t m_db;
    logger_ptr_t m_scheduler;
    logger_ptr_t m_catalog;
};

/*@}*/
} // namespace logging
/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
