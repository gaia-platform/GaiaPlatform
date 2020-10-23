/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

#include "gaia_exception.hpp"
#include "logger.hpp"

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup Common
 * @{
 */
namespace logging
{

class logger_manager_t
{

public:
    static constexpr char c_sys_logger[] = "sys";
    static constexpr char c_db_logger[] = "db";
    static constexpr char c_rules_logger[] = "rules";
    static constexpr char c_rules_stats_logger[] = "rules_stats";
    static constexpr char c_catalog_logger[] = "catalog";

    /** Default logging path used if none is specified via configuration. */
    static constexpr char c_default_log_path[] = "logs/gaia.log";

    /** Default location of the log configuration file */
    // REVIEW it is unclear to me how we can provide this
    static constexpr char c_default_log_conf_path[] = "log_conf.toml";

    logger_manager_t(const logger_manager_t&) = delete;
    logger_manager_t& operator=(const logger_manager_t&) = delete;
    logger_manager_t(logger_manager_t&&) = delete;
    logger_manager_t& operator=(logger_manager_t&&) = delete;

    static logger_manager_t& get();

    // Retrieve well-known loggers instances.
    logger_t& sys_logger()
    {
        if (!m_is_log_initialized)
        {
            uninitialized_failure();
        }
        return *m_sys_logger;
    };

    logger_t& db_logger()
    {
        if (!m_is_log_initialized)
        {
            uninitialized_failure();
        }
        return *m_db_logger;
    }

    logger_t& rules_logger()
    {
        if (!m_is_log_initialized)
        {
            uninitialized_failure();
        }
        return *m_rules_logger;
    }

    logger_t& catalog_logger()
    {
        if (!m_is_log_initialized)
        {
            uninitialized_failure();
        }
        return *m_catalog_logger;
    }

    logger_t& rules_stats_logger()
    {
        if (!m_is_log_initialized)
        {
            uninitialized_failure();
        }
        return *m_rules_stats_logger;
    }

    bool init_logging(const string& config_path);
    bool stop_logging();

private:
    logger_manager_t() = default;
    static void create_log_dir_if_not_exists(const char* log_file_path);
    static void uninitialized_failure()
    {
        throw logger_exception_t("Logger sub-system not initialized!");
    }

private:
    mutex m_log_init_mutex;
    atomic_bool m_is_log_initialized = false;

    // Well-known loggers
    logger_ptr_t m_sys_logger;
    logger_ptr_t m_db_logger;
    logger_ptr_t m_rules_logger;
    logger_ptr_t m_rules_stats_logger;
    logger_ptr_t m_catalog_logger;
};

/*@}*/
} // namespace logging
/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
