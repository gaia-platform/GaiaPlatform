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
#include "logger_spdlog.hpp"

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

/** Rule engine */
logger_t& re();

/** Rule engine stats */
logger_t& re_stats();

/*@}*/
} // namespace logging
/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
