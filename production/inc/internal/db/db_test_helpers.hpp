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
// Duplicated from production/db/storage_engine/inc/storage_engine_server.hpp.
// (That header should not be included by anything but the code that
// instantiates the server.)
constexpr char c_disable_persistence_flag[] = " --disable-persistence";

// Erase the contents of the persistent directory but not the directory itself.
inline void remove_persistent_store()
{
    std::string cmd = "rm -rf ";
    cmd.append(PERSISTENT_DIRECTORY_PATH);
    cmd.append("/*");
    std::cerr << cmd << std::endl;
    ::system(cmd.c_str());
}

inline void wait_for_server_init()
{
    constexpr int c_poll_interval_millis = 10;
    constexpr int c_print_error_interval = 1000;
    // Initialize to 1 to avoid printing a spurious wait message.
    int counter = 1;

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

inline void reset_server()
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
    std::string boot_file_name(PERSISTENT_DIRECTORY_PATH);
    boot_file_name += "/boot_parameters.bin";
    truncate(boot_file_name.c_str(), 0);
    wait_for_server_init();
}

class db_server_t
{
public:
    db_server_t()
    {
        set_path(nullptr);
    }

    db_server_t(const char* db_server_path, bool disable_persistence = false)
        : m_disable_persistence(disable_persistence)
    {
        set_path(db_server_path);
    }

    void inline start(bool stop_server = true)
    {
        if (stop_server)
        {
            stop();
        }

        // Launch SE server in background.
        std::string cmd = c_daemonize_command + m_server_path;
        if (m_disable_persistence)
        {
            cmd.append(c_disable_persistence_flag);
        }
        std::cerr << cmd << std::endl;
        ::system(cmd.c_str());

        // Wait for server to initialize.
        std::cerr << "Waiting for server to initialize..." << std::endl;
        wait_for_server_init();
        m_server_started = true;
    }

    void inline stop()
    {
        // Try to kill the SE server process.
        // REVIEW: we should be using a proper process library for this, so we can kill by PID.
        std::string cmd = "pkill -f -KILL ";
        cmd.append(m_server_path);
        std::cerr << cmd << std::endl;
        ::system(cmd.c_str());
    }

    bool inline server_started()
    {
        return m_server_started;
    }

    void inline set_path(const char* db_server_path)
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

private:
    // Add a trailing '/' if not provided.
    static void inline terminate_path(std::string& path)
    {
        if (path.back() != '/')
        {
            path.append("/");
        }
    }

    std::string m_server_path;
    bool m_disable_persistence = false;
    bool m_server_started = false;
};

} // namespace db
} // namespace gaia
