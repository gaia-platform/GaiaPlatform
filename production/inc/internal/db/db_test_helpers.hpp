/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include <chrono>
#include <cstdlib>

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "db_types.hpp"
#include "gaia_db.hpp"
#include "gaia_db_internal.hpp"
#include "logger.hpp"
#include "system_error.hpp"

namespace gaia
{
namespace db
{

constexpr char c_daemonize_command[] = "daemonize ";

void remove_persistent_store()
{
    string cmd = "rm -rf ";
    cmd.append(PERSISTENT_DIRECTORY_PATH);
    cerr << cmd << endl;
    ::system(cmd.c_str());
}

void wait_for_server_init()
{
    constexpr int c_poll_interval_millis = 10;
    constexpr int c_print_error_interval = 1000;
    int counter = 0;

    // quick fix to initialize the server.
    gaia_log::initialize({});

    // Wait for server to initialize.
    while (true)
    {
        try
        {
            begin_session();
        }
        catch (system_error& ex)
        {
            if (ex.get_errno() == ECONNREFUSED)
            {
                if (counter % c_print_error_interval == 0)
                {
                    gaia_log::sys().warn(
                        "Cannot connect to Gaia Server, you may need to start the gaia_se_server process");
                    counter = 1;
                }
                else
                {
                    counter++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(c_poll_interval_millis));
                continue;
            }
            else
            {
                throw;
            }
        }
        catch (...)
        {
            throw;
        }
        break;
    }
    // This was just a test connection, so disconnect.
    end_session();
}

void reset_server()
{
    // We need to allow enough time after the signal is sent for the process to
    // receive and process the signal.
    constexpr int c_wait_signal_millis = 10;
    // We need to drop all client references to shared memory before resetting the server.
    // NB: this cannot be called within an active session!
    clear_shared_memory();
    // Reinitialize the server (forcibly disconnects all clients and clears database).
    // Resetting the server will cause Recovery to be skipped. Recovery will only occur post
    // server process reboot.
    ::system((std::string("pkill -f -HUP ") + SE_SERVER_EXEC_NAME).c_str());
    // Wait a bit for the server's listening socket to be closed.
    // (Otherwise, a new session might be accepted after the signal has been sent
    // but before the server has been reinitialized.)
    std::this_thread::sleep_for(std::chrono::milliseconds(c_wait_signal_millis));
    // WLW Note: This is temporary.
    string boot_file_name(PERSISTENT_DIRECTORY_PATH);
    boot_file_name += "/boot_parameters.bin";
    truncate(boot_file_name.c_str(), 0);
    wait_for_server_init();
}

class db_server_t
{
public:
    void start(const char* db_server_path, bool stop_server = true)
    {
        set_path(db_server_path);

        if (stop_server)
        {
            stop();
        }

        // Launch SE server in background.
        string cmd = c_daemonize_command + m_server_path;
        cerr << cmd << endl;
        ::system(cmd.c_str());

        // Wait for server to initialize.
        cerr << "Waiting for server to initialize..." << endl;
        wait_for_server_init();
        m_server_started = true;
    }

    void stop()
    {
        // Try to kill the SE server process.
        // REVIEW: we should be using a proper process library for this, so we can kill by PID.
        string cmd = "pkill -f -KILL ";
        cmd.append(m_server_path.c_str());
        cerr << cmd << endl;
        ::system(cmd.c_str());
    }

    bool server_started()
    {
        return m_server_started;
    }

    // Add a trailing '/' if not provided.
    static void terminate_path(string& path)
    {
        if (path.back() != '/')
        {
            path.append("/");
        }
    }

private:
    void set_path(const char* db_server_path)
    {
        if (!db_server_path)
        {
            m_server_path = gaia::db::SE_SERVER_EXEC_NAME;
        }
        else
        {
            m_server_path = db_server_path;
            terminate_path(m_server_path);
            m_server_path.append(gaia::db::SE_SERVER_EXEC_NAME);
        }
    }

    string m_server_path;
    bool m_server_started = false;
};

} // namespace db
} // namespace gaia
