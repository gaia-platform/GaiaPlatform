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
//static const char* bubu_logger = "BUBU Logger";

class test_logging_t : public ::testing::Test {

protected:
    void SetUp() override {
        // Some tests register the bubu_logger in the registry.
        // Removing it to prevent tests from interfering with
        // each other.
        //gaia_log::unregister_logger(bubu_logger);

        // DAX: should probably go in SetupTestSuite    
        // gaia_log::initialize(string());
    }
};

TEST_F(test_logging_t, logger_api) {
    //gaia_log::gaia_logger_t logger(gaia_log::c_gaia_root_logger);

    // DAX: should probably go in SetupTestSuite
    gaia_log::initialize(string());

    gaia_log::g_sys.trace("trace");
    gaia_log::g_sys.trace("trace const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
    gaia_log::g_sys.debug("debug");
    gaia_log::g_sys.debug("debug const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
    gaia_log::g_sys.info("info");
    gaia_log::g_sys.info("info const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
    gaia_log::g_sys.warn("warn");
    gaia_log::g_sys.warn("warn const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
    gaia_log::g_sys.error("error");
    gaia_log::g_sys.error("error const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
    gaia_log::g_sys.critical("critical");
    gaia_log::g_sys.critical("critical const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);

    gaia_log::g_sys.log(gaia_log::log_level_t::info, "Static message");
    gaia_log::g_sys.log(gaia_log::log_level_t::info, "Dynamic const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
}

/*
TEST_F(test_logging_t, default_logger_api) {
    gaia_log::g_default.trace
    gaia_log::trace("trace");
    gaia_log::trace("trace const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
    gaia_log::debug("debug");
    gaia_log::debug("debug const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
    gaia_log::info("info");
    gaia_log::info("info const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
    gaia_log::warn("warn");
    gaia_log::warn("warn const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
    gaia_log::error("error");
    gaia_log::error("error const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
    gaia_log::critical("critical");
    gaia_log::critical("critical const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);

    gaia_log::log(gaia_log::log_level_t::info, "Static message");
    gaia_log::log(gaia_log::log_level_t::info, "Dynamic const char*: '{}', std::string: '{}', number: '{}'", const_char_msg, string_msg, int_msg);
}

TEST_F(test_logging_t, create_existing_logger) {
    gaia_log::gaia_logger_t logger(gaia_log::c_gaia_root_logger);
    ASSERT_EQ(logger.get_name(), gaia_log::c_gaia_root_logger);

    logger.trace("Trace message");
}

TEST_F(test_logging_t, create_new_logger) {
    gaia_log::gaia_logger_t logger(bubu_logger);
    ASSERT_EQ(logger.get_name(), bubu_logger);
    logger.trace("Trace message from BUBU");
}

TEST_F(test_logging_t, default_logger_available) {
    auto logger = gaia_log::default_logger();
    logger->trace("Message from default logger");
}

TEST_F(test_logging_t, register_new_logger) {
    auto logger = make_shared<gaia_log::gaia_logger_t>(bubu_logger);
    gaia_log::register_logger(logger);
    gaia_log::get(bubu_logger)->trace("Message from the registry");
}

TEST_F(test_logging_t, register_duplicated_logger) {
    auto logger1 = make_shared<gaia_log::gaia_logger_t>(bubu_logger);
    gaia_log::register_logger(logger1);

    auto logger2 = make_shared<gaia_log::gaia_logger_t>(bubu_logger);

    EXPECT_THROW(
        gaia_log::register_logger(logger1),
        gaia_log::logger_exception_t);
}
*/