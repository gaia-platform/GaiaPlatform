/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "logger.hpp"

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
}

TEST(logger_test, logger_api)
{
    verify_uninitialized_loggers();

    gaia_log::initialize("./log_conf.toml");

    vector<gaia_log::logger_t> loggers = {
        gaia_log::sys(),
        gaia_log::catalog(),
        gaia_log::rules(),
        gaia_log::db(),
        gaia_log::rules_stats()};

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

        logger.log(gaia_log::log_level_t::info, "Static message");
        logger.log(gaia_log::log_level_t::info, "Dynamic const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
    }

    gaia_log::shutdown();

    // Sanity check that we are uninitialized now.
    verify_uninitialized_loggers();
}
