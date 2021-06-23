/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

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

void verify_log_levels(string log_config_path, bool trace_status, bool debug_status, bool info_status, bool warn_status, bool error_status, bool critical_status)
{
    gaia_log::initialize(log_config_path);

    vector<gaia_log::logger_t> loggers = {
        gaia_log::sys(),
        gaia_log::catalog(),
        gaia_log::rules(),
        gaia_log::db(),
        gaia_log::rules_stats(),
        gaia_log::app()};

    for (auto logger : loggers)
    {
        EXPECT_EQ(logger.trace(), trace_status);
        EXPECT_EQ(logger.debug(), debug_status);
        EXPECT_EQ(logger.info(), info_status);
        EXPECT_EQ(logger.warn(), warn_status);
        EXPECT_EQ(logger.error(), error_status);
        EXPECT_EQ(logger.critical(), critical_status);
    }

    gaia_log::shutdown();
}

TEST(logger_test, is_log_level_enabled)
{
    // Trace is the deepest log level, so every log level must be active.
    verify_log_levels("./logging_configs/log_trace.conf", true, true, true, true, true, true);

    verify_log_levels("./logging_configs/log_debug.conf", false, true, true, true, true, true);

    verify_log_levels("./logging_configs/log_info.conf", false, false, true, true, true, true);

    verify_log_levels("./logging_configs/log_warn.conf", false, false, false, true, true, true);

    verify_log_levels("./logging_configs/log_error.conf", false, false, false, false, true, true);

    // Critical is the shallowest log level, so all other log levels must be inactive.
    verify_log_levels("./logging_configs/log_critical.conf", false, false, false, false, false, true);

    // All log levels should be inactive if the level is "off".
    verify_log_levels("./logging_configs/log_off.conf", false, false, false, false, false, false);
}
