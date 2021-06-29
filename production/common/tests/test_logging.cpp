/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "gaia_internal/common/debug_logger.hpp"
#include "gaia_internal/common/logger_internal.hpp"

using namespace std;

static constexpr char c_const_char_msg[] = "const char star message";
static const std::string c_string_msg = "string message";
static constexpr int64_t c_int_msg = 1234;

void verify_uninitialized_loggers()
{
    EXPECT_THROW(gaia_log::sys(), gaia_log::logger_exception_t);
    EXPECT_THROW(gaia_log::db(), gaia_log::logger_exception_t);
    EXPECT_THROW(gaia_log::rules(), gaia_log::logger_exception_t);
    EXPECT_THROW(gaia_log::rules_stats(), gaia_log::logger_exception_t);
    EXPECT_THROW(gaia_log::catalog(), gaia_log::logger_exception_t);
    EXPECT_THROW(gaia_log::app(), gaia_log::logger_exception_t);
}

TEST(logger_test, logger_api)
{
    verify_uninitialized_loggers();

    gaia_log::initialize("./gaia_log.conf");

    vector<gaia_log::logger_t> loggers = {
        gaia_log::sys(),
        gaia_log::catalog(),
        gaia_log::rules(),
        gaia_log::db(),
        gaia_log::rules_stats(),
        gaia_log::app()};

    for (auto logger : loggers)
    {
        logger.trace("trace");
        logger.trace("trace const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
        logger.debug("debug");
        logger.debug("debug const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
        logger.info("info");
        logger.info("info const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
        logger.warn("warn");
        logger.warn("warn const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
        logger.error("error");
        logger.error("error const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
        logger.critical("critical");
        logger.critical("critical const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
    }

    gaia_log::shutdown();

    // Sanity check that we are uninitialized now.
    verify_uninitialized_loggers();
}

// Creates a debug logger, which exposes its underlying spdlog member.
// It is used to manually set the log level for loggers like app() and sys().
gaia::common::logging::logger_t* create_debug_logger(
    const gaia_spdlog::level::level_enum log_level, const char* logger_name)
{
    gaia_log::debug_logger_t* debug_logger_ptr = gaia_log::debug_logger_t::create(logger_name);
    auto spdlogger = debug_logger_ptr->get_spdlogger();
    spdlogger->set_level(log_level);
    return debug_logger_ptr;
}

void verify_log_levels(gaia_spdlog::level::level_enum log_level, bool trace_status, bool debug_status, bool info_status, bool warn_status, bool error_status, bool critical_status)
{
    gaia_log::initialize({});

    // These functions let you replace the default system loggers with your own.
    gaia_log::set_sys_logger(create_debug_logger(log_level, "sys_logger"));
    gaia_log::set_db_logger(create_debug_logger(log_level, "db_logger"));
    gaia_log::set_rules_logger(create_debug_logger(log_level, "rules_logger"));
    gaia_log::set_rules_stats_logger(create_debug_logger(log_level, "rules_stats_logger"));
    gaia_log::set_catalog_logger(create_debug_logger(log_level, "catalog_logger"));
    gaia_log::set_app_logger(create_debug_logger(log_level, "app_logger"));

    vector<gaia_log::logger_t> loggers = {
        gaia_log::sys(),
        gaia_log::catalog(),
        gaia_log::rules(),
        gaia_log::db(),
        gaia_log::rules_stats(),
        gaia_log::app()};

    for (auto logger : loggers)
    {
        EXPECT_EQ(logger.is_trace_enabled(), trace_status);
        EXPECT_EQ(logger.is_debug_enabled(), debug_status);
        EXPECT_EQ(logger.is_info_enabled(), info_status);
        EXPECT_EQ(logger.is_warn_enabled(), warn_status);
        EXPECT_EQ(logger.is_error_enabled(), error_status);
        EXPECT_EQ(logger.is_critical_enabled(), critical_status);
    }

    gaia_log::shutdown();
}

TEST(logger_test, is_log_level_enabled)
{
    // Trace is the deepest log level, so every log level must be active.
    verify_log_levels(gaia_spdlog::level::trace, true, true, true, true, true, true);

    verify_log_levels(gaia_spdlog::level::debug, false, true, true, true, true, true);

    verify_log_levels(gaia_spdlog::level::info, false, false, true, true, true, true);

    verify_log_levels(gaia_spdlog::level::warn, false, false, false, true, true, true);

    verify_log_levels(gaia_spdlog::level::err, false, false, false, false, true, true);

    // Critical is the shallowest log level, so all other log levels must be inactive.
    verify_log_levels(gaia_spdlog::level::critical, false, false, false, false, false, true);

    // All log levels should be inactive if the level is "off".
    verify_log_levels(gaia_spdlog::level::off, false, false, false, false, false, false);
}
