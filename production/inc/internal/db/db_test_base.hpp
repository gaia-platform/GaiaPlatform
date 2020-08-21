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
#include "gtest/gtest.h"

#include "retail_assert.hpp"
#include "system_error.hpp"
#include "gaia_db.hpp"
#include "gaia_db_internal.hpp"

using namespace gaia::common;
using namespace gaia::db;

namespace gaia {
namespace db {

class db_test_base_t : public ::testing::Test {
private:
    bool m_client_manages_session;

protected:
    static void reset_server() {
        // We need to drop all client references to shared memory before resetting the server.
        // NB: this cannot be called within an active session!
        clear_shared_memory();
        static constexpr int POLL_INTERVAL_MILLIS = 10;
        // Reinitialize the server (forcibly disconnects all clients and clears database).
        ::system((std::string("pkill -f -HUP ") + SE_SERVER_NAME).c_str());
        // Wait a bit for the server's listening socket to be closed.
        // (Otherwise, a new session might be accepted after the signal has been sent
        // but before the server has been reinitialized.)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

    db_test_base_t(bool client_manages_session) : m_client_manages_session(client_manages_session) {
    }

    db_test_base_t() : db_test_base_t(false) {
    }

    // Since ctest always launches each gtest in a new process, there is no point
    // to defining separate SetUpTestSuite/TearDownTestSuite methods.  However, tests
    // that need to do one-time initialization when running outside of ctest
    // can provide SetUpTestSuite/TearDownTestSuite methods and call reset_server()
    // themselves.  These tests should also override SetUp() and TearDown()
    // methods to ensure that the server isn't reset for every test case.

    void SetUp() override {
        reset_server();
        if (!m_client_manages_session) {
            begin_session();
        }
    }

    void TearDown() override {
        if (!m_client_manages_session) {
            end_session();
        }
    }
};

}  // namespace db
}  // namespace gaia
