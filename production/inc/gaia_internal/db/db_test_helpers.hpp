/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <unistd.h>

#include <chrono>
#include <cstdlib>

#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include "gaia/db/db.hpp"

#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/common/system_error.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

namespace gaia
{
namespace db
{

constexpr char c_daemonize_command[] = "daemonize ";
// Duplicated from production/db/core/inc/db_server.hpp.
// (That header should not be included by anything but the code that
// instantiates the server.)
constexpr char c_disable_persistence_flag[] = "--disable-persistence";
constexpr char c_reinitialize_persistent_store_flag[] = "--reinitialize-persistent-store";
constexpr char c_data_dir_flag[] = "--data-dir ~/.local/gaia/db";

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
        catch (gaia::common::system_error& ex)
        {
            if (ex.get_errno() == ECONNREFUSED)
            {
                if (counter % c_print_error_interval == 0)
                {
                    gaia_log::sys().warn(
                        "Cannot connect to Gaia Server; the 'gaia_db_server' process may not be running!");
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
    ::system((std::string("pkill -f -HUP ") + c_db_server_exec_name).c_str());
    // Wait a bit for the server's listening socket to be closed.
    // (Otherwise, a new session might be accepted after the signal has been sent
    // but before the server has been reinitialized.)
    std::this_thread::sleep_for(std::chrono::milliseconds(c_wait_signal_millis));
    wait_for_server_init();
}

class server_t
{
public:
    server_t()
    {
        set_path(nullptr);
    }

    explicit server_t(
        const char* db_server_path,
        bool disable_persistence = false)
        : m_disable_persistence(disable_persistence)
    {
        set_path(db_server_path);
    }

    void inline start(bool stop_server = true, bool remove_persistent_store = true)
    {
        if (stop_server)
        {
            stop();
        }

        // Launch SE server in background.
        std::string cmd = c_daemonize_command + m_server_path.string();
        if (m_disable_persistence)
        {
            cmd.append(" ");
            cmd.append(c_disable_persistence_flag);
        }
        if (remove_persistent_store)
        {
            cmd.append(" ");
            cmd.append(c_reinitialize_persistent_store_flag);
            cmd.append(" ");
            cmd.append(c_data_dir_flag);
        }
        ::system(cmd.c_str());

        // Wait for server to initialize.
        wait_for_server_init();
        m_server_started = true;
    }

    void inline start_and_retain_persistent_store()
    {
        bool stop_server = true, remove_persistent_store = false;
        start(stop_server, remove_persistent_store);
    }

    void inline stop()
    {
        // Try to kill the SE server process.
        // REVIEW: we should be using a proper process library for this, so we can kill by PID.
        std::string cmd = "pkill -f -KILL ";
        cmd.append(m_server_path.string());
        cmd.append(m_server_path);
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
            m_server_path = gaia::db::c_db_server_exec_name;
        }
        else
        {
            m_server_path = db_server_path;
            m_server_path /= gaia::db::c_db_server_exec_name;
        }
    }

    void inline disable_persistence()
    {
        m_disable_persistence = true;
    }

private:
    std::filesystem::path m_server_path;
    bool m_disable_persistence = false;
    bool m_server_started = false;
};

} // namespace db
} // namespace gaia
