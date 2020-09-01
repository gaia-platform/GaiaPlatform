//
// Created by simone on 8/28/20.
//

#ifndef PRODUCTION_GAIA_LOGGING_HPP
#define PRODUCTION_GAIA_LOGGING_HPP
class logging {
};

//#include "spdlog/spdlog.h"
//#include "spdlog/sinks/basic_file_sink.h"
//
//int main()
//{
//    spdlog::info("Welcome to Gaia Platform!");
//    spdlog::error("Some error message with arg: {}", 1);
//
//    spdlog::warn("Easy padding in numbers like {:08d}", 12);
//    spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
//    spdlog::info("Support for floats {:03.2f}", 1.23456);
//    spdlog::info("Positional args are {1} {0}..", "too", "supported");
//    spdlog::info("{:<30}", "left aligned");
//
//    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
//    spdlog::debug("This message should be displayed..");
//
//    // change log pattern
//    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
//
//    // Compile time log levels
//    // define SPDLOG_ACTIVE_LEVEL to desired level
//    SPDLOG_TRACE("Some trace message with param {}", 42);
//    SPDLOG_DEBUG("Some debug message");
//
//    // Set the default logger to file logger
//    auto file_logger = spdlog::basic_logger_mt("basic_logger", "logs/basic.txt");
//    spdlog::set_default_logger(file_logger);

//}

#include <iostream>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/syslog_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h" // or "../stdout_sinks.h" if no colors needed
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include "glog/logging.h"

#include <chrono>

using namespace std;
long long NUM_LOGS = 1024 * 1024;

void printStats(const string &message, long duration_micros) {
    auto total_time_ms = duration_micros / 1000;
    auto log_time = (double)duration_micros / (double)NUM_LOGS;
    cout << message << ": " << total_time_ms << "millis -> " << log_time << "micros/message" << endl;
}

void baseline() {
    uint64_t acc = 0;

    for (int i = 0; i < NUM_LOGS; i++) {
        acc += i;
    }
}

void spdlog_default() {
    freopen("/dev/null", "w", stderr);
    auto err_logger = spdlog::stderr_color_mt(__FUNCTION__);
    spdlog::set_default_logger(err_logger);
    spdlog::set_level(spdlog::level::level_enum::info);

    for (int i = 0; i < NUM_LOGS; i++) {
        spdlog::info("Welcome to Gaia Platform!");
    }
}

void spdlog_file() {
    auto file_logger = spdlog::basic_logger_mt(__FUNCTION__, "logs/basic.txt");
    spdlog::set_default_logger(file_logger);

    for (int i = 0; i < NUM_LOGS; i++) {
        spdlog::info("Welcome to Gaia Platform!");
    }
}

void spdlog_async_file() {

    //    auto console_sink = make_shared<spdlog::sinks::stderr_color_sink_mt>();
    //    console_sink->set_level(spdlog::level::warn);
    //    console_sink->set_pattern("[multi_sink_example] [%^%l%$] %v");

    auto file_sink = make_shared<spdlog::sinks::basic_file_sink_mt>("logs/gaia.txt", true);
    file_sink->set_level(spdlog::level::trace);

    spdlog::sinks_init_list sink_list = {file_sink};

    // It has to be a shared pointer. This is by design because the logger is shared with the logging thread.
    auto logger = make_shared<spdlog::async_logger>("gaia", sink_list.begin(), sink_list.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);

    for (int i = 0; i < NUM_LOGS; i++) {
        logger->critical("Welcome to Gaia Platform!");
    }

    logger->flush();
}

void spdlog_async_multiple() {

    auto console_sink = make_shared<spdlog::sinks::stderr_color_sink_mt>();
    console_sink->set_level(spdlog::level::trace);
    console_sink->set_pattern("[multi_sink_example] [%^%l%$] %v");

    auto file_sink = make_shared<spdlog::sinks::basic_file_sink_mt>("logs/gaia.txt", true);
    file_sink->set_level(spdlog::level::trace);


    std::string syslog_ident = "spdlog-example";
    int syslog_option = 0;
    int syslog_facility = LOG_USER;
    bool enable_formatting = false;
    auto syslog_sink = make_shared<spdlog::sinks::syslog_sink_mt>(syslog_ident, syslog_option, syslog_facility, enable_formatting);
    syslog_sink->set_level(spdlog::level::trace);

    spdlog::sinks_init_list sink_list{file_sink, console_sink, syslog_sink};

    // It has to be a shared pointer. This is by design because the logger is shared with the logging thread.
    auto logger = make_shared<spdlog::async_logger>("gaia", sink_list.begin(), sink_list.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
//    spdlog::set_default_logger(logger)


    for (int i = 0; i < NUM_LOGS; i++) {
//        SPDLOG_ERROR("Welcome to Gaia Platform!");
        logger->critical("Welcome to Gaia Platform!");
    }
    SPDLOG_ERROR
    logger->flush();

}

void spdlog_disabled() {
    auto file_logger = spdlog::basic_logger_mt(__FUNCTION__, "logs/basic.txt");
    spdlog::set_default_logger(file_logger);

    for (int i = 0; i < NUM_LOGS; i++) {
        spdlog::debug("Welcome to Gaia Platform!");
    }
}

void glog_default() {
    for (int i = 0; i < NUM_LOGS; i++) {
        LOG(INFO) << "Welcome to Gaia Platform!";
    }
}

void test_performance(void (*f)(), const string &label) {
    auto t1 = chrono::high_resolution_clock::now();
    f();
    auto t2 = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();

    printStats(label, duration);
}

int main(int, char *[]) {
    try {
        test_performance(baseline, "baseline");

        test_performance(spdlog_default, "spdlog_default");
        test_performance(spdlog_file, "spdlog_file");
        test_performance(spdlog_disabled, "spdlog_disabled");
        spdlog::init_thread_pool(1000000, 4);
        test_performance(spdlog_async_file, "spdlog_async_file");
        test_performance(spdlog_async_multiple, "spdlog_async_multiple");

        google::InitGoogleLogging("Gaia Platform");
        test_performance(glog_default, "glog_file");
        google::ShutdownGoogleLogging();

        setenv("GLOG_logtostderr", "1", 0);
        google::InitGoogleLogging("Gaia Platform");
        test_performance(glog_default, "glog_stderr");
        google::ShutdownGoogleLogging();

        //        freopen("/dev/null", "w", stderr);
        //
        //        spdlog::init_thread_pool(8192, 2);
        //
        //        auto console_sink = make_shared<spdlog::sinks::stderr_color_sink_mt>();
        //        console_sink->set_level(spdlog::level::warn);
        //        console_sink->set_pattern("[multi_sink_example] [%^%l%$] %v");
        //
        //        auto file_sink = make_shared<spdlog::sinks::basic_file_sink_mt>("logs/gaia.txt", true);
        //        file_sink->set_level(spdlog::level::trace);
        //
        //        spdlog::sinks_init_list sink_list = {file_sink, console_sink};
        //
        //        // It has to be a shared pointer. This is by design because the logger is shared with the logging thread.
        //        auto logger = make_shared<spdlog::async_logger>("gaia", sink_list.begin(), sink_list.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::overrun_oldest);
        //        logger->set_level(spdlog::level::debug);
        //
        //        // Static logging
        //
        //        auto t1 = chrono::high_resolution_clock::now();
        //        for (int i = 0; i < NUM_LOGS; i++) {
        //            logger->warn("Static logging");
        //        }
        //        auto t2 = chrono::high_resolution_clock::now();
        //        auto duration = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
        //
        //        printStats("Static", duration);
        //
        //        // Formatting 1 arg
        //        string text("fixed_text");
        //        t1 = chrono::high_resolution_clock::now();
        //        for (int i = 0; i < NUM_LOGS; i++) {
        //            logger->warn("String: {}", text);
        //        }
        //        t2 = chrono::high_resolution_clock::now();
        //        duration = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
        //
        //        printStats("Formatting 1 argument: ", duration);
        //
        //        // Formatting 2 args
        //        t1 = chrono::high_resolution_clock::now();
        //        for (int i = 0; i < NUM_LOGS; i++) {
        //            logger->warn("String: {} and Integer {}", text, i);
        //        }
        //        t2 = chrono::high_resolution_clock::now();
        //        duration = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
        //
        //        printStats("Formatting 2 arguments: ", duration);

    } catch (const spdlog::spdlog_ex &ex) {
        cout << "Log initialization failed: " << ex.what() << endl;
    }
}

#endif //PRODUCTION_GAIA_LOGGING_HPP
