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

class db_server_t {
  public:
    void start(const char *db_server_path, bool stop_server = true) {
        set_path(db_server_path);
        
        if (stop_server) {
            stop();
        }

        // Launch SE server in background.
        string cmd = m_server_path + " &";
        cerr << cmd << endl;
        ::system(cmd.c_str());

        // Wait for server to initialize.
        // Ideally the client shouldn't come up until the server is up; maybe 
        // this should be programmed in begin_session()?
        cerr << "Waiting for server to initialize..." << endl;
        ::sleep(3);
        m_server_started = true;
    }

    void stop() {
        // Try to kill the SE server process.
        // REVIEW: we should be using a proper process library for this, so we can kill by PID.
        string cmd = "pkill -f -KILL ";
        cmd.append(m_server_path.c_str());
        cerr << cmd << endl;
        ::system(cmd.c_str());
    }

    void sigterm_stop() {
        string cmd = "pkill -f ";
        cmd.append(m_server_path.c_str());
        cerr << cmd << endl;
        ::system(cmd.c_str());
    }

    bool server_started() {
        return m_server_started;
    }

    // Add a trailing '/' if not provided.
    static void terminate_path(string &path) {
        if (path.back() != '/') {
            path.append("/");
        }
    }

  private:
    
    void set_path(const char *db_server_path) {
        if (!db_server_path) {
            m_server_path = gaia::db::SE_SERVER_NAME;
        } else {
            m_server_path = db_server_path;
            terminate_path(m_server_path);
            m_server_path.append(gaia::db::SE_SERVER_NAME);
        }
    }

    string m_server_path;
    bool m_server_started = false;
};

class db_test_base_t : public ::testing::Test {
public:
    static void remove_persistent_store() {
        string cmd = "rm -rf ";
        cmd.append(PERSISTENT_DIRECTORY_PATH);
        cerr << cmd << endl;
        ::system(cmd.c_str());
    }
private:
    bool m_client_manages_session;

protected:
    static void reset_server() {
        // We need to drop all client references to shared memory before resetting the server.
        // NB: this cannot be called within an active session!
        clear_shared_memory();
        static constexpr int POLL_INTERVAL_MILLIS = 10;
        // Reinitialize the server (forcibly disconnects all clients and clears database).
        // Resetting the server will cause Recovery to be skipped. Recovery will only occur post 
        // server process reboot. 
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
        // remove_persistent_store();
    }
};

}  // namespace db
}  // namespace gaia
