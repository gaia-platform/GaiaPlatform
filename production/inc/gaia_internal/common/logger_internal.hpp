/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <shared_mutex>
#include <sstream>
#include <string>
#include <unordered_map>

#include "gaia/exception.hpp"
#include "gaia/logger.hpp"

#include "gaia_internal/common/logger_spdlog.hpp"

namespace gaia
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup Gaia
 * @{
 */
namespace logging
{

/**
 * Gaia Internal logging API. Please use the loggers defined in this file.
 *
 * The logging system MUST be initialized calling `initialize(const string& m_config_path)`.
 *
 * The logging is performed via logger_t objects. Each instance of this class represent
 * a separated logger. Different submodules should use different loggers.
 * FUnctions for the different submodules are provided: sys(), db(), scheduler() etc..
 *
 * Calling:
 *
 *  gaia_log::sys().info("I'm the Sys logger")
 *  gaia_log::db().info("I'm the Storage Engine logger")
 *  gaia_log::scheduler().info("I'm the Rules logger")
 *
 * Dynamic creation of logger is not supported and generally discouraged. If you want to
 * add a new logger add a constant in this file.
 *
 * \addtogroup Logging
 * @{
 */

typedef std::shared_ptr<logger_t> logger_ptr_t;

/**
 * Expose a public constructor to build logger_t objects internally.
 */
class internal_logger_t : public logger_t
{
public:
    explicit internal_logger_t(const std::string& logger_name)
        : logger_t(logger_name){};
};

/**
 * Initializes all loggers from the passed in configuration. If the configuration
 * does not contain one of the required loggers (g_sys etc..) a default logger is
 * instantiated.
 */
void initialize(const std::string& config_path);

/**
 * Release the resources used by the logging framework.
 */
void shutdown();

/*
 * Exposed loggers. Filled in by initialize and destroyed on shutdown.
 */

/** System */
logger_t& sys();

/** Database */
logger_t& db();

/** Catalog */
logger_t& catalog();

/** Rules engine */
logger_t& rules();

/** Rules engine stats */
logger_t& rules_stats();

/*@}*/
} // namespace logging
/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
