/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include "gtest/gtest.h"
#include "logger.hpp"

static const char* const_char_msg = "const char star message";
static const std::string string_msg = "string message";
static const int64_t int_msg = 1234;

class test_logging_t : public ::testing::Test {

protected:
    static void SetUpTestSuite() {
        gaia_log::initialize(string());
    }
};

TEST_F(test_logging_t, logger_api) {

    vector<gaia_log::logger_t> loggers = {gaia_log::g_sys, gaia_log::g_catalog, gaia_log::g_scheduler, gaia_log::g_db};

    for (auto logger : loggers) {
        logger.trace("trace");
        logger.trace("trace const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
        logger.debug("debug");
        logger.debug("debug const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
        logger.info("info");
        logger.info("info const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
        logger.warn("warn");
        logger.warn("warn const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
        logger.error("error");
        logger.error("error const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
        logger.critical("critical");
        logger.critical("critical const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);

        logger.log(gaia_log::log_level_t::info, "Static message");
        logger.log(gaia_log::log_level_t::info, "Dynamic const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
    }
}
