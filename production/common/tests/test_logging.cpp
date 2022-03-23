/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <gtest/gtest.h>

#include "gaia/exceptions.hpp"
#include "gaia/optional.hpp"

#include "gaia_internal/common/debug_logger.hpp"
#include "gaia_internal/common/logger.hpp"

using namespace std;

static constexpr char c_const_char_msg[] = "const char star message";
static const std::string c_string_msg = "string message";
static constexpr int64_t c_int_msg = 1234;

void verify_uninitialized_loggers()
{
    EXPECT_THROW(gaia_log::sys(), gaia_log::logger_exception);
    EXPECT_THROW(gaia_log::db(), gaia_log::logger_exception);
    EXPECT_THROW(gaia_log::rules(), gaia_log::logger_exception);
    EXPECT_THROW(gaia_log::rules_stats(), gaia_log::logger_exception);
    EXPECT_THROW(gaia_log::catalog(), gaia_log::logger_exception);
    EXPECT_THROW(gaia_log::app(), gaia_log::logger_exception);
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

void verify_log_levels(gaia_spdlog::level::level_enum log_level)
{
    gaia_log::initialize({});

    // These functions let you replace the default system loggers with your own.
    gaia_log::set_sys_logger(create_debug_logger(log_level, "sys_logger"));
    gaia_log::set_db_logger(create_debug_logger(log_level, "db_logger"));
    gaia_log::set_rules_logger(create_debug_logger(log_level, "rules_logger"));
    gaia_log::set_rules_stats_logger(create_debug_logger(log_level, "rules_stats_logger"));
    gaia_log::set_catalog_logger(create_debug_logger(log_level, "catalog_logger"));
    gaia_log::set_app_logger(create_debug_logger(log_level, "app_logger"));

    bool f_trace = false, f_debug = false, f_info = false, f_warn = false, f_error = false, f_critical = false;
    // Fall through from the most to least verbose log levels.
    switch (log_level)
    {
    case gaia_spdlog::level::trace:
        f_trace = true;
    case gaia_spdlog::level::debug:
        f_debug = true;
    case gaia_spdlog::level::info:
        f_info = true;
    case gaia_spdlog::level::warn:
        f_warn = true;
    case gaia_spdlog::level::err:
        f_error = true;
    case gaia_spdlog::level::critical:
        f_critical = true;
    default:
        break;
    }

    vector<gaia_log::logger_t> loggers = {
        gaia_log::sys(),
        gaia_log::catalog(),
        gaia_log::rules(),
        gaia_log::db(),
        gaia_log::rules_stats(),
        gaia_log::app()};

    for (auto logger : loggers)
    {
        EXPECT_EQ(logger.is_trace_enabled(), f_trace);
        EXPECT_EQ(logger.is_debug_enabled(), f_debug);
        EXPECT_EQ(logger.is_info_enabled(), f_info);
        EXPECT_EQ(logger.is_warn_enabled(), f_warn);
        EXPECT_EQ(logger.is_error_enabled(), f_error);
        EXPECT_EQ(logger.is_critical_enabled(), f_critical);
    }

    gaia_log::shutdown();
}

TEST(logger_test, is_log_level_enabled)
{
    verify_log_levels(gaia_spdlog::level::trace);
    verify_log_levels(gaia_spdlog::level::debug);
    verify_log_levels(gaia_spdlog::level::info);
    verify_log_levels(gaia_spdlog::level::warn);
    verify_log_levels(gaia_spdlog::level::err);
    verify_log_levels(gaia_spdlog::level::critical);
    verify_log_levels(gaia_spdlog::level::off);
}

template <typename T>
void verify_optional_logging(T value)
{
    gaia::common::optional_t<T> optional, null_optional;
    optional = value;
    null_optional = gaia::common::nullopt;
    gaia_log::app().info("Optional has value '{}', '{}'", optional, null_optional);
}

TEST(logger_test, optional)
{
    gaia_log::initialize("./gaia_log.conf");
    verify_optional_logging<uint8_t>(1);
    verify_optional_logging<int8_t>(2);
    verify_optional_logging<uint16_t>(3);
    verify_optional_logging<int16_t>(4);
    verify_optional_logging<uint32_t>(5);
    verify_optional_logging<int32_t>(6);
    verify_optional_logging<uint64_t>(7);
    verify_optional_logging<int64_t>(8);
    verify_optional_logging<float>(9.0);
    verify_optional_logging<double>(10.0);
    verify_optional_logging<bool>(false);
    gaia_log::shutdown();
}
