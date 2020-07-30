/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdlib>
#include <random>
#include <string>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include "process.hpp"
#include "gtest/gtest.h"

#include "retail_assert.hpp"
#include "system_error.hpp"
#include "gaia_db.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace TinyProcessLib;

namespace gaia {
namespace db {

class db_test_base_t : public ::testing::Test {
protected:
    db_test_base_t(bool manages_session) : m_manages_session(manages_session) {
    }

    db_test_base_t() : db_test_base_t(true) {
    }

    static void SetUpTestSuite() {
        // Force the client to connect using our random socket name.
        set_server_socket_name(get_socket_name().c_str());
        start_server();
    }

    static void TearDownTestSuite() {
        stop_server();
    }

    void SetUp() override {
        if (m_manages_session) {
            begin_session();
        }
    }

    void TearDown() override {
        if (m_manages_session) {
            end_session();
        }
    }

private:
    bool m_manages_session;
    static constexpr size_t SOCKET_NAME_LEN = 16;

    static const std::string& get_socket_name() {
        static std::string socket_name = compute_socket_name();
        return socket_name;
    }

    static std::string compute_socket_name() {
        std::string str("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
        std::random_device rd;
        std::mt19937 generator(rd());
        std::shuffle(str.begin(), str.end(), generator);
        return str.substr(0, SOCKET_NAME_LEN);  // assumes SOCKET_NAME_LEN < number of characters in str
    }

    static Process& get_server_proc() {
        std::string cmd = std::string(SE_SERVER_NAME) + " " + get_socket_name();
        static Process server_proc(cmd);
        return server_proc;
    }

    static void start_server() {
        static constexpr int POLL_INTERVAL_MILLIS = 10;
        // Launch server process with random socket name.
        get_server_proc();
        // Wait for server to initialize.
        while (true) {
            try {
                begin_session();
            } catch (system_error& ex) {
                if (ex.get_errno() == ECONNREFUSED) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MILLIS));
                    continue;
                } else {
                    throw;
                }
            } catch (...) {
                throw;
            }
            break;
        }
        // This was just a test connection, so disconnect.
        end_session();
    }

    static void stop_server() {
        // Send SIGTERM to server process.
        Process& server_proc = get_server_proc();
        server_proc.kill(true);
        int exit_status = server_proc.get_exit_status();
        // The exit status should reflect catching SIGTERM (numeric value 15).
        EXPECT_EQ(exit_status, 15);
    }
};

}  // namespace db
}  // namespace gaia
